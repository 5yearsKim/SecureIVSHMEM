/*
 * INTEL CONFIDENTIAL
 * Copyright (c) 2025 Intel Corporation All Rights Reserved
 *
 * The source code contained or described herein and all documents related
 * to the source code ("Material") are owned by Intel Corporation or its
 * suppliers or licensors. Title to the Material remains with Intel
 * Corporation or its suppliers and licensors. The Material contains trade
 * secrets and proprietary and confidential information of Intel or its
 * suppliers and licensors. The Material is protected by worldwide copyright
 * and trade secret laws and treaty provisions. No part of the Material may
 * be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intel's prior
 * express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or
 * delivery of the Materials, either expressly, by implication, inducement,
 * estoppel or otherwise. Any license under such intellectual property rights
 * must be express and approved by Intel in writing.
 */

#include <linux/fs.h>
#include <linux/ftrace.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/version.h>

#define ERR_INVALID_PTR "Invalid Pointer."
#define ERR_INVALID_RET "Invalild return value."
#define ERR_INVALID_FD "Invalid File Descripter."
#define ERR_INVALID_PARAMS "Invalid Parameters."
#define ERR_MEMORY_SHORTAGE "Failed to allocate the memory."

#define DBG_MSG(fmt, ...)                                                      \
        do {                                                                   \
                pr_info("[IAPG] " fmt " / func : %s, line : %d\n",             \
                        ##__VA_ARGS__, __func__, __LINE__);                    \
        } while (0)

#define ERR_MSG(fmt, ...)                                                      \
        do {                                                                   \
                pr_err("[IAPG ERR] " fmt " / func : %s, line : %d\n",          \
                       ##__VA_ARGS__, __func__, __LINE__);                     \
        } while (0)

#define FORMULA_GUARD(formula, ret, msg, ...)                                  \
        do {                                                                   \
                if (unlikely(formula)) {                                       \
                        if (strcmp(msg, ""))                                   \
                                ERR_MSG(msg, ##__VA_ARGS__);                   \
                        return ret;                                            \
                }                                                              \
        } while (0)

#define FORMULA_GUARD_WITH_EXIT_FUNC(formula, ret, func, msg, ...)             \
        do {                                                                   \
                if (unlikely(formula)) {                                       \
                        if (strcmp(msg, ""))                                   \
                                ERR_MSG(msg, ##__VA_ARGS__);                   \
                        return func, ret;                                      \
                }                                                              \
        } while (0)

#define PTR_FREE(ptr)                                                          \
        do {                                                                   \
                if (ptr) {                                                     \
                        kfree(ptr);                                            \
                        ptr = NULL;                                            \
                }                                                              \
        } while (0)

enum ivshmem_ftrace_e {
        REGISTER,
        REMOVE,
        RESET = 0,
        SET,
};

struct ivshmem_context {
        unsigned long target_addr;
        unsigned long limit_map_size;

        struct ftrace_ops *ops;
};

static struct ivshmem_context *__ivshmem_ctx_create(void);
static struct ftrace_ops *__ivshmem_ops_create(void);

static void notrace __ivshmem_ftrace_hook_func(unsigned long ip,
                                               unsigned long parent_ip,
                                               struct ftrace_ops *ops,
                                               struct ftrace_regs *regs);
static int __ivshmem_dummy_mmap(struct file *file, struct vm_area_struct *vma);

static inline int __ivshmem_uio_hook_destroy(void *del);
static inline int __ivshmem_uio_hook_exit(struct ivshmem_context *ctx);

static struct ivshmem_context *g_ctx = NULL;

static int __init ivshmem_uio_hook_init(void) {
        FORMULA_GUARD(g_ctx != NULL, -EINVAL, ERR_INVALID_PTR);

        struct ivshmem_context *ctx = __ivshmem_ctx_create();
        FORMULA_GUARD(ctx == NULL, -ENOMEM, "");

        struct ftrace_ops *ops = __ivshmem_ops_create();
        FORMULA_GUARD_WITH_EXIT_FUNC(ops == NULL, -ENOMEM,
                                     __ivshmem_uio_hook_exit(ctx), "");

        /* init the chaining */
        ctx->ops = ops;
        ops->private = ctx;

        /* set the ftrace */
        int ret =
            ftrace_set_filter_ip(ctx->ops, ctx->target_addr, REGISTER, RESET);
        FORMULA_GUARD_WITH_EXIT_FUNC(ret < 0, ret, __ivshmem_uio_hook_exit(ctx),
                                     ERR_INVALID_RET);

        ret = register_ftrace_function(ctx->ops);
        FORMULA_GUARD_WITH_EXIT_FUNC(ret < 0, ret, __ivshmem_uio_hook_exit(ctx),
                                     ERR_INVALID_RET);

        /* for module_exit */
        g_ctx = ctx;

        return 0;
}

static void __exit ivshmem_uio_hook_exit(void) {
        __ivshmem_uio_hook_exit(g_ctx);
}

static struct ivshmem_context *__ivshmem_ctx_create(void) {
        struct ivshmem_context *new =
            kmalloc(sizeof(struct ivshmem_context), GFP_KERNEL);
        FORMULA_GUARD(new == NULL, NULL, ERR_MEMORY_SHORTAGE);

        struct kprobe kp = {
            .symbol_name = "uio_mmap",
        };

        int ret = register_kprobe(&kp);
        FORMULA_GUARD_WITH_EXIT_FUNC(
            ret < 0, NULL, __ivshmem_uio_hook_destroy(new), ERR_INVALID_RET);

        new->target_addr = (unsigned long)kp.addr;
        unregister_kprobe(&kp);
        FORMULA_GUARD_WITH_EXIT_FUNC(new->target_addr == 0, NULL,
                                     __ivshmem_uio_hook_destroy(new),
                                     ERR_INVALID_RET);

        new->limit_map_size = 0x1000; /* 4KB */

        return new;
}

static struct ftrace_ops *__ivshmem_ops_create(void) {
        struct ftrace_ops *new = kmalloc(sizeof(struct ftrace_ops), GFP_KERNEL);
        FORMULA_GUARD(new == NULL, NULL, ERR_MEMORY_SHORTAGE);

        new->func = __ivshmem_ftrace_hook_func;
        new->flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION;

        return new;
}

static void notrace __ivshmem_ftrace_hook_func(unsigned long ip,
                                               unsigned long parent_ip,
                                               struct ftrace_ops *ops,
                                               struct ftrace_regs *regs) {
        FORMULA_GUARD(ops == NULL || regs == NULL, , ERR_INVALID_PTR);

        struct ivshmem_context *ctx = (struct ivshmem_context *)ops->private;
        FORMULA_GUARD(ctx == NULL, , ERR_INVALID_PTR);

        struct pt_regs *pt_regs;
        struct file *file;
        struct vm_area_struct *vma;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0)
        pt_regs = arch_ftrace_get_regs(regs);
        file = (struct file *)ftrace_regs_get_argument(regs, 0);
        vma = (struct vm_area_struct *)ftrace_regs_get_argument(regs, 1);
#else
        pt_regs = (struct pt_regs *)regs;
        file = (struct file *)pt_regs->di;
        vma = (struct vm_area_struct *)pt_regs->si;
#endif
        FORMULA_GUARD(file == NULL || vma == NULL, , ERR_INVALID_PTR);

        if (file->f_path.dentry) {
                const char *dname = file->f_path.dentry->d_name.name;

                /* only check from the /dev/uioX */
                if (strncmp(dname, "uio", 3) == 0) {
                        size_t length = vma->vm_end - vma->vm_start;
                        if (length > ctx->limit_map_size) {
                                /* call the dummy mmap */
                                instruction_pointer_set(
                                    pt_regs, __ivshmem_dummy_mmap(file, vma));
                                return;
                        }
                }
        }

        /* origin mmap */
        instruction_pointer_set(pt_regs, ctx->target_addr);
}

static int __ivshmem_dummy_mmap(struct file *file, struct vm_area_struct *vma) {
        FORMULA_GUARD(1, -EINVAL, "Mapping size exceeds allowed limit!");
}

static inline int __ivshmem_uio_hook_destroy(void *del) {
        FORMULA_GUARD(del == NULL, -EINVAL, ERR_INVALID_PTR);

        PTR_FREE(del);

        return 0;
}

static inline int __ivshmem_uio_hook_exit(struct ivshmem_context *ctx) {
        FORMULA_GUARD(ctx == NULL, -EINVAL, ERR_INVALID_PTR);

        unregister_ftrace_function(ctx->ops);
        ftrace_set_filter_ip(ctx->ops, ctx->target_addr, REMOVE, RESET);

        __ivshmem_uio_hook_destroy(ctx->ops);
        __ivshmem_uio_hook_destroy(ctx);

        return 0;
}

module_init(ivshmem_uio_hook_init);
module_exit(ivshmem_uio_hook_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lee, Jerry J");
MODULE_DESCRIPTION("PCI UIO generic driver hooking module for the ivshmem.");
