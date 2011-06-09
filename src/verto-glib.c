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

#include <glib.h>
#include <verto-glib.h>

#include <setjmp.h>

typedef struct _glibLoop {
	GMainContext *context;
	GMainLoop    *loop;
} glibLoop;

static vLoopPrivate *glib_convert_(GMainContext *mc, GMainLoop *ml) {
	glibLoop *l = g_new0(glibLoop, 1);
	if (l) {
		if (mc) {
			l->context = g_main_context_ref(mc);
			if (ml)
				l->loop = g_main_loop_ref(ml);
			else
				l->loop = g_main_loop_new(l->context, FALSE);
		} else {
			l->context = g_main_context_ref(g_main_context_default());
			l->loop = g_main_loop_new(l->context, FALSE);
		}
	}

	return l;
}

static vLoopPrivate *glib_loop_new() {
	GMainContext *mc = g_main_context_new();
	vLoopPrivate *lp = glib_convert_(mc, NULL);
	g_main_context_unref(mc);
	return lp;
}

static vLoopPrivate *glib_loop_default() {
	return glib_convert_(g_main_context_default(), NULL);
}

static vLoopPrivate *glib_loop_convert(va_list args) {
	va_list copy;
	va_copy(copy, args);

	GMainContext *mc = va_arg(copy, GMainContext*);
	GMainLoop    *ml = va_arg(copy, GMainLoop*);
	vLoopPrivate *lp = glib_convert_(mc, ml);
	va_end(copy);
	return lp;
}

static void glib_loop_free(vLoopPrivate *loopdata) {
	g_main_loop_unref(((glibLoop*) loopdata)->loop);
	g_main_context_unref(((glibLoop*) loopdata)->context);
	g_free(loopdata);
}

static void glib_loop_run(vLoopPrivate *loopdata) {
	g_main_loop_run(((glibLoop*) loopdata)->loop);
}

static void glib_loop_run_once(vLoopPrivate *loopdata) {
	g_main_context_iteration(((glibLoop*) loopdata)->context, TRUE);
}

static void glib_loop_break(vLoopPrivate *loopdata) {
	g_main_loop_quit(((glibLoop*) loopdata)->loop);
}

static unsigned int glib_loop_hook_add(vLoop *loop, vLoopPrivate *priv, vCallback callback, void *data, vHookType type, long long spec, vHookPriority priority) {
switch (type) {
	case V_HOOK_TYPE_READ:

		break;
	case V_HOOK_TYPE_WRITE:
		break;
	case V_HOOK_TYPE_TIMEOUT:
		break;
	case V_HOOK_TYPE_SIGNAL:
		break;
	case V_HOOK_TYPE_CHILD:
		break;
	default:
		break;
}
}

static void glib_loop_hook_del(vLoop *loop, unsigned int hook) {

}

static vLoopSpec spec = {
	.loop_new      = glib_loop_new,
	.loop_default  = glib_loop_default,
	.loop_convert  = glib_loop_convert,
	.loop_free     = glib_loop_free,
	.loop_run      = glib_loop_run,
	.loop_run_once = glib_loop_run_once,
	.loop_break    = glib_loop_break,
	.loop_hook_add = glib_loop_hook_add,
	.loop_hook_del = glib_loop_hook_del
};

VERTO_MODULE(glib, g_main_context_default) {
	return v_context_new(&spec);
}
