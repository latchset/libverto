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

#include <stdint.h> /* For uintptr_t */
#include <time.h>   /* For time_t */
#include <unistd.h> /* For pid_t */

struct vertoEvCtx;
struct vertoEv;

typedef void (*vertoCallback)(struct vertoEv *ev);

enum vertoEvType {
    VERTO_EV_TYPE_READ = 1,
    VERTO_EV_TYPE_WRITE = 1 << 1,
    VERTO_EV_TYPE_TIMEOUT = 1 << 2,
    VERTO_EV_TYPE_IDLE = 1 << 3,
    VERTO_EV_TYPE_SIGNAL = 1 << 4,
    VERTO_EV_TYPE_CHILD = 1 << 5,
    _VERTO_EV_TYPE_MAX = VERTO_EV_TYPE_CHILD
};

enum vertoEvPriority {
    VERTO_EV_PRIORITY_DEFAULT = 0,
    VERTO_EV_PRIORITY_LOW = 1,
    VERTO_EV_PRIORITY_MEDIUM = 2,
    VERTO_EV_PRIORITY_HIGH = 3,
    _VERTO_EV_PRIORITY_MAX = VERTO_EV_PRIORITY_HIGH
};

struct vertoEv {
    struct vertoEvCtx *ctx;
    enum vertoEvType type;
    enum vertoEvPriority priority;
    vertoCallback callback;
    void *priv;
    union {
        int fd;
        int signal;
        time_t interval;
        pid_t child;
        void *rsvd; /* Reserved for future expansion */
    } data;
};

/**
 * Creates a new event context using an optionally specified implementation.
 *
 * If you are an application that has already decided on using a particular
 * event loop implementation, you should not call this function, but instead
 * import the verto-NAME.h header and link against the verto-NAME.so, where
 * NAME is the implementation you wish to use.
 *
 * If you are a library, you should generally avoid creating event contexts
 * on your own but allow applications to pass in a vertoEvCtx you can use.
 *
 * There are two cases where you should use this function.  The first is
 * where you have a need to choose an implementation at run time, usually
 * for testing purposes.  The second and more common is when you simply
 * wish to remain implementation agnostic.  In this later case, you should
 * always call like this: verto_new(NULL).  This lets verto choose the best
 * implementation to use.
 *
 * If impl is not NULL, a new context is returned which is backed by the
 * implementation specified. If the implementation specified is not
 * available, NULL is returned.  The parameter 'impl' can specify:
 *   * The full path to an implementation library
 *   * The name of the implementation library (i.e. - "glib" or "libev")
 *
 * If impl is NULL, verto will attempt to automatically determine the
 * best implementation to use.
 *
 * First, verto will attempt to use an existing, previously loaded
 * implementation.  This is handled automatically by internal caching.
 *
 * Second, verto will attempt to discern if you are already linked to any
 * of the supported implementations (to avoid wasting memory by loading
 * extra unnecessary libraries).  If you are linked to one supported
 * implementation, that implementation will be chosen.  If you are linked
 * to more than one supported implementation one of the ones linked to
 * will be chosen, but the order of the particular choice is undefined.
 *
 * Third, verto will attempt to load the compile-time default, if available.
 *
 * Last, verto will attempt to load any implementation installed.  The
 * specific order of this step is undefined.
 *
 * @param impl The implementation to use, or NULL.
 * @return A new EvCtx, or NULL on error.  Call verto_free() when done.
 */
struct vertoEvCtx *
verto_new(const char *impl);

/**
 * Gets the default event context using an optionally specified implementation.
 *
 * You probably don't want this function.  See verto_new() for details.
 *
 * @see verto_new()
 * @param impl The implementation to use, or NULL.
 * @return The default EvCtx, or NULL on error.  Call verto_free() when done.
 */
struct vertoEvCtx *
verto_default(const char *impl);

/**
 * Frees a vertoEvCtx.
 *
 * @param ctx The vertoEvCtx to free.
 */
void
verto_free(struct vertoEvCtx *ctx);

/**
 * Run the vertoEvCtx forever, or at least until verto_break() is called.
 *
 * @see verto_break()
 * @param ctx The vertoEvCtx to run.
 */
void
verto_run(struct vertoEvCtx *ctx);

/**
 * Run the vertoEvCtx once. May block.
 *
 * @param ctx The vertoEvCtx to run once.
 */
void
verto_run_once(struct vertoEvCtx *ctx);

/**
 * Exits the currently running vertoEvCtx.
 *
 * @see verto_run()
 * @param ctx The vertoEvCtx to exit.
 */
void
verto_break(struct vertoEvCtx *ctx);

/**
 * Adds a callback executed when a file descriptor is ready to be read.
 *
 * To stop this callback from being called for future events, see verto_del().
 *
 * The vertoEv returned is automatically freed either when verto_del() is
 * called or when its vertoEvCtx is freed.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param priority The priority of the event (priority is not supported on
 *                 all implementations).
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param fd The file descriptor to watch for reads.
 * @return The vertoEv registered with the event context.
 */
struct vertoEv *
verto_add_read(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
               vertoCallback callback, void *priv, int fd);

/**
 * Adds a callback executed when a file descriptor is ready to be written.
 *
 * To stop this callback from being called for future events, see verto_del().
 *
 * The vertoEv returned is automatically freed either when verto_del() is
 * called or when its vertoEvCtx is freed.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param priority The priority of the event (priority is not supported on
 *                 all implementations).
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param fd The file descriptor to watch for writes.
 * @return The vertoEv registered with the event context.
 */
struct vertoEv *
verto_add_write(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                vertoCallback callback, void *priv, int fd);

/**
 * Adds a callback executed after a period of time.
 *
 * To stop this callback from being called for future events, see verto_del().
 *
 * The vertoEv returned is automatically freed either when verto_del() is
 * called or when its vertoEvCtx is freed.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param priority The priority of the event (priority is not supported on
 *                 all implementations).
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param interval Time period to wait before firing (in milliseconds).
 * @return The vertoEv registered with the event context.
 */
struct vertoEv *
verto_add_timeout(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                  vertoCallback callback, void *priv, time_t interval);

/**
 * Adds a callback executed when there is nothing else to do.
 *
 * To stop this callback from being called for future events, see verto_del().
 *
 * The vertoEv returned is automatically freed either when verto_del() is
 * called or when its vertoEvCtx is freed.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param priority The priority of the event (priority is not supported on
 *                 all implementations).
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @return The vertoEv registered with the event context.
 */
struct vertoEv *
verto_add_idle(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
               vertoCallback callback, void *priv);

/**
 * Adds a callback executed when a signal is received.
 *
 * To stop this callback from being called for future events, see verto_del().
 *
 * The vertoEv returned is automatically freed either when verto_del() is
 * called or when its vertoEvCtx is freed.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param priority The priority of the event (priority is not supported on
 *                 all implementations).
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param signal The signal to watch for.
 * @return The vertoEv registered with the event context.
 */
struct vertoEv *
verto_add_signal(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                 vertoCallback callback, void *priv, int signal);

/**
 * Adds a callback executed when a child process exits.
 *
 * To stop this callback from being called for future events, see verto_del().
 *
 * The vertoEv returned is automatically freed either when verto_del() is
 * called or when its vertoEvCtx is freed.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param priority The priority of the event (priority is not supported on
 *                 all implementations).
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param child The pid of the child to watch for.
 * @return The vertoEv registered with the event context.
 */
struct vertoEv *
verto_add_child(struct vertoEvCtx *ctx, enum vertoEvPriority priority,
                vertoCallback callback, void *priv, pid_t child);

/**
 * Gets the file descriptor associated with a read/write vertoEv.
 *
 * @see verto_add_read()
 * @see verto_add_write()
 * @param ev The vertoEv to retrieve the file descriptor from.
 * @return The file descriptor, or -1 if not a read/write event.
 */
#define verto_get_fd(ev) \
    (((ev)->type & (VERTO_EV_TYPE_READ | VERTO_EV_TYPE_WRITE)) \
            ? (ev)->data.fd : (int) -1)

/**
 * Gets the interval associated with a timeout vertoEv.
 *
 * @see verto_add_timeout()
 * @param ev The vertoEv to retrieve the interval from.
 * @return The interval, or 0 if not a timeout event.
 */
#define verto_get_interval(ev) \
    (((ev)->type & VERTO_EV_TYPE_TIMEOUT) \
            ? (ev)->data.timeout : (time_t) 0)

/**
 * Gets the signal associated with a signal vertoEv.
 *
 * @see verto_add_signal()
 * @param ev The vertoEv to retrieve the signal from.
 * @return The signal, or -1 if not a signal event.
 */
#define verto_get_signal(ev) \
    (((ev)->type & VERTO_EV_TYPE_SIGNAL) \
            ? (ev)->data.signal : (int) -1)

/**
 * Gets the pid associated with a child vertoEv.
 *
 * @see verto_add_child()
 * @param ev The vertoEv to retrieve the file descriptor from.
 * @return The pid, or 0 if not a child event.
 */
#define verto_get_pid(ev) \
    (((ev)->type & VERTO_EV_TYPE_CHILD) \
            ? (ev)->data.child : (pid_t) 0)

/**
 * Removes an event from from the event context and frees it.
 *
 * The event and its contents cannot be used after this call.
 *
 * @see verto_add_read()
 * @see verto_add_write()
 * @see verto_add_timeout()
 * @see verto_add_idle()
 * @see verto_add_signal()
 * @see verto_add_child()
 * @param ev The event to delete.
 */
void
verto_del(struct vertoEv *ev);

#endif /* VERTO_H_ */
