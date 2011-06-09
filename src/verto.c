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

#define _GNU_SOURCE // For dladdr()

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>

#include <verto.h>

struct _vContext {
	size_t    ref;
	void     *module;
	vLoopSpec spec;
};

struct _vLoop {
	size_t        ref;
	vContext     *ctx;
	vLoopPrivate *priv;
};

#define  _str(s) # s
#define __str(s) _str(s)

static vContext *defctx = NULL;

static inline vContext *do_load_file(const char *filename, bool reqsym) {
	void *dll = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
	if (!dll) {
		//printf("%s -- %s\n", filename.c_str(), dlerror());
		return NULL;
	}

	struct _vModule *module = (struct _vModule*) dlsym(dll, __str(_VERTO_MODULE_TABLE));
	if (!module || module->vers != _VERTO_MODULE_VERSION)
		goto error;

	// Check to make sure that we have our required symbol if reqsym == true
	if (module->symb && reqsym) {
		void *tmp = dlopen(NULL, 0);
		if (!tmp || !dlsym(tmp, module->symb)) {
			if (tmp)
				dlclose(tmp);
			goto error;
		}
		dlclose(tmp);
	}

	// Get the context creation function
	vContext *(*makectx)() = (vContext *(*)()) dlsym(dll, module->name);
	if (!makectx)
		goto error;

	// Call it
	vContext *ctx = makectx();
	if (!ctx)
		goto error;

	ctx->module = dll;
	return ctx;

	error:
		dlclose(dll);
		return NULL;
}

static inline vContext *do_load_dir(const char *dirname, const char *prefix, const char *suffix, bool reqsym) {
	vContext *ctx = NULL;
	DIR *dir = opendir(dirname);
	if (!dir)
		return NULL;

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

		char *tmp = (char *) malloc(sizeof(char) * (flen + strlen(dirname) + 2));
		if (!tmp)
			continue;

		strcpy(tmp, dirname);
		strcat(tmp, "/");
		strcat(tmp, ent->d_name);

		ctx = do_load_file(tmp, reqsym);
		free(tmp);
		if (ctx)
			break;
	}

	closedir(dir);
	return ctx;
}

static inline vLoop *make_loop(vContext *ctx, void *priv) {
	if (!ctx)
		return NULL;

	vLoop *loop = malloc(sizeof(vLoop));
	if (!loop) {
		ctx->spec.loop_free(priv);
		return NULL;
	}

	loop->ref = 1;
	loop->ctx = ctx;
	loop->priv = priv;
	return loop;
}

vContext *v_context_new(const vLoopSpec *loopspec) {
	vContext *ctx = malloc(sizeof(vContext));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(vContext));
	ctx->ref = 1;
	ctx->spec = *loopspec; // Copy the spec
	return ctx;
}

vContext *v_context_load(const char *name_or_path) {
	vContext *ctx = NULL;
	Dl_info dlinfo;

	if (!dladdr(v_context_load+1, &dlinfo))
		return NULL;

	char *suffix = strstr(dlinfo.dli_fname, MODSUFFIX);
	if (!suffix)
		return NULL;

	char *prefix = strndup(dlinfo.dli_fname, suffix-dlinfo.dli_fname+1);
	if (!prefix)
		return NULL;
	prefix[strlen(prefix)-1] = '-'; // Ex: /usr/lib/libverto-

	if (name_or_path) {
		// Try to do a load by the path
		ctx = do_load_file(name_or_path, false);
		if (!ctx) {
			// Try to do a load by the name
			char *tmp = malloc(strlen(name_or_path) + strlen(prefix) + strlen(suffix) + 1);
			if (tmp) {
				strcpy(tmp, prefix);
				strcat(tmp, name_or_path);
				strcat(tmp, suffix);
				ctx = do_load_file(tmp, false);
				free(tmp);
			}
		}
	} else {
		// NULL was passed, so we will use the dirname of
		// the prefix to try and find any possible plugins
		char *tmp = strdup(prefix);
		if (tmp) {
			char *dname = strdup(dirname(tmp));
			free(tmp);

			tmp = strdup(basename(prefix));
			free(prefix);
			prefix = tmp;

			if (dname && prefix) {
				ctx = do_load_dir(dname, prefix, suffix, true);
				if (!ctx)
					ctx = do_load_dir(dname, prefix, suffix, false);
			}

			free(dname);
		}
	}

	free(prefix);
	return ctx;
}

vContext *v_context_incref(vContext *ctx) {
	if (ctx)
		ctx->ref++;
	return ctx;
}

void v_context_decref(vContext *ctx) {
	if (!ctx || --ctx->ref)
		return;

	if (ctx->module)
		dlclose(ctx->module);
	free(ctx);
	return;
}

vContext *v_context_default_get(void) {
	return defctx;
}

void      v_context_default_set(vContext *ctx) {
	v_context_decref(defctx);
	defctx = v_context_incref(ctx);
}

vLoop *v_loop_new(vContext *ctx) {
	return make_loop(ctx, ctx ? ctx->spec.loop_new() : NULL);
}

vLoop *v_loop_default(vContext *ctx) {
	return make_loop(ctx, ctx ? ctx->spec.loop_default() : NULL);
}

vLoop *v_loop_convert(vContext *ctx, ...) {
	va_list args;
	va_start(args, ctx);
	vLoop *loop = make_loop(ctx, ctx ? ctx->spec.loop_convert(args) : NULL);
	va_end(args);
	return loop;
}

vLoop *v_loop_incref(vLoop *loop) {
	if (loop)
		loop->ref++;
	return loop;
}

void v_loop_decref(vLoop *loop) {
	if (!loop || --loop->ref)
		return;

	vContext *ctx = loop->ctx;
	loop->ctx->spec.loop_free(loop->priv);
	v_context_decref(ctx);
	return;
}

void v_loop_run(vLoop *loop) {
	if (!loop)
		return;
	loop->ctx->spec.loop_run(loop->priv);
}

void v_loop_run_once(vLoop *loop) {
	if (!loop)
		return;
	loop->ctx->spec.loop_run_once(loop->priv);
}

void v_loop_break(vLoop *loop) {
	if (!loop)
		return;
	loop->ctx->spec.loop_break(loop->priv);
}

vHook *v_loop_hook_add(vLoop *loop, vCallback callback, void *data, vHookType type, long long spec, vHookPriority priority) {
	if (!loop)
		return 0;
	return loop->ctx->spec.loop_hook_add(loop->priv, loop, callback, data, type, spec, priority);
}

void v_loop_hook_del(vLoop *loop, vHook *hook) {
	if (!loop)
		return;
	loop->ctx->spec.loop_hook_del(loop->priv, hook);
}
