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

#define VERTO_MODULE_VERSION 1
#define VERTO_MODULE_TABLE verto_module_table
#define VERTO_MODULE(name, symb) \
    static struct vertoEvCtxFuncs name ## _funcs = { \
        name ## _ctx_free, \
        name ## _ctx_run, \
        name ## _ctx_run_once, \
        name ## _ctx_break, \
        name ## _ctx_add, \
        name ## _ctx_del \
    }; \
    struct vertoModule VERTO_MODULE_TABLE = { \
                    VERTO_MODULE_VERSION, \
                    # name, \
                    # symb, \
                    verto_new_ ## name, \
                    verto_default_ ## name, \
    };

typedef struct vertoEvCtx *(*vertoEvCtxConstructor)();

struct vertoModule {
    unsigned int vers;
    const char *name;
    const char *symb;
    vertoEvCtxConstructor new_ctx;
    vertoEvCtxConstructor def_ctx;
};

struct vertoEvCtxFuncs {
    void  (*ctx_free)(void *ctx);
    void  (*ctx_run)(void *ctx);
    void  (*ctx_run_once)(void *ctx);
    void  (*ctx_break)(void *ctx);
    void *(*ctx_add)(void *ctx, const struct vertoEv *ev);
    void  (*ctx_del)(void *ctx, const struct vertoEv *ev, void *evpriv);
};

/**
 * Converts an existing implementation specific loop to a vertoEvCtx.
 *
 * This function also sets the internal default implementation so that future
 * calls to verto_new(NULL) or verto_default(NULL) will use this specific
 * implementation.
 *
 * @param name The name of the module (unquoted)
 * @param priv The context private to store
 * @return A new EvCtx, or NULL on error. Call verto_free() when done.
 */
#define verto_convert(name, priv) \
        verto_convert_funcs(&name ## _funcs, &VERTO_MODULE_TABLE, priv)

/**
 * Converts an existing implementation specific loop to a vertoEvCtx.
 *
 * This function also sets the internal default implementation so that future
 * calls to verto_new(NULL) or verto_default(NULL) will use this specific
 * implementation.
 *
 * If you are a module implementation, you probably want the macro above.  This
 * function is generally used directly only when an application is attempting
 * to expose a home-grown event loop to verto.
 *
 * @param name The name of the module (unquoted)
 * @param priv The context private to store
 * @return A new EvCtx, or NULL on error. Call verto_free() when done.
 */
struct vertoEvCtx *
verto_convert_funcs(const struct vertoEvCtxFuncs *funcs,
                    const struct vertoModule *module,
                    void *priv);

/**
 * Calls the callback of the vertoEv and then frees it via verto_del().
 *
 * The vertoEv is not freed (verto_del() is not called) if it is a signal event.
 *
 * @see verto_add_read()
 * @see verto_add_write()
 * @see verto_add_timeout()
 * @see verto_add_idle()
 * @see verto_add_signal()
 * @see verto_add_child()
 * @see verto_del()
 * @param ev The vertoEv
 */
void
verto_fire(struct vertoEv *ev);

/**
 * Sets the status of the pid which caused this event to fire.
 *
 * This function does nothing if the vertoEv is not a child type.
 *
 * @see verto_add_child()
 * @param ev The vertoEv to set the status in.
 * @param status The pid status.
 */
void
verto_set_pid_status(struct vertoEv *ev, int status);

#endif /* VERTO_MODULE_H_ */
