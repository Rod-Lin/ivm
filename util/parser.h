#ifndef _IVM_UTIL_PARSER_H_
#define _IVM_UTIL_PARSER_H_

#include "pub/com.h"
#include "pub/type.h"
#include "pub/vm.h"

#include "std/list.h"

IVM_COM_HEADER

ivm_list_t *
_ivm_parser_getTokens(const ivm_char_t *src);

ivm_double_t
_ivm_parser_parseNum(const ivm_char_t *src,
					 ivm_size_t len,
					 ivm_bool_t *err);

ivm_char_t *
_ivm_parser_parseStr(const ivm_char_t *str,
					 ivm_size_t len);

ivm_exec_t *
ivm_parser_tokenToExec(ivm_list_t *tokens);

IVM_COM_END

#endif

#if 0
opcode -> t_identifier

instr -> opcode arg1[, arg2[, ...]] '\n'
#endif