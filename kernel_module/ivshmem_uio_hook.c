/*
 * copyright: Lee, Jerry J (jerry.j.lee@intel.com)
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

static struct ivshmem_context *g_ctx = NULL;

static int __init ivshmem_uio_hook_init(void) {
        FORMULA_GUARD(g_ctx != NULL, -EPERM, ERR_INVALID_PTR);
        return 0;
}

static void __exit ivshmem_uio_hook_exit(void) {
        FORMULA_GUARD(g_ctx == NULL, , ERR_INVALID_PTR);
}

module_init(ivshmem_uio_hook_init);
module_exit(ivshmem_uio_hook_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lee, Jerry J");
MODULE_DESCRIPTION("PCI UIO generic driver hooking module for the ivshmem.");
