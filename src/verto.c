/*
 * Copyright 2011 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _GNU_SOURCE /* For dladdr(), asprintf() */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>

#include <verto-module.h>

#define  _str(s) # s
#define __str(s) _str(s)
#define vnew(type) ((type*) malloc(sizeof(type)))
#define vnew0(type) ((type*) memset(vnew(type), 0, sizeof(type)))

struct vertoEvCtx {
    void *dll;
    void *modpriv;
    struct vertoEvCtxFuncs funcs;
    struct vertoEv *events;
};

struct vertoChild {
    pid_t pid;
    int   status;
};

struct vertoEv {
    struct vertoEv *next;
    struct vertoEvCtx *ctx;
    enum vertoEvType type;
    enum vertoEvPriority priority;
    vertoCallback callback;
    void *priv;
    void *modpriv;
    union {
        int fd;
        int signal;
        time_t interval;
        struct vertoChild child;
    } option;
};

static struct {
    void *dll;
    struct vertoModule *module;
} defimpl;

static inline bool
do_load_file(const char *filename, bool reqsym, void **dll,
             struct vertoModule **module)
{
    *dll = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (!dll) {
        /* printf("%s -- %s\n", filename, dlerror()); */
        return false;
    }

    *module = (struct vertoModule*) dlsym(*dll, __str(_VERTO_MODULE_TABLE));
    if (!*module || (*module)->vers != _VERTO_MODULE_VERSION
            || !(*module)->new_ctx || !(*module)->def_ctx)
        goto error;

    /* Check to make sure that we have our required symbol if reqsym == true */
    if ((*module)->symb && reqsym) {
        void *tmp = dlopen(NULL, 0);
        if (!tmp || !dlsym(tmp, (*module)->symb)) {
            if (tmp)
                dlclose(tmp);
            goto error;
        }
        dlclose(tmp);
    }

    return true;

    error:
        dlclose(dll);
        return false;
}

static inline bool
do_load_dir(const char *dirname, const char *prefix, const char *suffix,
            bool reqsym, void **dll, struct vertoModule **module)
{
    *module = NULL;
    DIR *dir = opendir(dirname);
    if (!dir)
        return false;

    struct dirent *ent = NULL;
    while ((ent = readdir(dir))) {
        size_t flen = strlen(ent->d_name);
        size_t slen = strlen(suffix);

        if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name))
            continue;
        if (strstr(ent->d_name, prefix) != ent->d_name)
            continue;
        if (flen < slen || strcmp(ent->d_name + flen - slen, suffix))
            continue;

        char *tmp = NULL;
        if (asprintf(&tmp, "%s/%s", dirname, ent->d_name) < 0)
            continue;

        bool success = do_load_file(tmp, reqsym, dll, module);
        free(tmp);
        if (success)
            break;
        *module = NULL;
    }

    closedir(dir);
    return module != NULL;
}

static bool
load_module(const char *impl, void **dll, struct vertoModule **module)
{
    bool success = false;
    Dl_info dlinfo;
    char *prefix = NULL;
    char *suffix = NULL;
    char *tmp = NULL;

    if (!dladdr(verto_convert_funcs, &dlinfo))
        return NULL;

    suffix = strstr(dlinfo.dli_fname, MODSUFFIX);
    if (!suffix)
        return NULL;

    prefix = strndup(dlinfo.dli_fname, suffix - dlinfo.dli_fname + 1);
    if (!prefix)
        return NULL;
    prefix[strlen(prefix) - 1] = '-'; /* Ex: /usr/lib/libverto- */

    if (impl) {
        /* Try to do a load by the path */
        if (strchr(impl, '/'))
            success = do_load_file(impl, false, dll, module);
        if (!success) {
            /* Try to do a load by the name */
            tmp = NULL;
            if (asprintf(&tmp, "%s%s%s", prefix, impl, suffix) > 0) {
                success = do_load_file(tmp, false, dll, module);
                free(tmp);
            }
        }
    } else {
        /* First, try the default implementation (aka 'the cache')*/
        *dll = NULL;
        *module = defimpl.module;
        if (!(success = *module != NULL)) {
            /* NULL was passed, so we will use the dirname of
             * the prefix to try and find any possible plugins */
            tmp = strdup(prefix);
            if (tmp) {
                char *dname = strdup(dirname(tmp));
                free(tmp);

                tmp = strdup(basename(prefix));
                free(prefix);
                prefix = tmp;

                if (dname && prefix) {
                    /* Attempt to find a module we are already linked to */
                    success = do_load_dir(dname, prefix, suffix, true, dll,
                                          module);
                    if (!success) {
#ifdef DEFAULT_LIBRARY
                        /* Attempt to find the default module */
                        success = load_module(DEFAULT_LIBRARY, dll, module);
                        if (!success)
#endif /* DEFAULT_LIBRARY */
                        /* Attempt to load any plugin (we're desperate) */
                        success = do_load_dir(dname, prefix, suffix, false, dll,
                                              module);
                    }
                }

                free(dname);
            }
        }
    }

    free(prefix);

    if (success && !defimpl.module) {
        defimpl.dll = *dll;
        defimpl.module = *module;
        *dll = NULL; // Don't free this module (aka leak the module pointer)
    }

    return success;
}

static inline struct vertoEv *
make_ev(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
        vertoCallback callback, void *priv, enum vertoEvType type)
{
    struct vertoEv *ev = NULL;

    if (!ctx || !callback)
        return NULL;

    priority = priority < _VERTO_EV_PRIORITY_MAX + 1
                   ? priority
                   : VERTO_EV_PRIORITY_DEFAULT;

    ev = malloc(sizeof(struct vertoEv));
    if (ev) {
        ev->next       = NULL;
        ev->ctx        = ctx;
        ev->type       = type;
        ev->priority   = priority;
        ev->callback   = callback;
        ev->priv       = priv;
    }

    return ev;
}

static inline void
push_ev(struct vertoEvCtx *ctx, struct vertoEv *ev)
{
    if (!ctx || !ev)
        return;

    struct vertoEv *tmp = ctx->events;
    ctx->events = ev;
    ctx->events->next = tmp;
}

static void
remove_ev(struct vertoEv **origin, struct vertoEv *item)
{
    if (!origin || !*origin || !item)
        return;

    if (*origin == item)
        *origin = (*origin)->next;
    else
        remove_ev(&((*origin)->next), item);
}

struct vertoEvCtx *
verto_new(const char *impl)
{
    void *dll = NULL;
    struct vertoModule *module = NULL;
    struct vertoEvCtx *ctx = NULL;

    if (!load_module(impl, &dll, &module))
        return NULL;

    ctx = module->new_ctx();
    if (ctx)
        ctx->dll = dll;
    else if (dll)
        dlclose(dll);

    return ctx;
}

struct vertoEvCtx *
verto_default(const char *impl)
{
    void *dll = NULL;
    struct vertoModule *module = NULL;
    struct vertoEvCtx *ctx = NULL;

    if (!load_module(impl, &dll, &module))
        return NULL;

    ctx = module->def_ctx();
    if (ctx)
        ctx->dll = dll;
    else if (dll)
        dlclose(dll);

    return ctx;
}

void
verto_free(struct vertoEvCtx *ctx)
{
    int i;
    sigset_t old;
    sigset_t block;
    struct sigaction act;
    Dl_info info;

    if (!ctx)
        return;

    /* Cancel all pending events */
    while (ctx->events)
        verto_del(ctx->events);

    /* Free the private */
    ctx->funcs.ctx_free(ctx->modpriv);

    /* Unload the module */
    if (ctx->dll) {
        /* If dlclose() unmaps memory that is registered as a signal handler
         * we have to remove that handler otherwise if that signal is fired
         * we jump into unmapped memory. So we loop through and test each
         * handler to see if it is in unmapped memory.  If it is, we set it
         * back to the default handler. Lastly, if a signal were to fire it
         * could be a race condition. So we mask out all signals during this
         * process.
         */
        sigfillset(&block);
        sigprocmask(SIG_SETMASK, &block, &old);
        dlclose(ctx->dll);
        for (i=0 ; i < _NSIG ; i++) {
            if (sigaction(i, NULL, &act) == 0) {
                if (act.sa_flags & SA_SIGINFO) {
                    if (dladdr(act.sa_sigaction, &info) == 0)
                        signal(i, SIG_DFL);
                } else if (act.sa_handler != SIG_DFL
                        && act.sa_handler != SIG_IGN
                        && dladdr(act.sa_handler, &info) == 0) {
                    signal(i, SIG_DFL);
                }
            }
        }
        sigprocmask(SIG_SETMASK, &old, NULL);
    }

    free(ctx);
}

void
verto_run(struct vertoEvCtx *ctx)
{
    if (!ctx)
        return;
    ctx->funcs.ctx_run(ctx->modpriv);
}

void
verto_run_once(struct vertoEvCtx *ctx)
{
    if (!ctx)
        return;
    ctx->funcs.ctx_run_once(ctx->modpriv);
}

void
verto_break(struct vertoEvCtx *ctx)
{
    if (!ctx)
        return;
    ctx->funcs.ctx_break(ctx->modpriv);
}

#define doadd(set, type) \
    struct vertoEv *ev = make_ev(ctx, priority, callback, priv, type); \
    if (ev) { \
        set; \
        ev->modpriv = ctx->funcs.ctx_add(ctx->modpriv, ev); \
        if (!ev->modpriv) { \
            free(ev); \
            return NULL; \
        } \
        push_ev(ctx, ev); \
    } \
    return ev;

struct vertoEv *
verto_add_read(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
               vertoCallback callback, void *priv, int fd)
{
    if (fd < 0)
        return NULL;
    doadd(ev->option.fd = fd, VERTO_EV_TYPE_READ);
}


struct vertoEv *
verto_add_write(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                vertoCallback callback, void *priv, int fd)
{
    if (fd < 0)
        return NULL;
    doadd(ev->option.fd = fd, VERTO_EV_TYPE_WRITE);
}

struct vertoEv *
verto_add_timeout(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                  vertoCallback callback, void *priv, time_t interval)
{
    doadd(ev->option.interval = interval, VERTO_EV_TYPE_TIMEOUT);
}

struct vertoEv *
verto_add_idle(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
               vertoCallback callback, void *priv)
{
    doadd(, VERTO_EV_TYPE_IDLE);
}

struct vertoEv *
verto_add_signal(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                 vertoCallback callback, void *priv, int signal)
{
    if (signal < 0)
        return NULL;
    doadd(ev->option.signal = signal, VERTO_EV_TYPE_SIGNAL);
}

struct vertoEv *
verto_add_child(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                vertoCallback callback, void *priv, pid_t pid)
{
    if (pid < 1)
        return NULL;
    doadd(ev->option.child.pid = pid, VERTO_EV_TYPE_CHILD);
}

struct vertoEv *
verto_repeat(const struct vertoEv *ev)
{
    switch (ev->type) {
    case VERTO_EV_TYPE_READ:
        return verto_add_read(ev->ctx, ev->priority, ev->callback, ev->priv,
                              ev->option.fd);
    case VERTO_EV_TYPE_WRITE:
        return verto_add_write(ev->ctx, ev->priority, ev->callback, ev->priv,
                               ev->option.fd);
    case VERTO_EV_TYPE_TIMEOUT:
        return verto_add_timeout(ev->ctx, ev->priority, ev->callback, ev->priv,
                                 ev->option.interval);
    case VERTO_EV_TYPE_IDLE:
        return verto_add_idle(ev->ctx, ev->priority, ev->callback, ev->priv);
    case VERTO_EV_TYPE_CHILD:
        return verto_add_child(ev->ctx, ev->priority, ev->callback, ev->priv,
                               ev->option.child.pid);
    case VERTO_EV_TYPE_SIGNAL:
    default:
        break; /* Not supported */
    }

    return NULL;
}

void *
verto_get_private(const struct vertoEv *ev)
{
    return ev->priv;
}

enum vertoEvType
verto_get_type(const struct vertoEv *ev)
{
    return ev->type;
}

enum vertoEvPriority
verto_get_priority(const struct vertoEv *ev)
{
    return ev->priority;
}

int
verto_get_fd(const struct vertoEv *ev)
{
    if (ev && (ev->type & (VERTO_EV_TYPE_READ | VERTO_EV_TYPE_WRITE)))
        return ev->option.fd;
    return -1;
}

time_t
verto_get_interval(const struct vertoEv *ev)
{
    if (ev && (ev->type & VERTO_EV_TYPE_TIMEOUT))
        return ev->option.interval;
    return 0;
}

int
verto_get_signal(const struct vertoEv *ev)
{
    if (ev && (ev->type & VERTO_EV_TYPE_SIGNAL))
        return ev->option.signal;
    return -1;
}

pid_t
verto_get_pid(const struct vertoEv *ev) {
    if (ev && ev->type == VERTO_EV_TYPE_CHILD)
        return ev->option.child.pid;
    return 0;
}

int
verto_get_pid_status(const struct vertoEv *ev)
{
    return ev->option.child.status;
}

void
verto_del(struct vertoEv *ev)
{
    if (!ev)
        return;
    ev->ctx->funcs.ctx_del(ev->ctx->modpriv, ev, ev->modpriv);
    remove_ev(&(ev->ctx->events), ev);
    free(ev);
}

/*** THE FOLLOWING ARE FOR IMPLEMENTATION MODULES ONLY ***/

struct vertoEvCtx *
verto_convert_funcs(const struct vertoEvCtxFuncs *funcs, void *ctx_private)
{
    struct vertoEvCtx *ctx = NULL;

    if (!funcs || !ctx_private)
        return NULL;

    ctx = vnew0(struct vertoEvCtx);
    if (!ctx)
        return NULL;

    ctx->modpriv = ctx_private;
    ctx->funcs = *funcs;
    return ctx;
}

void
verto_fire(struct vertoEv *ev)
{
    ev->callback(ev->ctx, ev);
    if (ev->type != VERTO_EV_TYPE_SIGNAL)
        verto_del(ev);
}

void
verto_set_pid_status(struct vertoEv *ev, int status)
{
    if (ev && ev->type == VERTO_EV_TYPE_CHILD)
        ev->option.child.status = status;
}
