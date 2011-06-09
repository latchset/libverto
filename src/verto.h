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

#ifndef VERTO_H_
#define VERTO_H_

#include <stdarg.h>

#define _VERTO_MODULE_VERSION 1
#define _VERTO_MODULE_TABLE verto_module_table
#define VERTO_MODULE(name, symb) \
	struct _vModule _VERTO_MODULE_TABLE = { \
			_VERTO_MODULE_VERSION, \
			"v_context_new_" # name, \
			# symb \
	}; \
	vContext *v_context_new_ ## name()

struct _vModule {
	unsigned int vers;
	const char  *name;
	const char  *symb;
};

typedef enum {
	V_HOOK_TYPE_READ    = 1 << 0,
	V_HOOK_TYPE_WRITE   = 1 << 1,
	V_HOOK_TYPE_TIMEOUT = 1 << 2,
	V_HOOK_TYPE_SIGNAL  = 1 << 3,
	V_HOOK_TYPE_CHILD   = 1 << 4,
} vHookType;

typedef enum {
	V_HOOK_PRIORITY_DEFAULT = 0,
	V_HOOK_PRIORITY_LOW     = 1,
	V_HOOK_PRIORITY_MEDIUM  = 2,
	V_HOOK_PRIORITY_HIGH    = 3,
} vHookPriority;

typedef struct _vContext  vContext;
typedef struct _vLoop     vLoop;
typedef struct _vLoopSpec vLoopSpec;
typedef struct _vHook     vHook;

typedef void vLoopPrivate;

typedef void (*vCallback)(vLoop *loop, vHook *hook);

struct _vLoopSpec {
	vLoopPrivate *(*loop_new)     ();
	vLoopPrivate *(*loop_default) ();
	vLoopPrivate *(*loop_convert) (va_list args);
	void          (*loop_free)    (vLoopPrivate *loopdata);
	void          (*loop_run)     (vLoopPrivate *loopdata);
	void          (*loop_run_once)(vLoopPrivate *loopdata);
	void          (*loop_break)   (vLoopPrivate *loopdata);
	vHook        *(*loop_hook_add)(vLoopPrivate *lp, vLoop *loop, vCallback callback, void *data, vHookType type, long long spec, vHookPriority priority);
	void          (*loop_hook_del)(vLoopPrivate *lp, vHook *hook);
};

struct _vHook {
	vHookType     type;
	vHookPriority priority;
	long long     spec;
	vCallback     callback;
	void         *data;
};

vContext *v_context_new(const vLoopSpec *loopspec);
vContext *v_context_load(const char *name_or_path);

vContext *v_context_incref(vContext *ctx);
void      v_context_decref(vContext *ctx);

vContext *v_context_default_get(void);
void      v_context_default_set(vContext *ctx);

vLoop    *v_loop_new(vContext *ctx);
vLoop    *v_loop_default(vContext *ctx);
vLoop    *v_loop_convert(vContext *ctx, ...);

vLoop    *v_loop_incref(vLoop *loop);
void      v_loop_decref(vLoop *loop);

void      v_loop_run(vLoop *loop);
void      v_loop_run_once(vLoop *loop);
void      v_loop_break(vLoop *loop);

vHook    *v_loop_hook_add(vLoop *loop, vCallback callback, void *data, vHookType type, long long spec, vHookPriority priority);
void      v_loop_hook_del(vLoop *loop, vHook *hook);

#endif /* VERTO_H_ */
