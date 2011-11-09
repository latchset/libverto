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
#include <stdarg.h>

#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <verto-module.h>

#ifdef WIN32
#define pdlmtype HMODULE
#define pdlopenl(filename) LoadLibraryEx(filename, NULL, \
                                         DONT_RESOLVE_DLL_REFERENCES)
#define pdlclose(module) FreeLibrary((pdlmtype) module)
#define pdlsym(mod, sym) ((void *) GetProcAddress(mod, sym))

static pdlmtype
pdlreopen(const char *filename, pdlmtype module)
{
    pdlclose(module);
    return LoadLibrary(filename);
}

static char *
pdlerror(void) {
    char *amsg;
    LPTSTR msg;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &msg, 0, NULL);
    amsg = strdup((const char*) msg);
    LocalFree(msg);
    return amsg;
}

static bool
pdlsymlinked(const char *modn, const char *symb) {
    return (GetProcAddress(GetModuleHandle(modn), symb) != NULL ||
            GetProcAddress(GetModuleHandle(NULL), symb) != NULL);
}

static bool
pdladdrmodname(void *addr, char **buf) {
    MEMORY_BASIC_INFORMATION info;
    HMODULE mod;
    char modname[MAX_PATH];

    if (!VirtualQuery(addr, &info, sizeof(info)))
        return false;
    mod = (HMODULE) info.AllocationBase;

    if (!GetModuleFileNameA(mod, modname, MAX_PATH))
        return false;

    if (buf) {
        *buf = strdup(modname);
        if (!buf)
            return false;
    }

    return true;
}
#else
#define pdlmtype void *
#define pdlopenl(filename) dlopen(filename, RTLD_LAZY | RTLD_LOCAL)
#define pdlclose(module) dlclose((pdlmtype) module)
#define pdlreopen(filename, module) module
#define pdlsym(mod, sym) dlsym(mod, sym)
#define pdlerror() strdup(dlerror())

static int
pdlsymlinked(const char* modn, const char* symb)
{
    void* mod = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL);
    if (mod) {
        void* sym = dlsym(mod, symb);
        dlclose(mod);
        return sym != NULL;
    }
    return 0;
}

static int
pdladdrmodname(void *addr, char **buf) {
    Dl_info dlinfo;
    if (!dladdr(addr, &dlinfo))
        return 0;
    if (buf) {
        *buf = strdup(dlinfo.dli_fname);
        if (!*buf)
            return 0;
    }
    return 1;
}
#endif

#define  _str(s) # s
#define __str(s) _str(s)

struct _verto_ctx {
    size_t ref;
    verto_mod_ctx *ctx;
    const verto_module *module;
    verto_ev *events;
    int deflt;
    int exit;
};

typedef struct {
    verto_proc proc;
    verto_proc_status status;
} verto_child;

struct _verto_ev {
    verto_ev *next;
    verto_ctx *ctx;
    verto_ev_type type;
    verto_callback *callback;
    verto_callback *onfree;
    void *priv;
    verto_mod_ev *ev;
    verto_ev_flag flags;
    verto_ev_flag actual;
    size_t depth;
    int deleted;
    union {
        int fd;
        int signal;
        time_t interval;
        verto_child child;
    } option;
};

typedef struct _module_record module_record;
struct _module_record {
    module_record *next;
    const verto_module *module;
    pdlmtype *dll;
    char *filename;
    verto_ctx *defctx;
};

static module_record *loaded_modules;

static int
int_vasprintf(char **strp, const char *fmt, va_list ap) {
    va_list apc;
    int size = 0;

    va_copy(apc, ap);
    size = vsnprintf(NULL, 0, fmt, apc);
    va_end(apc);

    if (size <= 0 || !(*strp = malloc(size + 1)))
        return -1;

    return vsnprintf(*strp, size + 1, fmt, ap);
}

static int
int_asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    int size = 0;

    va_start(ap, fmt);
    size = int_vasprintf(strp, fmt, ap);
    va_end(ap);
    return size;
}

static char *
int_get_table_name(const char *suffix)
{
    char *tmp;

    tmp = malloc(strlen(suffix) + strlen(__str(VERTO_MODULE_TABLE())) + 1);
    if (tmp) {
        strcpy(tmp, __str(VERTO_MODULE_TABLE()));
        strcat(tmp, suffix);
    }
    return tmp;
}

static char *
int_get_table_name_from_filename(const char *filename)
{
    char *bn = NULL, *tmp = NULL;

    if (!filename)
        return NULL;

    tmp = strdup(filename);
    if (!tmp)
        return NULL;

    bn = basename(tmp);
    if (bn)
        bn = strdup(bn);
    free(tmp);
    if (!bn)
        return NULL;

    tmp = strchr(bn, '-');
    if (tmp) {
        if (strchr(tmp+1, '.')) {
            *strchr(tmp+1, '.') = '\0';
            tmp = int_get_table_name(tmp + 1);
        } else
            tmp = NULL;
    }

    free(bn);
    return tmp;
}

static int
do_load_file(const char *filename, int reqsym, verto_ev_type reqtypes,
             module_record **record)
{
    char *tblname = NULL;
    module_record *tmp;

    /* Check the loaded modules to see if we already loaded one */
    for (*record = loaded_modules ; *record ; *record = (*record)->next) {
        if (!strcmp((*record)->filename, filename))
            return 1;
    }

    /* Create our module record */
    tmp = *record =  malloc(sizeof(module_record));
    if (!tmp)
        return 0;
    memset(tmp, 0, sizeof(module_record));
    tmp->filename = strdup(filename);
    if (!tmp->filename)
        goto error;

    /* Get the name of the module struct in the library */
    tblname = int_get_table_name_from_filename(filename);
    if (!tblname)
        goto error;

    /* Open the module library */
    tmp->dll = pdlopenl(filename);
    if (!tmp->dll) {
        /* printf("%s -- %s\n", filename, pdlerror()); */
        goto error;
    }

    /* Get the module struct */
    tmp->module = (verto_module*) pdlsym(tmp->dll, tblname);
    if (!tmp->module || tmp->module->vers != VERTO_MODULE_VERSION
            || !tmp->module->funcs
            || !tmp->module->funcs->ctx_new)
        goto error;

    /* Check to make sure that we have our required symbol if reqsym == true */
    if (tmp->module->symb && reqsym
            && !pdlsymlinked(NULL, tmp->module->symb))
        goto error;

    /* Check to make sure that this module supports our required features */
    if (reqtypes != VERTO_EV_TYPE_NONE
            && (tmp->module->types & reqtypes) != reqtypes)
        goto error;

    /* Re-open in execution mode */
    tmp->dll = pdlreopen(filename, tmp->dll);
    if (!tmp->dll)
        goto error;

    /* Get the module struct again */
    tmp->module = (verto_module*) pdlsym(tmp->dll, tblname);
    if (!tmp->module)
        goto error;

    /* Append the new module to the end of the loaded modules */
    for (tmp = loaded_modules ; tmp && tmp->next; tmp = tmp->next)
        continue;
    if (tmp)
        tmp->next = *record;
    else
        loaded_modules = *record;

    free(tblname);
    return 1;

    error:
        free(tblname);
        free(tmp->filename);
        if (tmp->dll)
            pdlclose(tmp->dll);
        free(tmp);
        *record = NULL;
        return 0;
}

static int
do_load_dir(const char *dirname, const char *prefix, const char *suffix,
            int reqsym, verto_ev_type reqtypes, module_record **record)
{
    DIR *dir;
    struct dirent *ent = NULL;

    *record = NULL;
    dir = opendir(dirname);
    if (!dir)
        return 0;


    while ((ent = readdir(dir))) {
        char *tmp = NULL;
        int success;
        size_t flen, slen;

        flen = strlen(ent->d_name);
        slen = strlen(suffix);

        if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name))
            continue;
        if (strstr(ent->d_name, prefix) != ent->d_name)
            continue;
        if (flen < slen || strcmp(ent->d_name + flen - slen, suffix))
            continue;

        if (int_asprintf(&tmp, "%s/%s", dirname, ent->d_name) < 0)
            continue;

        success = do_load_file(tmp, reqsym, reqtypes, record);
        free(tmp);
        if (success)
            break;
        *record = NULL;
    }

    closedir(dir);
    return *record != NULL;
}

static int
load_module(const char *impl, verto_ev_type reqtypes, module_record **record)
{
    int success = 0;
    char *prefix = NULL;
    char *suffix = NULL;
    char *tmp = NULL;

    /* Check the cache */
    if (impl) {
        for (*record = loaded_modules ; *record ; *record = (*record)->next) {
            if ((strchr(impl, '/') && !strcmp(impl, (*record)->filename))
                    || !strcmp(impl, (*record)->module->name))
                return 1;
        }
    } else if (loaded_modules) {
        for (*record = loaded_modules ; *record ; *record = (*record)->next) {
            if (reqtypes == VERTO_EV_TYPE_NONE
                    || ((*record)->module->types & reqtypes) == reqtypes)
                return 1;
        }
    }

    if (!pdladdrmodname(verto_convert_module, &prefix))
        return 0;

    /* Example output:
     *    prefix == /usr/lib/libverto-
     *    impl == glib
     *    suffix == .so.0
     * Put them all together: /usr/lib/libverto-glib.so.0 */
    tmp = strdup(prefix);
    if (!tmp) {
        free(prefix);
        return 0;
    }

    suffix = basename(tmp);
    suffix = strchr(suffix, '.');
    if (!suffix || strlen(suffix) < 1 || !(suffix = strdup(suffix))) {
        free(prefix);
        free(tmp);
        return 0;
    }
    strcpy(prefix + strlen(prefix) - strlen(suffix), "-");
    free(tmp);

    if (impl) {
        /* Try to do a load by the path */
        if (!success && strchr(impl, '/'))
            success = do_load_file(impl, 0, reqtypes, record);
        if (!success) {
            /* Try to do a load by the name */
            tmp = NULL;
            if (int_asprintf(&tmp, "%s%s%s", prefix, impl, suffix) > 0) {
                success = do_load_file(tmp, 0, reqtypes, record);
                free(tmp);
            }
        }
    } else {
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
                success = do_load_dir(dname, prefix, suffix, 1, reqtypes,
                                      record);
                if (!success) {
#ifdef DEFAULT_LIBRARY
                    /* Attempt to find the default module */
                    success = load_module(DEFAULT_LIBRARY, reqtypes, record);
                    if (!success)
#endif /* DEFAULT_LIBRARY */
                    /* Attempt to load any plugin (we're desperate) */
                    success = do_load_dir(dname, prefix, suffix, 0,
                                          reqtypes, record);
                }
            }

            free(dname);
        }
    }

    free(suffix);
    free(prefix);
    return success;
}

static verto_ev *
make_ev(verto_ctx *ctx, verto_callback *callback,
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
        ev->flags      = flags;
    }

    return ev;
}

static void
push_ev(verto_ctx *ctx, verto_ev *ev)
{
    verto_ev *tmp;

    if (!ctx || !ev)
        return;

    tmp = ctx->events;
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
signal_ignore(verto_ctx *ctx, verto_ev *ev)
{
}

verto_ctx *
verto_new(const char *impl, verto_ev_type reqtypes)
{
    module_record *mr = NULL;

    if (!load_module(impl, reqtypes, &mr))
        return NULL;

    return verto_convert_module(mr->module, 0, NULL);
}

verto_ctx *
verto_default(const char *impl, verto_ev_type reqtypes)
{
    module_record *mr = NULL;

    if (!load_module(impl, reqtypes, &mr))
        return NULL;

    return verto_convert_module(mr->module, 1, NULL);
}

int
verto_set_default(const char *impl, verto_ev_type reqtypes)
{
    module_record *mr;

    if (loaded_modules || !impl)
        return 0;

    return load_module(impl, reqtypes, &mr);
}

void
verto_free(verto_ctx *ctx)
{
    if (!ctx)
        return;

    ctx->ref = ctx->ref > 0 ? ctx->ref - 1 : 0;
    if (ctx->ref > 0)
        return;

    /* Cancel all pending events */
    while (ctx->events)
        verto_del(ctx->events);

    /* Free the private */
    if (!ctx->deflt || !ctx->module->funcs->ctx_default)
        ctx->module->funcs->ctx_free(ctx->ctx);

    free(ctx);
}

void
verto_run(verto_ctx *ctx)
{
    if (!ctx)
        return;

    if (ctx->module->funcs->ctx_break && ctx->module->funcs->ctx_run)
        ctx->module->funcs->ctx_run(ctx->ctx);
    else {
        while (!ctx->exit)
            ctx->module->funcs->ctx_run_once(ctx->ctx);
        ctx->exit = 0;
    }
}

void
verto_run_once(verto_ctx *ctx)
{
    if (!ctx)
        return;
    ctx->module->funcs->ctx_run_once(ctx->ctx);
}

void
verto_break(verto_ctx *ctx)
{
    if (!ctx)
        return;

    if (ctx->module->funcs->ctx_break && ctx->module->funcs->ctx_run)
        ctx->module->funcs->ctx_break(ctx->ctx);
    else
        ctx->exit = 1;
}

void
verto_reinitialize(verto_ctx *ctx)
{
    verto_ev *tmp, *next;
    if (!ctx)
        return;

    /* Delete all events, but keep around the forkable ev structs */
    for (tmp = ctx->events; tmp; tmp = next) {
        next = tmp->next;

        if (tmp->flags & VERTO_EV_FLAG_REINITIABLE)
            ctx->module->funcs->ctx_del(ctx->ctx, tmp, tmp->ev);
        else
            verto_del(tmp);
    }

    /* Reinit the loop */
    if (ctx->module->funcs->ctx_reinitialize)
        ctx->module->funcs->ctx_reinitialize(ctx->ctx);

    /* Recreate events that were marked forkable */
    for (tmp = ctx->events; tmp; tmp = tmp->next) {
        tmp->actual = tmp->flags;
        tmp->ev = ctx->module->funcs->ctx_add(ctx->ctx, tmp, &tmp->actual);
        assert(tmp->ev);
    }
}

#define doadd(ev, set, type) \
    ev = make_ev(ctx, callback, type, flags); \
    if (ev) { \
        set; \
        ev->actual = ev->flags; \
        ev->ev = ctx->module->funcs->ctx_add(ctx->ctx, ev, &ev->actual); \
        if (!ev->ev) { \
            free(ev); \
            return NULL; \
        } \
        push_ev(ctx, ev); \
    }

verto_ev *
verto_add_io(verto_ctx *ctx, verto_ev_flag flags,
             verto_callback *callback, int fd)
{
    verto_ev *ev;

    if (fd < 0 || !(flags & (VERTO_EV_FLAG_IO_READ | VERTO_EV_FLAG_IO_WRITE)))
        return NULL;

    doadd(ev, ev->option.fd = fd, VERTO_EV_TYPE_IO);
    return ev;
}

verto_ev *
verto_add_timeout(verto_ctx *ctx, verto_ev_flag flags,
                  verto_callback *callback, time_t interval)
{
    verto_ev *ev;
    doadd(ev, ev->option.interval = interval, VERTO_EV_TYPE_TIMEOUT);
    return ev;
}

verto_ev *
verto_add_idle(verto_ctx *ctx, verto_ev_flag flags,
               verto_callback *callback)
{
    verto_ev *ev;
    doadd(ev,, VERTO_EV_TYPE_IDLE);
    return ev;
}

verto_ev *
verto_add_signal(verto_ctx *ctx, verto_ev_flag flags,
                 verto_callback *callback, int signal)
{
    verto_ev *ev;

    if (signal < 0)
        return NULL;
#ifndef WIN32
    if (signal == SIGCHLD)
        return NULL;
#endif
    if (callback == VERTO_SIG_IGN) {
        callback = signal_ignore;
        if (!(flags & VERTO_EV_FLAG_PERSIST))
            return NULL;
    }
    doadd(ev, ev->option.signal = signal, VERTO_EV_TYPE_SIGNAL);
    return ev;
}

verto_ev *
verto_add_child(verto_ctx *ctx, verto_ev_flag flags,
                verto_callback *callback, verto_proc proc)
{
    verto_ev *ev;

    if (flags & VERTO_EV_FLAG_PERSIST) /* persist makes no sense */
        return NULL;
#ifdef WIN32
    if (proc == NULL)
#else
    if (proc < 1)
#endif
        return NULL;
    doadd(ev, ev->option.child.proc = proc, VERTO_EV_TYPE_CHILD);
    return ev;
}

void
verto_set_private(verto_ev *ev, void *priv, verto_callback *free)
{
    if (!ev)
        return;
    if (ev->onfree && free)
        ev->onfree(ev->ctx, ev);
    ev->priv = priv;
    ev->onfree = free;
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

verto_proc
verto_get_proc(const verto_ev *ev) {
    if (ev && ev->type == VERTO_EV_TYPE_CHILD)
        return ev->option.child.proc;
    return (verto_proc) 0;
}

verto_proc_status
verto_get_proc_status(const verto_ev *ev)
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
        ev->deleted = 1;
        return;
    }

    if (ev->onfree)
        ev->onfree(ev->ctx, ev);
    ev->ctx->module->funcs->ctx_del(ev->ctx->ctx, ev, ev->ev);
    remove_ev(&(ev->ctx->events), ev);
    free(ev);
}

verto_ev_type
verto_get_supported_types(verto_ctx *ctx)
{
    return ctx->module->types;
}

/*** THE FOLLOWING ARE FOR IMPLEMENTATION MODULES ONLY ***/

verto_ctx *
verto_convert_module(const verto_module *module, int deflt, verto_mod_ctx *mctx)
{
    verto_ctx *ctx = NULL;
    module_record *mr;

    if (!module)
        goto error;

    if (deflt) {
        for (mr = loaded_modules ; mr ; mr = mr->next) {
            if (mr->module == module && mr->defctx) {
                if (mctx)
                    module->funcs->ctx_free(mctx);
                mr->defctx->ref++;
                return mr->defctx;
            }
        }
    }

    if (!mctx) {
        mctx = deflt
                    ? (module->funcs->ctx_default
                        ? module->funcs->ctx_default()
                        : module->funcs->ctx_new())
                    : module->funcs->ctx_new();
        if (!mctx)
            goto error;
    }

    ctx = malloc(sizeof(verto_ctx));
    if (!ctx)
        goto error;
    memset(ctx, 0, sizeof(verto_ctx));

    ctx->ref = 1;
    ctx->ctx = mctx;
    ctx->module = module;
    ctx->deflt = deflt;

    if (deflt) {
        module_record **tmp = &loaded_modules;

        for (mr = loaded_modules ; mr ; mr = mr->next) {
            if (mr->module == module) {
                assert(mr->defctx == NULL);
                mr->defctx = ctx;
                return ctx;
            }

            if (!mr->next) {
                tmp = &mr->next;
                break;
            }
        }

        *tmp = malloc(sizeof(module_record));
        if (!*tmp) {
            free(ctx);
            goto error;
        }

        memset(*tmp, 0, sizeof(module_record));
        (*tmp)->defctx = ctx;
        (*tmp)->module = module;
    }

    return ctx;

error:
    if (mctx)
        module->funcs->ctx_free(mctx);
    return NULL;
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
        else if (!ev->actual & VERTO_EV_FLAG_PERSIST) {
            ev->actual = ev->flags;
            priv = ev->ctx->module->funcs->ctx_add(ev->ctx->ctx, ev, &ev->actual);
            assert(priv); /* TODO: create an error callback */
            ev->ctx->module->funcs->ctx_del(ev->ctx->ctx, ev, ev->ev);
            ev->ev = priv;
        }
    }
}

void
verto_set_proc_status(verto_ev *ev, verto_proc_status status)
{
    if (ev && ev->type == VERTO_EV_TYPE_CHILD)
        ev->option.child.status = status;
}
