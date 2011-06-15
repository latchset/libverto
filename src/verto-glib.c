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

#include <verto-glib.h>
#include <verto-module.h>

/* DEFAULT, LOW, MEDIUM, HIGH */
static gint priority_map[4] = {50, 100, 50, 0};

struct glibEvCtx {
    GMainContext *context;
    GMainLoop *loop;
};

struct glibEv {
    struct vertoEv ev;
    GSource *src;
    guint tag;
    GIOChannel *chan;
};

static void *
glib_convert_(GMainContext *mc, GMainLoop *ml)
{
    struct glibEvCtx *l = NULL;

    l = g_new0(struct glibEvCtx, 1);
    if (l) {
        if (mc) {
            /* Steal references */
            l->context = mc;
            l->loop = ml ? ml : g_main_loop_new(l->context, FALSE);

            if (g_main_context_default() == mc)
                g_main_context_ref(mc);
        } else {
            l->context = g_main_context_ref(g_main_context_default());
            l->loop = g_main_loop_new(l->context, FALSE);
        }
    }

    return l;
}

static void *
glib_ctx_new()
{
    GMainContext *mc = g_main_context_new();
    void *lp = glib_convert_(mc, NULL);
    g_main_context_unref(mc);
    return lp;
}

static void *
glib_ctx_default()
{
    return glib_convert_(g_main_context_default(), NULL);
}

static void
glib_ctx_free(void *lp)
{
    g_main_loop_unref(((struct glibEvCtx*) lp)->loop);
    g_main_context_unref(((struct glibEvCtx*) lp)->context);
    g_free(lp);
}

static void
glib_ctx_run(void *lp)
{
    g_main_loop_run(((struct glibEvCtx*) lp)->loop);
}

static void
glib_ctx_run_once(void *lp)
{
    g_main_context_iteration(((struct glibEvCtx*) lp)->context, TRUE);
}

static void
glib_ctx_break(void *lp)
{
    g_main_loop_quit(((struct glibEvCtx*) lp)->loop);
}

static gboolean
glib_callback(gpointer data)
{
    struct glibEv *ev = ((struct glibEv*) data);
    if (ev)
        ev->ev.callback(&ev->ev);
    return TRUE;
}

#define ctx_add(makesrc) \
    struct glibEv *ev = g_new0(struct glibEv, 1); \
    if (!ev) \
        goto error; \
    makesrc; \
    if (!ev->src) \
        goto error; \
    g_source_set_can_recurse(ev->src, FALSE); \
    g_source_set_priority(ev->src, priority_map[priority]); \
    g_source_set_callback(ev->src, glib_callback, ev, NULL); \
    ev->tag = g_source_attach(ev->src, ((struct glibLoop*) lp)->context); \
    if (ev->tag == 0) \
        goto error; \
    return &ev->ev; \
    error: \
        verto_ctx_del(ev); \
        return NULL

#define ctx_add_io(flag) \
    ctx_add( \
        ev->chan = g_io_channel_unix_new(fd); \
        if (!ev->chan) \
            goto error; \
        g_io_channel_set_close_on_unref(ev->chan, FALSE); \
        ev->src = g_io_create_watch(ev->chan, flag) \
    )

static struct vertoEv *
glib_ctx_add_read(void *lp, enum vertoEvPriority priority,
                  vertoCallback callback, void *data, int fd)
{
    ctx_add_io(G_IO_IN);
}

static struct vertoEv *
glib_ctx_add_write(void *lp, enum vertoEvPriority priority,
                   vertoCallback callback, void *data, int fd)
{
    ctx_add_io(G_IO_OUT);
}

static struct vertoEv *
glib_ctx_add_timeout(void *lp, enum vertoEvPriority priority,
                     vertoCallback callback, void *data, time_t interval)
{
    ctx_add(ev->src = g_timeout_source_new(interval));
}

static struct vertoEv *
glib_ctx_add_idle(void *lp, enum vertoEvPriority priority,
                  vertoCallback callback, void *data)
{
    ctx_add(ev->src = g_idle_source_new());
}

static struct vertoEv *
glib_ctx_add_signal(void *lp, enum vertoEvPriority priority,
                    vertoCallback callback, void *data, int signal)
{
    ctx_add(ev->src = g_unix_signal_source_new(signal));
}

static struct vertoEv *
glib_ctx_add_child(void *lp, enum vertoEvPriority priority,
                                    vertoCallback callback, void *data,
                                    pid_t pid)
{
    ctx_add(ev->src = g_child_watch_source_new(pid));
}

static void
glib_ctx_del(void *lp, struct vertoEv *ev)
{
    if (!ev)
        return;

    if (((struct glibEv*) ev)->tag > 0)
        g_source_remove(((struct glibEv*) ev)->tag);
    if (((struct glibEv*) ev)->src)
        g_source_unref(((struct glibEv*) ev)->src);
    if (((struct glibEv*) ev)->chan)
        g_io_channel_unref(((struct glibEv*) ev)->chan);
    g_free(ev);
}

VERTO_MODULE(glib, g_main_context_default);

struct vertoEvCtx *
verto_convert_glib(GMainContext *mc, GMainLoop *ml)
{
    return verto_convert_funcs(&glib_funcs, glib_convert_(mc, ml));
}

