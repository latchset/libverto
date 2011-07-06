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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>

#include <verto-module.h>

typedef char bool;
#define true ((bool) 1)
#define false ((bool) 0)

#define  _str(s) # s
#define __str(s) _str(s)
#define vnew(type) ((type*) malloc(sizeof(type)))
#define vnew0(type) ((type*) memset(vnew(type), 0, sizeof(type)))

struct _verto_ev_ctx {
    void *dll;
    void *modpriv;
    verto_ev_ctx_funcs funcs;
    verto_ev *events;
};

typedef struct {
    pid_t pid;
    int   status;
} verto_child;

struct _verto_ev {
    verto_ev *next;
    verto_ev_ctx *ctx;
    verto_ev_type type;
    verto_callback *callback;
    void *priv;
    void *modpriv;
    verto_ev_flag flags;
    bool persists;
    size_t depth;
    bool deleted;
    union {
        int fd;
        int signal;
        time_t interval;
        verto_child child;
    } option;
};

const verto_module *defmodule;

static bool
do_load_file(const char *filename, bool reqsym, void **dll,
             const verto_module **module)
{
    *dll = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (!dll) {
        /* printf("%s -- %s\n", filename, dlerror()); */
        return false;
    }

    *module = (verto_module*) dlsym(*dll, __str(VERTO_MODULE_TABLE));
    if (!*module || (*module)->vers != VERTO_MODULE_VERSION
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

static bool
do_load_dir(const char *dirname, const char *prefix, const char *suffix,
            bool reqsym, void **dll, const verto_module **module)
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
load_module(const char *impl, void **dll, const verto_module **module)
{
    bool success = false;
    Dl_info dlinfo;
    char *prefix = NULL;
    char *suffix = NULL;
    char *tmp = NULL;

    if (!dladdr(verto_convert_funcs, &dlinfo))
        return false;

    suffix = strstr(dlinfo.dli_fname, MODSUFFIX);
    if (!suffix)
        return false;

    prefix = strndup(dlinfo.dli_fname, suffix - dlinfo.dli_fname + 1);
    if (!prefix)
        return false;
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
        if (!(success = (*module = defmodule) != NULL)) {
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
    return success;
}

static verto_ev *
make_ev(verto_ev_ctx *ctx, verto_callback *callback, void *priv,
        verto_ev_type type, verto_ev_flag flags)
{
    verto_ev *ev = NULL;

    if (!ctx || !callback)
        return NULL;

    ev = malloc(sizeof(verto_ev));
    if (ev) {
        memset(ev, 0, sizeof(verto_ev));
        ev->ctx        = ctx;
        ev->type       = type;
        ev->callback   = callback;
        ev->priv       = priv;
        ev->flags      = flags;
    }

    return ev;
}

static void
push_ev(verto_ev_ctx *ctx, verto_ev *ev)
{
    if (!ctx || !ev)
        return;

    verto_ev *tmp = ctx->events;
    ctx->events = ev;
    ctx->events->next = tmp;
}

static void
remove_ev(verto_ev **origin, verto_ev *item)
{
    if (!origin || !*origin || !item)
        return;

    if (*origin == item)
        *origin = (*origin)->next;
    else
        remove_ev(&((*origin)->next), item);
}

static void
signal_ignore(verto_ev_ctx *ctx, verto_ev *ev)
{
}

verto_ev_ctx *
verto_new(const char *impl)
{
    void *dll = NULL;
    const verto_module *module = NULL;
    verto_ev_ctx *ctx = NULL;

    if (!load_module(impl, &dll, &module))
        return NULL;

    ctx = module->new_ctx();
    if (!ctx && dll)
        dlclose(dll);
    if (ctx && defmodule != module)
        ctx->dll = dll;

    return ctx;
}

verto_ev_ctx *
verto_default(const char *impl)
{
    void *dll = NULL;
    const verto_module *module = NULL;
    verto_ev_ctx *ctx = NULL;

    if (!load_module(impl, &dll, &module))
        return NULL;

    ctx = module->def_ctx();
    if (!ctx && dll)
        dlclose(dll);
    if (ctx && defmodule != module)
        ctx->dll = dll;

    return ctx;
}

int
verto_set_default(const char *impl)
{
    void *dll = NULL; /* we will leak the dll */
    return (!defmodule && impl && load_module(impl, &dll, &defmodule));
}

void
verto_free(verto_ev_ctx *ctx)
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
        for (i=1 ; i < _NSIG ; i++) {
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
verto_run(verto_ev_ctx *ctx)
{
    if (!ctx)
        return;
    ctx->funcs.ctx_run(ctx->modpriv);
}

void
verto_run_once(verto_ev_ctx *ctx)
{
    if (!ctx)
        return;
    ctx->funcs.ctx_run_once(ctx->modpriv);
}

void
verto_break(verto_ev_ctx *ctx)
{
    if (!ctx)
        return;
    ctx->funcs.ctx_break(ctx->modpriv);
}

#define doadd(set, type) \
    verto_ev *ev = make_ev(ctx, callback, priv, type, flags); \
    if (ev) { \
        set; \
        ev->modpriv = ctx->funcs.ctx_add(ctx->modpriv, ev, &ev->persists); \
        if (!ev->modpriv) { \
            free(ev); \
            return NULL; \
        } \
        push_ev(ctx, ev); \
    } \
    return ev;

verto_ev *
verto_add_io(verto_ev_ctx *ctx, verto_ev_flag flags,
             verto_callback *callback, void *priv, int fd)
{
    if (fd < 0 || !(flags & (VERTO_EV_FLAG_IO_READ | VERTO_EV_FLAG_IO_WRITE)))
        return NULL;
    doadd(ev->option.fd = fd, VERTO_EV_TYPE_IO);
}

verto_ev *
verto_add_timeout(verto_ev_ctx *ctx, verto_ev_flag flags,
                  verto_callback *callback, void *priv, time_t interval)
{
    doadd(ev->option.interval = interval, VERTO_EV_TYPE_TIMEOUT);
}

verto_ev *
verto_add_idle(verto_ev_ctx *ctx, verto_ev_flag flags,
               verto_callback *callback, void *priv)
{
    doadd(, VERTO_EV_TYPE_IDLE);
}

verto_ev *
verto_add_signal(verto_ev_ctx *ctx, verto_ev_flag flags,
                 verto_callback *callback, void *priv, int signal)
{
    if (signal < 0 || signal == SIGCHLD)
        return NULL;
    if (callback == VERTO_SIG_IGN)
        callback = signal_ignore;
    doadd(ev->option.signal = signal, VERTO_EV_TYPE_SIGNAL);
}

verto_ev *
verto_add_child(verto_ev_ctx *ctx, verto_ev_flag flags,
                verto_callback *callback, void *priv, pid_t pid)
{
    if (pid < 1 || (flags & VERTO_EV_FLAG_PERSIST)) /* persist makes no sense */
        return NULL;
    doadd(ev->option.child.pid = pid, VERTO_EV_TYPE_CHILD);
}

void *
verto_get_private(const verto_ev *ev)
{
    return ev->priv;
}

verto_ev_type
verto_get_type(const verto_ev *ev)
{
    return ev->type;
}

verto_ev_flag
verto_get_flags(const verto_ev *ev)
{
    return ev->flags;
}

int
verto_get_fd(const verto_ev *ev)
{
    if (ev && (ev->type == VERTO_EV_TYPE_IO))
        return ev->option.fd;
    return -1;
}

time_t
verto_get_interval(const verto_ev *ev)
{
    if (ev && (ev->type == VERTO_EV_TYPE_TIMEOUT))
        return ev->option.interval;
    return 0;
}

int
verto_get_signal(const verto_ev *ev)
{
    if (ev && (ev->type == VERTO_EV_TYPE_SIGNAL))
        return ev->option.signal;
    return -1;
}

pid_t
verto_get_pid(const verto_ev *ev) {
    if (ev && ev->type == VERTO_EV_TYPE_CHILD)
        return ev->option.child.pid;
    return 0;
}

int
verto_get_pid_status(const verto_ev *ev)
{
    return ev->option.child.status;
}

void
verto_del(verto_ev *ev)
{
    if (!ev)
        return;

    /* If the event is freed in the callback, we just set a flag so that
     * verto_fire() can actually do the delete when the callback completes.
     *
     * If we don't do this, than verto_fire() will access freed memory. */
    if (ev->depth > 0) {
        ev->deleted = true;
        return;
    }

    ev->ctx->funcs.ctx_del(ev->ctx->modpriv, ev, ev->modpriv);
    remove_ev(&(ev->ctx->events), ev);
    free(ev);
}

/*** THE FOLLOWING ARE FOR IMPLEMENTATION MODULES ONLY ***/

verto_ev_ctx *
verto_convert_funcs(const verto_ev_ctx_funcs *funcs,
               const verto_module *module,
               void *ctx_private)
{
    verto_ev_ctx *ctx = NULL;

    if (!funcs || !module || !ctx_private)
        return NULL;

    ctx = vnew0(verto_ev_ctx);
    if (!ctx)
        return NULL;

    ctx->modpriv = ctx_private;
    ctx->funcs = *funcs;

    if (!defmodule)
        defmodule = module;

    return ctx;
}

void
verto_fire(verto_ev *ev)
{
    void *priv;

    ev->depth++;
    ev->callback(ev->ctx, ev);
    ev->depth--;

    if (ev->depth == 0) {
        if (!(ev->flags & VERTO_EV_FLAG_PERSIST) || ev->deleted)
            verto_del(ev);
        else if (!ev->persists) {
            priv = ev->ctx->funcs.ctx_add(ev->ctx->modpriv, ev, &ev->persists);
            assert(priv); /* TODO: create an error callback */
            ev->ctx->funcs.ctx_del(ev->ctx->modpriv, ev, ev->modpriv);
            ev->modpriv = priv;
        }
    }
}

void
verto_set_pid_status(verto_ev *ev, int status)
{
    if (ev && ev->type == VERTO_EV_TYPE_CHILD)
        ev->option.child.status = status;
}
