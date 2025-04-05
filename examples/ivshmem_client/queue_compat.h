#ifndef QUEUE_COMPAT_H
#define QUEUE_COMPAT_H

#include <sys/queue.h>

/* Map QEMU tail-queue macros to BSD's sys/queue.h names */
#define QTAILQ_HEAD(name, type) TAILQ_HEAD(name, type)
#define QTAILQ_ENTRY(type) TAILQ_ENTRY(type)
#define QTAILQ_INIT(head) TAILQ_INIT(head)
#define QTAILQ_INSERT_TAIL(head, elm, field) TAILQ_INSERT_TAIL(head, elm, field)
#define QTAILQ_REMOVE(head, elm, field) TAILQ_REMOVE(head, elm, field)
#define QTAILQ_FIRST(head) TAILQ_FIRST(head)
#define QTAILQ_FOREACH(var, head, field) TAILQ_FOREACH(var, head, field)

#define PATH_MAX 4096 /* # chars in a path name including nul */

#endif /* QUEUE_COMPAT_H */
