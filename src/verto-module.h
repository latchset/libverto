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

/*** THE FOLLOWING ARE FOR IMPLEMENTATION MODULES ONLY ***/

#ifndef VERTO_MODULE_H_
#define VERTO_MODULE_H_

#include <verto.h>

#define _VERTO_MODULE_VERSION 1
#define _VERTO_MODULE_TABLE verto_module_table
#define VERTO_MODULE(name, symb) \
    static struct vertoEvCtxFuncs name ## _funcs = { \
        name ## _ctx_new, \
        name ## _ctx_default, \
        name ## _ctx_free, \
        name ## _ctx_run, \
        name ## _ctx_run_once, \
        name ## _ctx_break, \
        name ## _ctx_add_read, \
        name ## _ctx_add_write, \
        name ## _ctx_add_timeout, \
        name ## _ctx_add_idle, \
        name ## _ctx_add_signal, \
        name ## _ctx_add_child, \
        name ## _ctx_del \
    }; \
    struct vertoEvCtx *verto_new_ ## name() { \
        return verto_new_funcs(&name ## _funcs); \
    } \
    struct vertoEvCtx *verto_default_ ## name() { \
        return verto_default_funcs(&name ## _funcs); \
    } \
    struct vertoModule _VERTO_MODULE_TABLE = { \
                    _VERTO_MODULE_VERSION, \
                    # name, \
                    # symb, \
                    verto_new_ ## name, \
                    verto_default_ ## name, \
    };

typedef struct vertoEvCtx *(*vertoEvCtxConstructor)(void);

struct vertoModule {
    unsigned int vers;
    const char *name;
    const char *symb;
    vertoEvCtxConstructor new_ctx;
    vertoEvCtxConstructor def_ctx;
};

struct vertoEvCtxFuncs {
    void *(*ctx_new)();
    void *(*ctx_default)();
    void (*ctx_free)(void *lp);
    void (*ctx_run)(void *lp);
    void (*ctx_run_once)(void *lp);
    void (*ctx_break)(void *lp);
    struct vertoEv *(*ctx_add_read)(void *lp, enum vertoEvPriority priority,
                                       vertoCallback callback, void *data,
                                       int fd);
    struct vertoEv *(*ctx_add_write)(void *lp, enum vertoEvPriority priority,
                                        vertoCallback callback, void *data,
                                        int fd);
    struct vertoEv *(*ctx_add_timeout)(void *lp, enum vertoEvPriority priority,
                                          vertoCallback callback, void *data,
                                          time_t interval);
    struct vertoEv *(*ctx_add_idle)(void *lp, enum vertoEvPriority priority,
                                       vertoCallback callback, void *data);
    struct vertoEv *(*ctx_add_signal)(void *lp, enum vertoEvPriority priority,
                                         vertoCallback callback, void *data,
                                         int signal);
    struct vertoEv *(*ctx_add_child)(void *lp, enum vertoEvPriority priority,
                                        vertoCallback callback, void *data,
                                        pid_t pid);
    void (*ctx_del)(void *lp, struct vertoEv *ev);
};

/**
 * Creates a new event loop from a vertoEvCtxFuncs.
 *
 * You don't want this function unless you are writing an implementation
 * module or are integrating a home-baked event loop into verto.
 *
 * @param funcs The vertoEvCtxFuncs vtable.
 * @return A new EvCtx, or NULL on error.  Call verto_free() when done.
 */
struct vertoEvCtx *
verto_new_funcs(const struct vertoEvCtxFuncs *funcs);

/**
 * Gets the default event loop from a vertoEvCtxFuncs.
 *
 * You don't want this function unless you are writing an implementation
 * module or are integrating a home-baked event loop into verto.
 *
 * @param funcs The vertoEvCtxFuncs vtable.
 * @return The default EvCtx, or NULL on error.  Call verto_free() when done.
 */
struct vertoEvCtx *
verto_default_funcs(const struct vertoEvCtxFuncs *funcs);

/**
 * Converts an existing implementation specific loop to a verto loop.
 *
 * You probably don't want this function.  This function exists for creating
 * implementation modules.  See instead verto-NAME.h, where NAME is your
 * specific implementation.
 *
 * @param funcs The vertoEvCtxFuncs vtable.
 * @return A new EvCtx, or NULL on error.  Call verto_decref() when done.
 */
struct vertoEvCtx *
verto_convert_funcs(const struct vertoEvCtxFuncs *funcs, void *ctx_private);

#endif /* VERTO_MODULE_H_ */