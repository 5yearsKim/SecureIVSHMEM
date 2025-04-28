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

#ifndef HOOK_DEV
#define HOOK_DEV 2 // 1: UIO, 2: MYSHM
#endif

#if HOOK_DEV == 1
#define HOOK_MMAP "uio_mmap"
#define HOOK_DNAME "uio"
#elif HOOK_DEV == 2
#define HOOK_MMAP "myshm_mmap"
#define HOOK_DNAME "myshm"
#else
#error "Unknown HOOK_MODE, must be UIO or MYSHM"
#endif

#define ERR_INVALID_PTR "Invalid Pointer."
#define ERR_INVALID_RET "Invalild return value."
#define ERR_INVALID_FD "Invalid File Descripter."
#define ERR_INVALID_PARAMS "Invalid Parameters."
#define ERR_MEMORY_SHORTAGE "Failed to allocate the memory."

#define DEBUG

#ifdef DEBUG
#define DBG_MSG(fmt, ...)                                                      \
        do {                                                                   \
                pr_info("[IVSHMEM] " fmt " / func : %s, line : %d\n",          \
                        ##__VA_ARGS__, __func__, __LINE__);                    \
        } while (0)
#else
#define DBG_MSG(fmt, ...)                                                      \
        do {                                                                   \
        } while (0)
#endif

#define ERR_MSG(fmt, ...)                                                      \
        do {                                                                   \
                pr_err("[IVSHMEM ERR] " fmt " / func : %s, line : %d\n",       \
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
        SET = 0,
        RESET,
};

enum ivshmem_section_e {
        MIN_VALID_OFFSET = 0x1000,
        MAX_ALLOWED_MMAP_SIZE = 0x200000, /* 2MB */
};

struct ivshmem_lock {
        volatile bool is_locked;
        spinlock_t *spin;
};

struct ivshmem_context {
        unsigned long target_addr;
        unsigned long limit_map_size;

        struct ftrace_ops *ops;
        struct ivshmem_lock *lock;
};

struct ivshmem_mmap_data {
        struct pt_regs *pt_regs;
        unsigned long target_addr;
};

static struct ivshmem_context *__ivshmem_ctx_create(void);
static struct ftrace_ops *__ivshmem_ops_create(void);
static struct ivshmem_lock *__ivshmem_lock_create(void);

static void notrace __ivshmem_ftrace_hook_func(unsigned long ip,
                                               unsigned long parent_ip,
                                               struct ftrace_ops *ops,
                                               struct ftrace_regs *regs);
static int __ivshmem_origin_mmap(struct file *file, struct vm_area_struct *vma);
static int __ivshmem_dummy_mmap(struct file *file, struct vm_area_struct *vma);
static int __ivshmem_pre_handler(struct kprobe *kp, struct pt_regs *regs);

static inline int __ivshmem_lock_destroy(void *del);
static inline int __ivshmem_ops_destroy(void *del);
static inline int __ivshmem_ctx_destroy(void *del);
static inline int __ivshmem_uio_hook_exit(struct ivshmem_context *ctx);
static inline int __ivshmem_mmap_guard(bool condition, spinlock_t *lock,
                                       void *private);

static struct kprobe kp = {
    .symbol_name = HOOK_MMAP,
    .pre_handler = __ivshmem_pre_handler,
};

static struct ivshmem_context *g_ctx = NULL;

static int __init ivshmem_uio_hook_init(void) {
        FORMULA_GUARD(g_ctx != NULL, -EINVAL, ERR_INVALID_PTR);

        struct ivshmem_context *ctx = __ivshmem_ctx_create();
        FORMULA_GUARD(ctx == NULL, -ENOMEM, "");

        struct ftrace_ops *ops = __ivshmem_ops_create();
        FORMULA_GUARD_WITH_EXIT_FUNC(ops == NULL, -ENOMEM,
                                     __ivshmem_uio_hook_exit(ctx), "");

        struct ivshmem_lock *lock = __ivshmem_lock_create();
        FORMULA_GUARD_WITH_EXIT_FUNC(lock == NULL, -ENOMEM,
                                     __ivshmem_uio_hook_exit(ctx), "");

        /* init the chaining */
        ctx->ops = ops;
        ctx->lock = lock;
        ops->private = ctx;

        /* set the ftrace */
        int ret =
            ftrace_set_filter_ip(ctx->ops, ctx->target_addr, REGISTER, SET);
        FORMULA_GUARD_WITH_EXIT_FUNC(ret < 0, ret, __ivshmem_uio_hook_exit(ctx),
                                     ERR_INVALID_RET);
        DBG_MSG("ftrace_set_filter_ip() returned %d", ret);

        ret = register_ftrace_function(ctx->ops);
        FORMULA_GUARD_WITH_EXIT_FUNC(ret < 0, ret, __ivshmem_uio_hook_exit(ctx),
                                     ERR_INVALID_RET);

        /* for module_exit */
        g_ctx = ctx;

        DBG_MSG("Success to init the ivshmem uio hook.");

        return 0;
}

static void __exit ivshmem_uio_hook_exit(void) {
        __ivshmem_uio_hook_exit(g_ctx);
}

static int __ivshmem_pre_handler(struct kprobe *kp, struct pt_regs *regs) {
        DBG_MSG("[IVSHMEM]>>> kprobe hit shm_mmap\n");
        printk("test");
        return 0;
}

static struct ivshmem_context *__ivshmem_ctx_create(void) {
        struct ivshmem_context *new =
            kmalloc(sizeof(struct ivshmem_context), GFP_KERNEL);
        FORMULA_GUARD(new == NULL, NULL, ERR_MEMORY_SHORTAGE);

        int ret = register_kprobe(&kp);

        FORMULA_GUARD_WITH_EXIT_FUNC(ret < 0, NULL, __ivshmem_ctx_destroy(new),
                                     ERR_INVALID_RET);

        new->target_addr = (unsigned long)kp.addr;

        DBG_MSG("Target Addr: 0x%016lx", new->target_addr);

        FORMULA_GUARD_WITH_EXIT_FUNC(new->target_addr == 0, NULL,
                                     __ivshmem_ctx_destroy(new),
                                     ERR_INVALID_RET);

        /* TODO: set limit size from control section in ivshmem */
        new->limit_map_size = 0x1000; /* 4KB */

        DBG_MSG("Success to create the ctx.");

        return new;
}

static struct ftrace_ops *__ivshmem_ops_create(void) {
        struct ftrace_ops *new = kmalloc(sizeof(struct ftrace_ops), GFP_KERNEL);
        FORMULA_GUARD(new == NULL, NULL, ERR_MEMORY_SHORTAGE);

        memset(new, 0, sizeof(struct ftrace_ops));
        new->func = __ivshmem_ftrace_hook_func;
        new->flags = FTRACE_OPS_FL_SAVE_REGS | FTRACE_OPS_FL_RECURSION |
                     FTRACE_OPS_FL_IPMODIFY;

        DBG_MSG("Success to create the ops.");

        return new;
}

static struct ivshmem_lock *__ivshmem_lock_create(void) {
        struct ivshmem_lock *new =
            kmalloc(sizeof(struct ivshmem_lock), GFP_KERNEL);
        FORMULA_GUARD(new == NULL, NULL, ERR_MEMORY_SHORTAGE);

        new->is_locked = false;
        new->spin = kmalloc(sizeof(spinlock_t), GFP_KERNEL);
        FORMULA_GUARD_WITH_EXIT_FUNC(new->spin == NULL, NULL,
                                     __ivshmem_lock_destroy(new),
                                     ERR_MEMORY_SHORTAGE);

        spin_lock_init(new->spin);

        DBG_MSG("Success to create the lock.");

        return new;
}

static void notrace __ivshmem_ftrace_hook_func(unsigned long ip,
                                               unsigned long parent_ip,
                                               struct ftrace_ops *ops,
                                               struct ftrace_regs *regs) {
        DBG_MSG("ftrace_hook_func: 0x%016lx, parent_ip: 0x%016lx, ops: %p, "
                "regs: %p",
                ip, parent_ip, ops, regs);
        FORMULA_GUARD(ops == NULL || regs == NULL, , ERR_INVALID_PTR);

        struct ivshmem_context *ctx = (struct ivshmem_context *)ops->private;
        FORMULA_GUARD(ctx == NULL || ctx->lock == NULL, , ERR_INVALID_PTR);

        struct ivshmem_lock *lock = ctx->lock;
        if (lock->is_locked == true) {
                DBG_MSG("Already locked in hook func");
                return;
        }

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

        spin_lock(lock->spin);
        lock->is_locked = true;

        DBG_MSG("Hooked mmap: 0x%016lx, file: %p, vma: %p", ip, file, vma);

        if (file->f_path.dentry) {
                const char *dname = file->f_path.dentry->d_name.name;

                /* only check from the /dev/uioX */
                if (strncmp(dname, HOOK_DNAME, strlen(HOOK_DNAME)) == 0) {
                        DBG_MSG("Hooked mmap: %s", dname);
                        struct ivshmem_mmap_data mmap_data = {
                            .pt_regs = pt_regs,
                            .target_addr = (unsigned long)__ivshmem_dummy_mmap,
                        };

                        int ret;
                        off_t offset = vma->vm_pgoff << PAGE_SHIFT;
                        size_t length = vma->vm_end - vma->vm_start;
                        DBG_MSG("offset: 0x%016lx, length: 0x%016lx", offset,
                                length);

                        /* control section */
                        ret = __ivshmem_mmap_guard(offset < MIN_VALID_OFFSET &&
                                                       offset + length >
                                                           MIN_VALID_OFFSET,
                                                   lock->spin, &mmap_data);
                        FORMULA_GUARD(ret < 0, , "Invalid length value");

                        /* data section */
                        ret = __ivshmem_mmap_guard(
                            offset >= MIN_VALID_OFFSET &&
                                offset + length > MAX_ALLOWED_MMAP_SIZE,
                            lock->spin, &mmap_data);
                        FORMULA_GUARD(ret < 0, ,
                                      "Mapping size exceeds allowed limit!");

                        /* overflow from the configuration setting value */
                        ret = __ivshmem_mmap_guard(length > ctx->limit_map_size,
                                                   lock->spin, &mmap_data);
                        FORMULA_GUARD(ret < 0, , "Overflow the length value.");
                }
        }

        /* origin mmap */
        instruction_pointer_set(pt_regs, (unsigned long)__ivshmem_origin_mmap);
        spin_unlock(lock->spin);
}

static int __ivshmem_origin_mmap(struct file *file,
                                 struct vm_area_struct *vma) {
        struct ivshmem_context *ctx = g_ctx;
        struct ivshmem_lock *lock = ctx->lock;

        int (*origin_mmap)(struct file *, struct vm_area_struct *) =
            (int (*)(struct file *, struct vm_area_struct *))ctx->target_addr;

        origin_mmap(file, vma);

        spin_lock(lock->spin);
        lock->is_locked = false;
        spin_unlock(lock->spin);

        FORMULA_GUARD(1, 0, "Call the origin mmap!");
}

static int __ivshmem_dummy_mmap(struct file *file, struct vm_area_struct *vma) {
        struct ivshmem_lock *lock = g_ctx->lock;
        spin_lock(lock->spin);
        lock->is_locked = false;
        spin_unlock(lock->spin);

        FORMULA_GUARD(1, -EINVAL, "Failed to call the uio_mmap,,");
}

static inline int __ivshmem_lock_destroy(void *del) {
        FORMULA_GUARD(del == NULL, -EINVAL, ERR_INVALID_PTR);

        PTR_FREE(((struct ivshmem_lock *)del)->spin);
        PTR_FREE(del);

        return 0;
}

static inline int __ivshmem_ops_destroy(void *del) {
        FORMULA_GUARD(del == NULL, -EINVAL, ERR_INVALID_PTR);

        PTR_FREE(del);

        return 0;
}

static inline int __ivshmem_ctx_destroy(void *del) {
        FORMULA_GUARD(del == NULL, -EINVAL, ERR_INVALID_PTR);

        PTR_FREE(del);

        return 0;
}

static inline int __ivshmem_uio_hook_exit(struct ivshmem_context *ctx) {
        FORMULA_GUARD(ctx == NULL, -EINVAL, ERR_INVALID_PTR);

        unregister_kprobe(&kp);
        pr_info("IVSHMEM: kprobe unregistered\n");

        unregister_ftrace_function(ctx->ops);
        ftrace_set_filter_ip(ctx->ops, ctx->target_addr, REMOVE, SET);

        __ivshmem_lock_destroy(ctx->lock);
        __ivshmem_ops_destroy(ctx->ops);
        __ivshmem_ctx_destroy(ctx);

        DBG_MSG("Success to exit the ivshmem uio hook.");

        return 0;
}

static inline int __ivshmem_mmap_guard(bool condition, spinlock_t *lock,
                                       void *private) {
        if (unlikely(condition)) {
                if (private != NULL) {
                        struct ivshmem_mmap_data *mmap_data =
                            (struct ivshmem_mmap_data *)private;
                        instruction_pointer_set(mmap_data->pt_regs,
                                                mmap_data->target_addr);
                }

                spin_unlock(lock);
                return -EINVAL;
        }

        return 0;
}

module_init(ivshmem_uio_hook_init);
module_exit(ivshmem_uio_hook_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lee, Jerry J");
MODULE_DESCRIPTION("PCI UIO generic driver hooking module for the ivshmem.");
