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

#include <stdlib.h>

#include <ev.h>
#include <verto-ev.h>

static vLoopPrivate *libev_loop_new() {
	return ev_loop_new(EVFLAG_AUTO);
}

static vLoopPrivate *libev_loop_default() {
	return ev_default_loop(EVFLAG_AUTO);
}

static vLoopPrivate *libev_loop_convert(va_list args) {
	va_list copy;
	va_copy(copy, args);
	struct ev_loop* l = va_arg(copy, struct ev_loop*);
	va_end(copy);
	return l;
}

static void libev_loop_free(vLoopPrivate *loopdata) {
	if (loopdata != EV_DEFAULT)
		ev_loop_destroy(loopdata);
}

static void libev_loop_run(vLoopPrivate *loopdata) {
	ev_run(loopdata, 0);
}

static void libev_loop_run_once(vLoopPrivate *loopdata) {
	ev_run(loopdata, EVRUN_ONCE);
}

static void libev_loop_break(vLoopPrivate *loopdata) {
	ev_break(loopdata, EVBREAK_ONE);
}

typedef struct _hookData {
	vHook     hook;
	vLoop    *loop;
	void     *watcher;
} hookData;

#define setuptype(type, ...) \
	hd->watcher = malloc(sizeof(ev_ ## type)); \
	if (!hd->watcher) goto error; \
	ev_ ## type ## _init((ev_ ## type *) hd->watcher, __VA_ARGS__); \
	((ev_ ## type *) hd->watcher)->data = hd; \
	ev_ ## type ## _start(priv, hd->watcher); \
	return &hd->hook;

#define setupcb(type) \
	static void type ## _cb (EV_P_ ev_ ## type *w, int revents) { \
		hookData *hd = ((hookData*) w->data); \
		hd->hook.callback(hd->loop, &hd->hook); \
	}

setupcb(io)
setupcb(idle)
setupcb(timer)
setupcb(signal)
setupcb(child)

static vHook *libev_loop_hook_add(vLoopPrivate *priv, vLoop *loop, vCallback callback, void *data, vHookType type, long long spec, vHookPriority priority) {
	hookData *hd = malloc(sizeof(hookData));
	if (hd) {
		hd->hook.type     = type;
		hd->hook.priority = priority;
		hd->hook.spec     = spec;
		hd->hook.callback = callback;
		hd->hook.data     = data;
		hd->loop          = loop;

		switch (type) {
		case V_HOOK_TYPE_READ:
			setuptype(io, io_cb, spec, EV_READ);
		case V_HOOK_TYPE_WRITE:
			setuptype(io, io_cb, spec, EV_WRITE);
		case V_HOOK_TYPE_TIMEOUT:
			if (spec == 0) {
				setuptype(idle, idle_cb);
			}
			setuptype(timer, timer_cb, ((ev_tstamp) spec) / 1000.0, ((ev_tstamp) spec) / 1000.0);
		case V_HOOK_TYPE_SIGNAL:
			setuptype(signal, signal_cb, spec);
		case V_HOOK_TYPE_CHILD:
			setuptype(child, child_cb, spec, 0);
		}
	}

	error:
		free(hd);
		return NULL;
}

static void libev_loop_hook_del(vLoopPrivate *priv, vHook *hook) {
	if (!priv || !hook) return;

	switch (hook->type) {
	case V_HOOK_TYPE_READ:
		ev_io_stop(priv, ((hookData*) hook)->watcher);
	case V_HOOK_TYPE_WRITE:
		ev_io_stop(priv, ((hookData*) hook)->watcher);
	case V_HOOK_TYPE_TIMEOUT:
		if (hook->spec == 0)
			ev_idle_stop(priv, ((hookData*) hook)->watcher);
		else
			ev_timer_stop(priv, ((hookData*) hook)->watcher);
	case V_HOOK_TYPE_SIGNAL:
		ev_signal_stop(priv, ((hookData*) hook)->watcher);
	case V_HOOK_TYPE_CHILD:
		ev_child_stop(priv, ((hookData*) hook)->watcher);
	}

	free(hook);
}

static vLoopSpec spec = {
	.loop_new      = libev_loop_new,
	.loop_default  = libev_loop_default,
	.loop_convert  = libev_loop_convert,
	.loop_free     = libev_loop_free,
	.loop_run      = libev_loop_run,
	.loop_run_once = libev_loop_run_once,
	.loop_break    = libev_loop_break,
	.loop_hook_add = libev_loop_hook_add,
	.loop_hook_del = libev_loop_hook_del
};

VERTO_MODULE(ev, ev_loop_new) {
	return v_context_new(&spec);
}
