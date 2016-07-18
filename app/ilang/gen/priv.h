#ifndef _IVM_APP_ILANG_GEN_PRIV_H_
#define _IVM_APP_ILANG_GEN_PRIV_H_

#include "pub/type.h"
#include "pub/com.h"
#include "pub/vm.h"
#include "pub/err.h"

#include "util/parser.h"

#include "gen.h"

#define FLAG ilang_gen_flag_build
#define CHECK_SE() ((ilang_gen_check_flag_t) { .has_side_effect = IVM_TRUE })
#define RETVAL ilang_gen_value_build
#define NORET() ((ilang_gen_value_t) { 0 })

#define GEN_ERR(p, ...) \
	IVM_TRACE("ilang generator error: at line %zd pos %zd: ", (p).line, (p).pos); \
	IVM_TRACE(__VA_ARGS__); \
	IVM_TRACE("\n"); \
	IVM_EXIT(1);

#define GEN_WARN(p, ...) \
	IVM_TRACE("ilang generator warning: at line %zd pos %zd: ", (p).line, (p).pos); \
	IVM_TRACE(__VA_ARGS__); \
	IVM_TRACE("\n");

#define GEN_ERR_MSG_CANNOT_ASSIGN_TO(expr_name)						"cannot assign to %s", (expr_name)
#define GEN_ERR_MSG_NESTED_RET										"nested return expression"
#define GEN_ERR_MSG_FAILED_PARSE_NUM(val, len)						"failed parse num '%.*s'", (int)(len), (val)
#define GEN_ERR_MSG_UNSUPPORTED_UNARY_OP(type)						"unsupported unary operation type %d", (type)
#define GEN_ERR_MSG_UNSUPPORTED_BINARY_OP(type)						"unsupported binary operation type %d", (type)
#define GEN_ERR_MSG_UNSUPPORTED_CMP_TYPE(type)						"unsupported compare type type %d", (type)
#define GEN_ERR_MSG_BREAK_OR_CONT_OUTSIDE_LOOP						"using break/cont outside a loop"
#define GEN_ERR_MSG_BREAK_OR_CONT_IGNORE_ARG						"ignore break/cont argument"

#define GEN_ERR_GENERAL(expr, ...) \
	GEN_ERR((expr)->pos, __VA_ARGS__)

#define GEN_WARN_GENERAL(expr, ...) \
	GEN_WARN((expr)->pos, __VA_ARGS__)

#define GEN_ASSERT_NOT_LEFT_VALUE(expr, name, flag) \
	if ((flag).is_left_val) { \
		GEN_ERR((expr)->pos, GEN_ERR_MSG_CANNOT_ASSIGN_TO(name)); \
	}

#define GEN_WARN_NO_NESTED_RET(expr, flag) \
	if (!(flag).is_top_level) { \
		GEN_WARN((expr)->pos, GEN_ERR_MSG_NESTED_RET); \
	}

#endif