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

#include <time.h>   /* For time_t */
#include <unistd.h> /* For pid_t */

#define VERTO_SIG_IGN ((vertoCallback *) 1)

typedef struct _vertoEvCtx vertoEvCtx;
typedef struct _vertoEv vertoEv;

typedef enum {
    VERTO_EV_TYPE_IO,
    VERTO_EV_TYPE_TIMEOUT,
    VERTO_EV_TYPE_IDLE,
    VERTO_EV_TYPE_SIGNAL,
    VERTO_EV_TYPE_CHILD
} vertoEvType;

typedef enum {
    VERTO_EV_FLAG_NONE = 0,
    VERTO_EV_FLAG_PERSIST = 1,
    VERTO_EV_FLAG_PRIORITY_LOW = 1 << 1,
    VERTO_EV_FLAG_PRIORITY_MEDIUM = 1 << 2,
    VERTO_EV_FLAG_PRIORITY_HIGH = 1 << 3,
    VERTO_EV_FLAG_IO_READ = 1 << 4,
    VERTO_EV_FLAG_IO_WRITE = 1 << 5,
    _VERTO_EV_FLAG_MAX = VERTO_EV_FLAG_IO_WRITE
} vertoEvFlag;

typedef void (vertoCallback)(vertoEvCtx *ctx, vertoEv *ev);

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
 * implementation. This is handled automatically by internal caching of either
 * the first implementation loaded or the one specified by verto_set_default().
 *
 * Second, verto will attempt to discern if you are already linked to any
 * of the supported implementations (to avoid wasting memory by loading
 * extra unnecessary libraries).  If you are linked to one supported
 * implementation, that implementation will be chosen.  If you are linked
 * to more than one supported implementation one of the ones linked to
 * will be chosen, but the order of the particular choice is undefined.
 *
 * Third, verto will attempt to load the compile-time default, if defined at
 * build time and available at runtime.
 *
 * Last, verto will attempt to load any implementation installed. The specific
 * order of this step is undefined.
 *
 * @see verto_set_default()
 * @param impl The implementation to use, or NULL.
 * @return A new EvCtx, or NULL on error.  Call verto_free() when done.
 */
vertoEvCtx *
verto_new(const char *impl);

/**
 * Gets the default event context using an optionally specified implementation.
 *
 * This function is essentially a singleton version of verto_new().  However,
 * since this function must return the same loop as the *_default() call of
 * the underlying implementation (if such a function exists), it is NOT a
 * global singleton, but a per-implementation singleton. For this reason, you
 * must call verto_free() when you are done with this loop. Even after calling
 * verto_free() on the default vertoEvCtx, you can safely call verto_default()
 * again and receive a new reference to the same (internally default) loop.
 *
 * In all other respects, verto_default() acts exactly like verto_new().
 *
 * @see verto_new()
 * @see verto_free()
 * @param impl The implementation to use, or NULL.
 * @return The default EvCtx, or NULL on error.  Call verto_free() when done.
 */
vertoEvCtx *
verto_default(const char *impl);

/**
 * Sets the default implementation to use by its name.
 *
 * This function returns 1 on success and 0 on failure.  It can fail for the
 * following reasons:
 *   1. The default implementation was already set via verto_set_default().
 *   2. The implementation specified could not be found.
 *   3. The impl argument was NULL.
 *   4. verto_new() was already called.
 *   5. verto_default() was already called.
 *   6. verto_new_NAME() was already called.
 *   7. verto_default_NAME() was already called.
 *   8. verto_convert_NAME() was already called.
 *
 * @see verto_new()
 * @see verto_default()
 * @param impl The implementation to use.
 * @return The default EvCtx, or NULL on error.  Call verto_free() when done.
 */
int
verto_set_default(const char *impl);

/**
 * Frees a vertoEvCtx.
 *
 * When called on a default vertoEvCtx, the reference will be freed but the
 * internal default loop will still be available via another call to
 * verto_default().
 *
 * @see verto_new()
 * @see verto_default()
 * @param ctx The vertoEvCtx to free.
 */
void
verto_free(vertoEvCtx *ctx);

/**
 * Run the vertoEvCtx forever, or at least until verto_break() is called.
 *
 * @see verto_break()
 * @param ctx The vertoEvCtx to run.
 */
void
verto_run(vertoEvCtx *ctx);

/**
 * Run the vertoEvCtx once. May block.
 *
 * @param ctx The vertoEvCtx to run once.
 */
void
verto_run_once(vertoEvCtx *ctx);

/**
 * Exits the currently running vertoEvCtx.
 *
 * @see verto_run()
 * @param ctx The vertoEvCtx to exit.
 */
void
verto_break(vertoEvCtx *ctx);

/**
 * Adds a callback executed when a file descriptor is ready to be read/written.
 *
 * All vertoEv events are automatically freed when their parent vertoEvCtx is
 * freed. You do not need to free them manually. If VERTO_EV_FLAG_PERSIST is
 * provided, the event will repeat until verto_del() is called. If
 * VERTO_EV_FLAG_PERSIST is not provided, the event will be freed automatically
 * after its execution. In either case, you may call verto_del() at any time
 * to prevent the event from executing.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param flags The flags to set (at least one VERTO_EV_FLAG_IO* required).
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param fd The file descriptor to watch for reads.
 * @return The vertoEv registered with the event context or NULL on error.
 */
vertoEv *
verto_add_io(vertoEvCtx *ctx, vertoEvFlag flags,
             vertoCallback *callback, void *priv, int fd);

/**
 * Adds a callback executed after a period of time.
 *
 * All vertoEv events are automatically freed when their parent vertoEvCtx is
 * freed. You do not need to free them manually. If VERTO_EV_FLAG_PERSIST is
 * provided, the event will repeat until verto_del() is called. If
 * VERTO_EV_FLAG_PERSIST is not provided, the event will be freed automatically
 * after its execution. In either case, you may call verto_del() at any time
 * to prevent the event from executing.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param flags The flags to set.
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param interval Time period to wait before firing (in milliseconds).
 * @return The vertoEv registered with the event context.
 */
vertoEv *
verto_add_timeout(vertoEvCtx *ctx, vertoEvFlag flags,
                  vertoCallback *callback, void *priv, time_t interval);

/**
 * Adds a callback executed when there is nothing else to do.
 *
 * All vertoEv events are automatically freed when their parent vertoEvCtx is
 * freed. You do not need to free them manually. If VERTO_EV_FLAG_PERSIST is
 * provided, the event will repeat until verto_del() is called. If
 * VERTO_EV_FLAG_PERSIST is not provided, the event will be freed automatically
 * after its execution. In either case, you may call verto_del() at any time
 * to prevent the event from executing.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param flags The flags to set.
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @return The vertoEv registered with the event context.
 */
vertoEv *
verto_add_idle(vertoEvCtx *ctx, vertoEvFlag flags,
               vertoCallback *callback, void *priv);

/**
 * Adds a callback executed when a signal is received.
 *
 * All vertoEv events are automatically freed when their parent vertoEvCtx is
 * freed. You do not need to free them manually. If VERTO_EV_FLAG_PERSIST is
 * provided, the event will repeat until verto_del() is called. If
 * VERTO_EV_FLAG_PERSIST is not provided, the event will be freed automatically
 * after its execution. In either case, you may call verto_del() at any time
 * to prevent the event from executing.
 *
 * NOTE: SIGCHLD is expressly not supported. If you want this notification,
 * please use verto_add_child().
 *
 * WARNNIG: Signal events can only be reliably received in the default EvCtx
 * in some implementations.  Attempting to receive signal events in non-default
 * loops may result in assert() failures.
 *
 * WARNING: While verto does its best to protect you from crashes, there is
 * essentially no way to do signal events if you mix multiple implementations in
 * a single process. Attempting to do so will result in undefined behavior,
 * and potentially even a crash. You have been warned.
 *
 * @see verto_add_child()
 * @see verto_repeat()
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param flags The flags to set.
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param signal The signal to watch for.
 * @return The vertoEv registered with the event context.
 */
vertoEv *
verto_add_signal(vertoEvCtx *ctx, vertoEvFlag flags,
                 vertoCallback *callback, void *priv, int signal);

/**
 * Adds a callback executed when a child process exits.
 *
 * This event will be freed automatically after its execution. Due to the
 * nature of a process' life-cycle, child events cannot persist (processes only
 * exit once). This function returns NULL if you attempt to use
 * VERTO_EV_FLAG_PERSIST. You may, of course, call verto_del() at any time to
 * prevent the callback from firing.
 *
 * @see verto_del()
 * @param ctx The vertoEvCtx which will fire the callback.
 * @param flags The flags to set.
 * @param callback The callback to fire.
 * @param priv Data which will be passed to the callback.
 * @param child The pid of the child to watch for.
 * @return The vertoEv registered with the event context.
 */
vertoEv *
verto_add_child(vertoEvCtx *ctx, vertoEvFlag flags,
                vertoCallback *callback, void *priv, pid_t pid);

/**
 * Gets the private pointer of the vertoEv.
 *
 * @see verto_add_io()
 * @see verto_add_timeout()
 * @see verto_add_idle()
 * @see verto_add_signal()
 * @see verto_add_child()
 * @param ev The vertoEv
 * @return The vertoEv private pointer
 */
void *
verto_get_private(const vertoEv *ev);

/**
 * Gets the type of the vertoEv.
 *
 * @see verto_add_io()
 * @see verto_add_timeout()
 * @see verto_add_idle()
 * @see verto_add_signal()
 * @see verto_add_child()
 * @param ev The vertoEv
 * @return The vertoEv type
 */
vertoEvType
verto_get_type(const vertoEv *ev);

/**
 * Gets the flags associated with the given vertoEv.
 *
 * @see verto_add_io()
 * @see verto_add_timeout()
 * @see verto_add_idle()
 * @see verto_add_signal()
 * @see verto_add_child()
 * @param ev The vertoEv
 * @return The vertoEv type
 */
vertoEvFlag
verto_get_flags(const vertoEv *ev);

/**
 * Gets the file descriptor associated with a read/write vertoEv.
 *
 * @see verto_add_io()
 * @param ev The vertoEv to retrieve the file descriptor from.
 * @return The file descriptor, or -1 if not a read/write event.
 */
int
verto_get_fd(const vertoEv *ev);

/**
 * Gets the interval associated with a timeout vertoEv.
 *
 * @see verto_add_timeout()
 * @param ev The vertoEv to retrieve the interval from.
 * @return The interval, or 0 if not a timeout event.
 */
time_t
verto_get_interval(const vertoEv *ev);

/**
 * Gets the signal associated with a signal vertoEv.
 *
 * @see verto_add_signal()
 * @param ev The vertoEv to retrieve the signal from.
 * @return The signal, or -1 if not a signal event.
 */
int
verto_get_signal(const vertoEv *ev);

/**
 * Gets the pid associated with a child vertoEv.
 *
 * @see verto_add_child()
 * @param ev The vertoEv to retrieve the file descriptor from.
 * @return The pid, or 0 if not a child event.
 */
pid_t
verto_get_pid(const vertoEv *ev);

/**
 * Gets the status of the pid which caused this event to fire.
 *
 * @see verto_add_child()
 * @param ev The vertoEv to retrieve the status from.
 * @return The pid status.
 */
int
verto_get_pid_status(const vertoEv *ev);

/**
 * Removes an event from from the event context and frees it.
 *
 * The event and its contents cannot be used after this call.
 *
 * @see verto_add_io()
 * @see verto_add_timeout()
 * @see verto_add_idle()
 * @see verto_add_signal()
 * @see verto_add_child()
 * @param ev The event to delete.
 */
void
verto_del(vertoEv *ev);

#endif /* VERTO_H_ */
