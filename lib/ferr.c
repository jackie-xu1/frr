/*
 * Copyright (c) 2015-16  David Lamparter, for NetDEF, Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "ferr.h"
#include "vty.h"
#include "jhash.h"
#include "memory.h"

DEFINE_MTYPE_STATIC(LIB, ERRINFO, "error information")

struct log_cat _lc_ROOT __attribute__((section(".data.logcats"))) = {
	.name = "ROOT"
};

DEFINE_LOGCAT(OK,                ROOT, "No error")
DEFINE_LOGCAT(CODE_BUG,          ROOT, "Code bug / internal inconsistency")
DEFINE_LOGCAT(CONFIG_INVALID,    ROOT, "Invalid configuration")
DEFINE_LOGCAT(CONFIG_REALITY,    ROOT, "Configuration mismatch against operational state")
DEFINE_LOGCAT(RESOURCE,          ROOT, "Out of resource/memory")
DEFINE_LOGCAT(SYSTEM,            ROOT, "System error")
DEFINE_LOGCAT(LIBRARY,           ROOT, "External library error")
DEFINE_LOGCAT(NET_INVALID_INPUT, ROOT, "Invalid input from network")
DEFINE_LOGCAT(SYS_INVALID_INPUT, ROOT, "Invalid local/system input")

struct log_ref_block *log_ref_blocks = NULL;
struct log_ref_block **log_ref_block_last = &log_ref_blocks;

void log_ref_block_add(struct log_ref_block *block)
{
	struct log_ref * const *lrp;
	static const char _lrid[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

	*log_ref_block_last = block;
	log_ref_block_last = &block->next;

	for (lrp = block->start; lrp < block->stop; lrp++) {
		struct log_ref *lr = *lrp;
		char *p;
		uint32_t id;

		if (!lr)
			continue;

		id = jhash(lr->fmtstring, strlen(lr->fmtstring),
			 jhash(lr->file, strlen(lr->file), 0xd4ed0298));
		lr->unique_id = id;

		p = lr->prefix + 8;
		*--p = '\0';
		*--p = _lrid[id & 0x1f]; id >>= 5;
		*--p = _lrid[id & 0x1f]; id >>= 5;
		*--p = _lrid[id & 0x1f]; id >>= 5;
		*--p = _lrid[id & 0x1f]; id >>= 5;
		*--p = _lrid[id & 0x1f]; id >>= 5;
		*--p = _lrid[id & 0x1f]; id >>= 5;
		*--p = _lrid[(id & 0x3) | 0x10 |
			((__builtin_popcount(lr->unique_id) << 2) & 0x0c)];
	}
}

/*
 * Thread-specific key for temporary storage of allocated ferr.
 */
static pthread_key_t errkey;

static void ferr_free(void *arg)
{
	XFREE(MTYPE_ERRINFO, arg);
}

static void err_key_init(void) __attribute__((_CONSTRUCTOR(500)));
static void err_key_init(void)
{
	pthread_key_create(&errkey, ferr_free);
}

static void err_key_fini(void) __attribute__((_DESTRUCTOR(500)));
static void err_key_fini(void)
{
	pthread_key_delete(errkey);
}

const struct ferr *ferr_get_last(ferr_r errval)
{
	struct ferr *last_error = pthread_getspecific(errkey);
	if (!last_error || !last_error->ref)
		return NULL;
	return last_error;
}

ferr_r ferr_clear(void)
{
	struct ferr *last_error = pthread_getspecific(errkey);
	if (last_error)
		last_error->ref = NULL;
	return ferr_ok();
}

static ferr_r ferr_set_va(struct log_ref *ref, const char *pathname,
			  int errno_val, const char *text, va_list va)
{
	struct ferr *error = pthread_getspecific(errkey);

	if (!error) {
		error = XCALLOC(MTYPE_ERRINFO, sizeof(*error));

		pthread_setspecific(errkey, error);
	}

	error->ref = ref;

	error->errno_val = errno_val;
	if (pathname)
		snprintf(error->pathname, sizeof(error->pathname), "%s",
			 pathname);
	else
		error->pathname[0] = '\0';

	vsnprintf(error->message, sizeof(error->message), text, va);
	return -1;
}

ferr_r ferr_set_internal(struct log_ref *ref, ...)
{
	ferr_r rv;
	va_list va;
	va_start(va, ref);
	rv = ferr_set_va(ref, NULL, 0, ref->fmtstring, va);
	va_end(va);
	return rv;
}

ferr_r ferr_set_internal_ext(struct log_ref *ref, const char *pathname,
			     int errno_val, ...)
{
	ferr_r rv;
	va_list va;
	va_start(va, errno_val);
	rv = ferr_set_va(ref, pathname, errno_val, ref->fmtstring, va);
	va_end(va);
	return rv;
}

#define REPLACE "$ERR"
void vty_print_error(struct vty *vty, ferr_r err, const char *msg, ...)
{
	char tmpmsg[512], *replacepos;
	const struct ferr *last_error = ferr_get_last(err);

	va_list va;
	va_start(va, msg);
	vsnprintf(tmpmsg, sizeof(tmpmsg), msg, va);
	va_end(va);

	replacepos = strstr(tmpmsg, REPLACE);
	if (!replacepos)
		vty_out(vty, "%s\n", tmpmsg);
	else {
		replacepos[0] = '\0';
		replacepos += sizeof(REPLACE) - 1;
		vty_out(vty, "%s%s%s\n", tmpmsg,
			last_error ? last_error->message : "(no error?)",
			replacepos);
	}
}
