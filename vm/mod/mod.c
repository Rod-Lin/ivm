#include <stdlib.h>

#include "pub/const.h"
#include "pub/type.h"
#include "pub/com.h"
#include "pub/err.h"
#include "pub/inlines.h"
#include "pub/vm.h"

#include "std/mem.h"
#include "std/string.h"
#include "std/io.h"
#include "std/env.h"
#include "std/sys.h"
#include "std/list.h"
#include "std/path.h"

#include "mod.h"
#include "dll.h"

typedef ivm_ptlist_t ivm_rawstr_list_t;
typedef IVM_PTLIST_ITER_TYPE(ivm_char_t *) ivm_rawstr_list_iteartor_t;

#define ivm_rawstr_list_init(list) ivm_ptlist_init_c((list), IVM_DEFAULT_RAWSTR_LIST_BUFFER_SIZE)
#define ivm_rawstr_list_dump ivm_ptlist_dump
#define ivm_rawstr_list_push ivm_ptlist_push
#define ivm_rawstr_list_pop(list) ((ivm_char_t *)ivm_ptlist_pop(list))

#define IVM_RAWSTR_LIST_ITER_GET(iter) IVM_PTLIST_ITER_GET(iter)
#define IVM_RAWSTR_LIST_EACHPTR(list, iter) IVM_PTLIST_EACHPTR((list), iter, ivm_char_t *)
#define IVM_RAWSTR_LIST_EACHPTR_R(list, iter) IVM_PTLIST_EACHPTR_R((list), iter, ivm_char_t *)

typedef ivm_list_t ivm_dll_list_t;
typedef IVM_LIST_ITER_TYPE(ivm_dll_t) ivm_dll_list_iteartor_t;

#define ivm_dll_list_init(list) ivm_list_init_c((list), sizeof(ivm_dll_t), IVM_DEFAULT_DLL_LIST_BUFFER_SIZE)
#define ivm_dll_list_dump(list) ivm_list_dump(list)
#define ivm_dll_list_push(list, val) ivm_list_push((list), (val))

#define IVM_DLL_LIST_ITER_GET(iter) IVM_LIST_ITER_GET(iter, ivm_dll_t)
#define IVM_DLL_LIST_EACHPTR(list, iter) IVM_LIST_EACHPTR((list), iter, ivm_dll_t)

typedef struct {
	const ivm_char_t *suf;
	ivm_size_t len;
	ivm_mod_loader_t loader;
} ivm_mod_suffix_t;

typedef ivm_list_t ivm_mod_suffix_list_t;
typedef IVM_LIST_ITER_TYPE(ivm_mod_suffix_t) ivm_mod_suffix_list_iteartor_t;

#define ivm_mod_suffix_list_init(list) ivm_list_init_c((list), sizeof(ivm_mod_suffix_t), 2)
#define ivm_mod_suffix_list_dump(list) ivm_list_dump(list)
#define ivm_mod_suffix_list_push(list, val) ivm_list_push((list), (val))

#define IVM_MOD_SUFFIX_LIST_ITER_GET(iter) IVM_LIST_ITER_GET(iter, ivm_mod_suffix_t)
#define IVM_MOD_SUFFIX_LIST_EACHPTR_R(list, iter) IVM_LIST_EACHPTR_R((list), iter, ivm_mod_suffix_t)

IVM_PRIVATE
ivm_rawstr_list_t _mod_path_list;

IVM_PRIVATE
ivm_size_t _mod_path_max_len = 0;

IVM_PRIVATE
ivm_dll_list_t _dll_list;

IVM_PRIVATE
ivm_mod_suffix_list_t _mod_suffix_list;

IVM_PRIVATE
ivm_size_t _mod_suffix_max_len = 0;

IVM_PRIVATE
ivm_object_t *
_ivm_mod_loadNative(const ivm_char_t *path,
					ivm_char_t **err,
					ivm_bool_t *is_const,
					ivm_vmstate_t *state,
					ivm_coro_t *coro,
					ivm_context_t *context);

// TODO: need locks

void
ivm_mod_addModPath(const ivm_char_t *path)
{
	ivm_size_t len = IVM_STRLEN(path);

	if (len > _mod_path_max_len)
		_mod_path_max_len = len;

	ivm_rawstr_list_push(&_mod_path_list, IVM_STRNDUP(path, len));

	return;
}

IVM_PRIVATE
void
_ivm_mod_addModPath_n(ivm_char_t *path)
{
	ivm_size_t len;

	if (path) {
		len = IVM_STRLEN(path);

		if (len > _mod_path_max_len)
			_mod_path_max_len = len;
	}

	ivm_rawstr_list_push(&_mod_path_list, path);

	return;
}

IVM_PRIVATE
void
_ivm_mod_addModPath_l(const ivm_char_t *path,
					  ivm_size_t len)
{
	if (len > _mod_path_max_len)
		_mod_path_max_len = len;

	ivm_rawstr_list_push(&_mod_path_list, IVM_STRNDUP(path, len));

	return;
}

IVM_PRIVATE
void
_ivm_mod_popModPath_n()
{
	ivm_rawstr_list_pop(&_mod_path_list);
	return;
}

void
ivm_mod_addModSuffix(const ivm_char_t *suffix,
					 ivm_mod_loader_t loader)
{
	ivm_size_t len = IVM_STRLEN(suffix);
	ivm_mod_suffix_t suf = {
		suffix, len, loader
	};

	if (len > _mod_suffix_max_len)
		_mod_suffix_max_len = len;

	ivm_mod_suffix_list_push(&_mod_suffix_list, &suf);

	return;
}

IVM_INLINE
void
_ivm_mod_initModPath()
{
	const ivm_char_t *paths = ivm_env_getVar("IVM_MOD_PATH"),
					 *start, *end;
	ivm_size_t len;

#if IVM_DEBUG
	ivm_mod_addModPath(IVM_LIB_PATH);
#endif

	ivm_mod_addModPath(IVM_MOD_DEFAULT_SEARCH_PATH);

	if (paths) {
		for (len = 0, start = end = paths;
			 *end; end++) {
			if (*end == ';') {
				if (len) {
					_ivm_mod_addModPath_l(start, len);
					len = 0;
				}
				start = end + 1;
			} else {
				len++;
			}
		}

		if (len) {
			// last path
			_ivm_mod_addModPath_l(start, len);
		}
	}

	return;
}

ivm_int_t
ivm_mod_init()
{
	ivm_rawstr_list_init(&_mod_path_list);
	ivm_dll_list_init(&_dll_list);
	ivm_mod_suffix_list_init(&_mod_suffix_list);

	ivm_mod_addModSuffix(IVM_DLL_SUFFIX, _ivm_mod_loadNative);

	_ivm_mod_initModPath();

	return 0;
}

void
ivm_mod_clean()
{
	ivm_rawstr_list_iteartor_t piter;
	ivm_dll_list_iteartor_t diter;

	IVM_RAWSTR_LIST_EACHPTR(&_mod_path_list, piter) {
		// IVM_TRACE("mod path: %s\n",IVM_RAWSTR_LIST_ITER_GET(piter));
		STD_FREE(IVM_RAWSTR_LIST_ITER_GET(piter));
	}
	ivm_rawstr_list_dump(&_mod_path_list);

	IVM_DLL_LIST_EACHPTR(&_dll_list, diter) {
		ivm_dll_close(IVM_DLL_LIST_ITER_GET(diter));
	}
	ivm_dll_list_dump(&_dll_list);

	ivm_mod_suffix_list_dump(&_mod_suffix_list);

	return;
}

IVM_INLINE
ivm_size_t
_get_max_buf_size(ivm_size_t mod_name_len)
{
	return _mod_path_max_len + 1 + mod_name_len + 5 + _mod_suffix_max_len + 1;
}

IVM_PRIVATE
struct {
	const ivm_char_t *suf;
	ivm_size_t len;
} _sufs[] = {
#define SUF(val) \
{ (val), sizeof(val) - 1 },

	SUF("")
	SUF(IVM_FILE_SEPARATOR_S "init")

#undef SUF
};

IVM_INLINE
ivm_mod_loader_t
_ivm_mod_searchPath(const ivm_char_t *path,
					const ivm_char_t *mod_name,
					ivm_size_t mod_len,
					ivm_char_t *buf)
{
	ivm_char_t *brk;
	ivm_int_t i;
	ivm_mod_suffix_t tmp_suf;
	ivm_mod_suffix_list_iteartor_t siter;

	if (path) {
		ivm_size_t len = IVM_STRLEN(path);

		STD_MEMCPY(buf, path, len);

		buf[len] = IVM_FILE_SEPARATOR;
		STD_MEMCPY(buf + len + 1, mod_name, mod_len);
		brk = buf + len + 1 + mod_len;
	} else {
		STD_MEMCPY(buf, mod_name, mod_len);
		brk = buf + mod_len;
	}

	for (i = 0; i < IVM_ARRLEN(_sufs); i++) {
		STD_MEMCPY(brk, _sufs[i].suf, _sufs[i].len);

		IVM_MOD_SUFFIX_LIST_EACHPTR_R(&_mod_suffix_list, siter) {
			tmp_suf = IVM_MOD_SUFFIX_LIST_ITER_GET(siter);
			STD_MEMCPY(brk + _sufs[i].len, tmp_suf.suf, tmp_suf.len + 1);

			// IVM_TRACE("mod search for %s %ld %s\n", buf, _mod_suffix_max_len, tmp_suf.suf);
			if (ivm_file_access(buf, IVM_FMODE_READ_BINARY)) {
				return tmp_suf.loader;
			}
		}
	}

	return IVM_NULL;
}

/* buffer guaranteed to have enough size */
IVM_INLINE
ivm_mod_loader_t
_ivm_mod_search_c(const ivm_char_t *mod_name,
				  ivm_size_t mod_len,
				  ivm_char_t *buf)
{
	ivm_rawstr_list_iteartor_t iter;
	const ivm_char_t *tmp;
	ivm_mod_loader_t ret;

	/*
		max situation:

		len1          1              len2        5         ?      1
		tmp + IVM_FILE_SEPARATOR + mod_name + "/init" + ".suf" + "\0"
	 */
	// ivm_char_t buf[_get_max_buf_size(len2)];

	// search in relative path
	ret = _ivm_mod_searchPath(IVM_NULL, mod_name, mod_len, buf);
	if (ret) return ret;

	IVM_RAWSTR_LIST_EACHPTR_R(&_mod_path_list, iter) {
		tmp = IVM_RAWSTR_LIST_ITER_GET(iter);

		if (!tmp) continue;

		ret = _ivm_mod_searchPath(tmp, mod_name, mod_len, buf);
		if (ret) return ret;
	}

	return IVM_NULL;
}

ivm_mod_loader_t
ivm_mod_search(const ivm_char_t *mod_name,
			   ivm_char_t *path_buf,
			   ivm_size_t buf_size)
{
	/*
		max situation:

		len1          1              len2        5         ?      1
		tmp + IVM_FILE_SEPARATOR + mod_name + "/init" + ".suf" + "\0"
	 */
	ivm_size_t mod_len = IVM_STRLEN(mod_name), len;
	ivm_char_t buf[_get_max_buf_size(mod_len)];
	ivm_mod_loader_t ret;

	ret = _ivm_mod_search_c(mod_name, mod_len, buf);

	if (ret) {
		len = IVM_STRLEN(buf) + 1;

		len = buf_size >= len ? len : buf_size;
		STD_MEMCPY(path_buf, buf, len);
		path_buf[len - 1] = '\0'; /* in case buf_size is smaller than string length */
	}

	return ret;
}

IVM_PRIVATE
ivm_object_t *
_ivm_mod_loadNative(const ivm_char_t *path,
					ivm_char_t **err,
					ivm_bool_t *is_const,
					ivm_vmstate_t *state,
					ivm_coro_t *coro,
					ivm_context_t *context)
{
	ivm_char_t *tmp_err = IVM_NULL;
	ivm_dll_t handler;
	ivm_mod_native_init_t init;
	ivm_object_t *ret = IVM_NULL;

	*is_const = IVM_TRUE;

	if (!ivm_dll_open(&handler, path)) {
		tmp_err = ivm_dll_error(handler, is_const);

		if (!tmp_err) {
			tmp_err = IVM_ERROR_MSG_UNKNOWN_ERROR;
		}

		goto END;
	}

	init = ivm_dll_getFunc(handler, IVM_MOD_NATIVE_INIT_FUNC, ivm_mod_native_init_t);

	if (!init) {
		ivm_dll_close(handler);

		tmp_err = ivm_dll_error(handler, is_const);

		if (!tmp_err) {
			tmp_err = IVM_ERROR_MSG_FAILED_LOAD_INIT_FUNC;
		}

		goto END;
	}

	ivm_dll_list_push(&_dll_list, &handler);

	ret = init(state, coro, context);

END:

	*err = tmp_err;

	return ret;
}

ivm_object_t *
ivm_mod_load(const ivm_string_t *mod_name,
			 ivm_vmstate_t *state,
			 ivm_coro_t *coro,
			 ivm_context_t *context)
{
	ivm_mod_loader_t loader;
	ivm_size_t len = ivm_string_length(mod_name);

	const ivm_char_t *mod = ivm_string_trimHead(mod_name);
	ivm_char_t *err = IVM_NULL;
	ivm_char_t *path;
	ivm_bool_t is_const, res;

	const ivm_string_t *path_backup, *rpath;

	ivm_object_t *ret;

	ivm_size_t buf_size = _get_max_buf_size(len);

	if (buf_size < IVM_PATH_MAX_LEN + 1) {
		buf_size = IVM_PATH_MAX_LEN + 1;
	}

	ivm_char_t buf[buf_size];

	path_backup = ivm_vmstate_getCurPath(state);

	// IVM_TRACE("cur path: %s\n", path_backup);

	// _ivm_mod_addModPath_n((ivm_char_t *)ivm_string_trimHead(path_backup));
	loader = _ivm_mod_search_c(mod, len, buf);
	// _ivm_mod_popModPath_n();

	IVM_CORO_NATIVE_ASSERT(coro, state, loader, IVM_ERROR_MSG_MOD_NOT_FOUND(mod));
	
	// avoid circular reference
	IVM_CORO_NATIVE_ASSERT(coro, state, ivm_path_realpath(buf, buf), IVM_ERROR_MSG_FAILED_GET_ABS_PATH(buf));

	rpath = ivm_vmstate_constantize_r(state, buf);

	ret = ivm_object_getSlot(ivm_vmstate_getLoadedMod(state), state, rpath);
	if (ret) return ret;

	path = ivm_sys_getBasePath(buf);
	res = ivm_vmstate_setCurPath(state, path);
	STD_FREE(path);

	if (!res) return IVM_NULL;

	ivm_object_setSlot(ivm_vmstate_getLoadedMod(state), state, rpath, IVM_NONE(state));
	
	ret = loader(buf, &err, &is_const, state, coro, context);

	ivm_object_setSlot(ivm_vmstate_getLoadedMod(state), state, rpath, ret);
	res = ivm_vmstate_setCurPath_c(state, path_backup);

	if (!res) return IVM_NULL;

	if (!ret) {
		if (!ivm_vmstate_getException(state)) {
			IVM_CORO_NATIVE_FATAL_C(
				coro, state,
				IVM_ERROR_MSG_MOD_LOAD_ERROR(mod, buf, err)
			);
		}
	}

	if (!is_const) {
		ivm_dll_freeError(err);
	}

	return ret;
}
