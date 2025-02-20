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

struct ivshmem_uio_hook_data {
        int limit_map_size;
        int (*origin_mmap)(struct file *file, struct vm_area_struct *vma);
};

struct ivshmem_context {
        unsigned long target_addr;

        struct ftrace_ops *ops;
        struct ivshmem_uio_hook_data *hook_data;
};

static struct ivshmem_context *__ivshmem_ctx_create(void);
static int __ivshmem_ctx_destroy(struct ivshmem_context *del);

static struct ftrace_ops *__ivshmem_ops_create(void);
static int __ivshmem_ops_destroy(struct ftrace_ops *del);

static struct ivshmem_uio_hook_data *__ivshmem_hook_data_create(void);
static int __ivshmem_hook_data_destroy(struct ivshmem_uio_hook_data *del);

static inline int __ivshmem_uio_hook_exit(struct ivshmem_context *ctx);

static struct ivshmem_context *g_ctx = NULL;

static int __init ivshmem_uio_hook_init(void) {
        FORMULA_GUARD(g_ctx != NULL, -EINVAL, ERR_INVALID_PTR);

        struct ivshmem_context *ctx = __ivshmem_ctx_create();
        FORMULA_GUARD_WITH_EXIT_FUNC(ctx == NULL, -ENOMEM,
                                     __ivshmem_uio_hook_exit, "");

        struct ftrace_ops *ops = __ivshmem_ops_create();
        FORMULA_GUARD_WITH_EXIT_FUNC(ops == NULL, -ENOMEM,
                                     __ivshmem_uio_hook_exit, "");

        struct ivshmem_uio_hook_data *hook_data = __ivshmem_hook_data_create();
        FORMULA_GUARD_WITH_EXIT_FUNC(hook_data == NULL, -ENOMEM,
                                     __ivshmem_uio_hook_exit, "");

        ctx->ops = ops;
        ctx->hook_data = hook_data;
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

        return new;
}

static int __ivshmem_ctx_destroy(struct ivshmem_context *del) {
        FORMULA_GUARD(del == NULL, -EINVAL, ERR_INVALID_PTR);

        PTR_FREE(del);

        return 0;
}

static struct ftrace_ops *__ivshmem_ops_create(void) {
        struct ftrace_ops *new = kmalloc(sizeof(struct ftrace_ops), GFP_KERNEL);
        FORMULA_GUARD(new == NULL, NULL, ERR_MEMORY_SHORTAGE);

        new->func = NULL;
        new->flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION;

        return new;
}

static int __ivshmem_ops_destroy(struct ftrace_ops *del) {
        FORMULA_GUARD(del == NULL, -EINVAL, ERR_INVALID_PTR);

        PTR_FREE(del);

        return 0;
}

static struct ivshmem_uio_hook_data *__ivshmem_hook_data_create(void) {
        struct ivshmem_uio_hook_data *new =
            kmalloc(sizeof(struct ivshmem_uio_hook_data), GFP_KERNEL);
        FORMULA_GUARD(new == NULL, NULL, ERR_MEMORY_SHORTAGE);

        new->limit_map_size = 0;
        new->origin_mmap = NULL;

        return new;
}

static int __ivshmem_hook_data_destroy(struct ivshmem_uio_hook_data *del) {
        FORMULA_GUARD(del == NULL, -EINVAL, ERR_INVALID_PTR);

        PTR_FREE(del);

        return 0;
}

static inline int __ivshmem_uio_hook_exit(struct ivshmem_context *ctx) {
        FORMULA_GUARD(ctx == NULL, -EINVAL, ERR_INVALID_PTR);

        __ivshmem_hook_data_destroy(ctx->hook_data);
        __ivshmem_ops_destroy(ctx->ops);
        __ivshmem_ctx_destroy(ctx);

        return 0;
}

module_init(ivshmem_uio_hook_init);
module_exit(ivshmem_uio_hook_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lee, Jerry J");
MODULE_DESCRIPTION("PCI UIO generic driver hooking module for the ivshmem.");
