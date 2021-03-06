#include <stdarg.h>

#include "pub/type.h"
#include "pub/const.h"
#include "pub/vm.h"
#include "pub/inlines.h"

#include "vm/obj.h"
#include "vm/num.h"
#include "vm/strobj.h"
#include "vm/func.h"
#include "vm/listobj.h"

#include "native.h"

/*
	rules:                                                                      output type
		1. 'n': check num type and get the value                             -- ivm_number_t *
		2. 'u': check num type and cast the value to uint32                  -- ivm_uint32_t *
		3. 'd': check num type and cast the value to sint32                  -- ivm_sint32_t *
		4. 's': check string type and get the value                          -- const ivm_string_t **
		5. 'r': check string type and get raw string                         -- const ivm_char_t *
		6. 'l': check list type and convert to list object                   -- ivm_list_object_t **
		7. 'f': check function type and convert to function object           -- ivm_function_object_t **
		8. 'b': check buffer type and convert to buffer object               -- ivm_buffer_object_t **
		9. 'c': check coro type and get the value                            -- ivm_coro_t **
	   10. '.': no type check but accept the object                          -- ivm_object_t **
	   11. '*' prefix: reverse the optional mark                             -- \

		NOTE:
			[1]. return address passed to an optional argument need to be initialized
			[2]. optional mark could only appear once
 */

ivm_bool_t
ivm_native_matchArgument(ivm_function_arg_t arg,
						 ivm_vmstate_t *state,
						 const ivm_char_t *func_name,
						 const ivm_char_t *rule, ...)
{
	ivm_bool_t opt = IVM_FALSE;
	ivm_argc_t i;
	ivm_object_t *tmp;
	va_list args;
	ivm_bool_t ret = IVM_TRUE;

	va_start(args, rule);

#define SET_ERR(...) \
	{                                             \
		ivm_char_t msg_buf[                       \
			IVM_DEFAULT_EXCEPTION_BUFFER_SIZE     \
		];                                        \
		ivm_object_t *exc;                        \
		IVM_SNPRINTF(                             \
			msg_buf, IVM_ARRLEN(msg_buf),         \
			__VA_ARGS__                           \
		);                                        \
		exc = ivm_exception_new(                  \
			(state), msg_buf, func_name, 0        \
		);                                        \
		ivm_vmstate_setException((state), exc);   \
		ret = IVM_FALSE;                          \
	}

#define SUB1(r, type, cvt, val) \
	case r:                                                        \
		if (ivm_function_arg_has(arg, i)) {                        \
			tmp = ivm_function_arg_at(arg, i);                     \
			if (IVM_IS_BTTYPE(tmp, state, (type))) {               \
				*((cvt *)va_arg(args, cvt *))                      \
				= (val);                                           \
			} else {                                               \
				if (!(opt && IVM_IS_NONE(state, tmp))) {           \
					SET_ERR(IVM_ERROR_MSG_WRONG_ARG(               \
						i, ivm_vmstate_getTypeName(state, (type)), \
						IVM_OBJECT_GET(tmp, TYPE_NAME)             \
					));                                            \
					goto END;                                      \
				} else {                                           \
					va_arg(args, cvt *);                           \
				}                                                  \
			}                                                      \
			i++;                                                   \
		} else {                                                   \
			if (!opt) {                                            \
				SET_ERR(IVM_ERROR_MSG_MISSING_ARG(                 \
					i, ivm_vmstate_getTypeName(state, (type))      \
				));                                                \
			}                                                      \
			goto END;                                              \
		}                                                          \
		break;

	for (i = 1; *rule; rule++) {
		switch (*rule) {
			SUB1('n', IVM_NUMERIC_T, ivm_number_t, ivm_numeric_getValue(tmp))
			SUB1('u', IVM_NUMERIC_T, ivm_uint32_t, ivm_numeric_getUInt32(tmp))
			SUB1('d', IVM_NUMERIC_T, ivm_sint32_t, ivm_numeric_getSInt32(tmp))
			SUB1('s', IVM_STRING_OBJECT_T, const ivm_string_t *, ivm_string_object_getValue(tmp))
			SUB1('r', IVM_STRING_OBJECT_T, const ivm_char_t *, ivm_string_trimHead(ivm_string_object_getValue(tmp)))
			SUB1('l', IVM_LIST_OBJECT_T, ivm_list_object_t *, IVM_AS(tmp, ivm_list_object_t))
			SUB1('f', IVM_FUNCTION_OBJECT_T, ivm_function_object_t *, IVM_AS(tmp, ivm_function_object_t))
			SUB1('b', IVM_BUFFER_OBJECT_T, ivm_buffer_object_t *, IVM_AS(tmp, ivm_buffer_object_t))
			SUB1('c', IVM_CORO_OBJECT_T, ivm_coro_t *, ivm_coro_object_getCoro(tmp))

			case '.':
				if (ivm_function_arg_has(arg, i)) {
					*(va_arg(args, ivm_object_t **)) = ivm_function_arg_at(arg, i);
					i++;
				} else {
					if (!opt) {
						SET_ERR(IVM_ERROR_MSG_MISSING_ARG(i, "any type"));
					}

					goto END;
				}
				break;

			case '*':
				// IVM_ASSERT(!opt, IVM_ERROR_MSG_REPEAT_OPTIONAL_MARK);
				opt = !opt;
				break;

			default:
				IVM_FATAL(IVM_ERROR_MSG_WRONG_NATIVE_ARG_RULE(*rule));
		}
	}

#undef SUB1

END:
	va_end(args);

	return ret;
}

ivm_object_t *
IVM_NATIVE_WRAP_CONS_c(ivm_vmstate_t *state,
					   ivm_object_t *proto,
					   ivm_native_function_t func)
{
	ivm_object_t *ret = ivm_function_object_newNative(state, func);
	ivm_object_setProto(ret, state, proto);

	return ret;
}
