#ifndef _IVM_APP_ILANG_GEN_GEN_H_
#define _IVM_APP_ILANG_GEN_GEN_H_

#include <setjmp.h>

#include "pub/com.h"
#include "pub/const.h"
#include "pub/type.h"
#include "pub/vm.h"
#include "pub/err.h"

#include "std/list.h"
#include "std/heap.h"

IVM_COM_HEADER

typedef struct {
	ivm_size_t line;
	ivm_size_t pos;
} ilang_gen_pos_t;

#define ilang_gen_pos_build(line, pos) ((ilang_gen_pos_t) { (line), (pos) })

typedef struct {
	const ivm_char_t *val;
	ivm_size_t len;
} ilang_gen_token_value_t;

#define ilang_gen_token_value_isEmpty(t) (!(t).val)
#define ilang_gen_token_value_build(val, len) ((ilang_gen_token_value_t) { (val), (len) })

typedef ivm_list_t ilang_gen_addr_list_t;
typedef IVM_LIST_ITER_TYPE(ivm_size_t) ilang_gen_addr_list_iterator_t;

typedef struct{
	ivm_size_t continue_addr;
	ilang_gen_addr_list_t *break_ref;
	ilang_gen_addr_list_t *end_ref; // instrs that jump to branch end(if/while)
	ilang_gen_addr_list_t *begin_ref; // instrs that jump to branch body(if/while)
	ivm_uint_t nl_block; // non-loop block
} ilang_gen_addr_set_t;

#define ilang_gen_addr_set_build(...) \
	((ilang_gen_addr_set_t) { __VA_ARGS__ })

#define ilang_gen_addr_set_init() \
	((ilang_gen_addr_set_t) { .continue_addr = -1, .nl_block = 0 })

struct ilang_gen_trans_unit_t_tag;
typedef struct {
	const ivm_char_t *file;

	ivm_string_pool_t *str_pool;
	ivm_exec_unit_t *unit;
	ivm_exec_t *cur_exec;
	jmp_buf err_handle;

	ilang_gen_addr_set_t addr;
	ivm_size_t sp;

	ivm_ptlist_t *list_log;
	ivm_heap_t *heap;

	struct ilang_gen_trans_unit_t_tag *tunit;
} ilang_gen_env_t;

void
ilang_gen_env_init(ilang_gen_env_t *env,
				   const ivm_char_t *file,
				   ivm_string_pool_t *str_pool,
				   ivm_exec_unit_t *unit,
				   ivm_exec_t *cur_exec,
				   struct ilang_gen_trans_unit_t_tag *tunit);

IVM_INLINE
void
ilang_gen_env_dump(ilang_gen_env_t *env)
{
	IVM_PTLIST_ITER_TYPE(ivm_list_t *) iter;

	IVM_PTLIST_EACHPTR(env->list_log, iter, ivm_list_t *) {
		ivm_list_free(IVM_PTLIST_ITER_GET(iter));
	}

	ivm_ptlist_free(env->list_log);
	ivm_heap_free(env->heap);

	return;
}

IVM_INLINE
ilang_gen_addr_list_t *
ilang_gen_addr_list_new(ilang_gen_env_t *env)
{
	ilang_gen_addr_list_t *ret = ivm_list_new(sizeof(ivm_size_t));

	ivm_ptlist_push(env->list_log, ret);

	return ret;
}

IVM_INLINE
ivm_size_t
ilang_gen_addr_list_push(ilang_gen_addr_list_t *list,
						 ivm_size_t addr)
{
	return ivm_list_push(list, &addr);
}

#define ilang_gen_addr_list_empty ivm_list_empty

#define ILANG_GEN_ADDR_LIST_ITER_SET(iter, val) IVM_LIST_ITER_SET((iter), (val), ivm_size_t)
#define ILANG_GEN_ADDR_LIST_ITER_GET(iter) IVM_LIST_ITER_GET((iter), ivm_size_t)
#define ILANG_GEN_ADDR_LIST_ITER_GET_PTR(iter) IVM_LIST_ITER_GET_PTR((iter), ivm_size_t)
#define ILANG_GEN_ADDR_LIST_EACHPTR(list, iter) IVM_LIST_EACHPTR((list), iter, ivm_size_t)
#define ILANG_GEN_ADDR_LIST_EACHPTR_R(list, iter) IVM_LIST_EACHPTR_R((list), iter, ivm_size_t)

typedef struct {
	ivm_bool_t is_left_val;
	ivm_bool_t is_top_level; // true: don't leave anything on the stack
	ivm_bool_t if_use_cond_reg; // use (virtual)register to in condition expression
	ivm_bool_t is_callee; // return base if possible
	ivm_bool_t is_slot_expr;
	ivm_bool_t has_branch; // whether the parent expr has branch structure(if/while)
	ivm_bool_t is_arg;
	ivm_int_t pa_argno;
	ivm_int_t varg_offset; // real offset + 1
	ivm_bool_t varg_enable; // parent node is aware of the varg expr
	ivm_bool_t is_del;
} ilang_gen_flag_t;

#define ilang_gen_flag_build(...) ((ilang_gen_flag_t) { __VA_ARGS__ })

typedef struct {
	ivm_bool_t use_cond_reg; // return value to confirm the use of register
	ivm_bool_t has_base; // has base object on the stack under top(second object)
	ivm_bool_t is_id_loc;
	ivm_bool_t is_id_top;
	ivm_bool_t use_branch;
} ilang_gen_value_t;

#define ilang_gen_value_build(...) ((ilang_gen_value_t) { __VA_ARGS__ })

struct ilang_gen_expr_t_tag;

typedef ilang_gen_value_t (*ilang_gen_eval_t)(struct ilang_gen_expr_t_tag *expr,
											  ilang_gen_flag_t flag,
											  ilang_gen_env_t *env);

typedef struct {
	ivm_bool_t has_side_effect;
} ilang_gen_check_flag_t;

typedef ivm_bool_t (*ilang_gen_checker_t)(struct ilang_gen_expr_t_tag *expr, ilang_gen_check_flag_t flag);

#define ILANG_GEN_EXPR_HEADER \
	ilang_gen_pos_t pos;        \
	ilang_gen_eval_t eval;      \
	ilang_gen_checker_t check;  \
	ivm_bool_t is_missing;

typedef struct ilang_gen_expr_t_tag {
	ILANG_GEN_EXPR_HEADER
	struct ilang_gen_expr_t_tag *oprs[];
} ilang_gen_expr_t;

IVM_INLINE
void
ilang_gen_expr_init(ilang_gen_expr_t *expr,
					ilang_gen_pos_t pos,
					ilang_gen_eval_t eval,
					ilang_gen_checker_t check)
{
	expr->pos = pos;
	expr->eval = eval;
	expr->check = check;
	expr->is_missing = IVM_FALSE;
	return;
}

#define ilang_gen_expr_isExpr(expr, name) \
	((expr)->eval == ilang_gen_##name##_eval)

#define ilang_gen_expr_getPos(expr) ((expr)->pos)

typedef ivm_ptlist_t ilang_gen_expr_list_t;
typedef IVM_PTLIST_ITER_TYPE(ilang_gen_expr_t *) ilang_gen_expr_list_iterator_t;

typedef struct ilang_gen_trans_unit_t_tag {
	ivm_heap_t *heap;
	ivm_char_t *file;

	ilang_gen_expr_list_t *expr_log;
	ivm_ptlist_t *ptlist_log;
	ivm_ptlist_t *list_log;
	ilang_gen_expr_t *top_level;
} ilang_gen_trans_unit_t;

IVM_INLINE
ilang_gen_expr_list_t *
ilang_gen_expr_list_new(ilang_gen_trans_unit_t *unit)
{
	ivm_ptlist_t *list = ivm_ptlist_new();

	ivm_ptlist_push(unit->ptlist_log, list);

	return list;
}

#define ilang_gen_expr_list_push ivm_ptlist_push
#define ilang_gen_expr_list_pushFront ivm_ptlist_pushFront
#define ilang_gen_expr_list_size ivm_ptlist_size
#define ilang_gen_expr_list_at(list, i) ivm_ptlist_at((list), (i))
#define ilang_gen_expr_list_reverse(list) ivm_ptlist_reverse(list)

#define ILANG_GEN_EXPR_LIST_ITER_SET(iter, val) (IVM_PTLIST_ITER_SET((iter), (val)))
#define ILANG_GEN_EXPR_LIST_ITER_GET(iter) ((ilang_gen_expr_t *)IVM_PTLIST_ITER_GET(iter))
#define ILANG_GEN_EXPR_LIST_ITER_IS_FIRST IVM_PTLIST_ITER_IS_FIRST
#define ILANG_GEN_EXPR_LIST_EACHPTR(list, iter) IVM_PTLIST_EACHPTR((list), iter, ilang_gen_expr_t *)
#define ILANG_GEN_EXPR_LIST_EACHPTR_R(list, iter) IVM_PTLIST_EACHPTR_R((list), iter, ilang_gen_expr_t *)

ivm_bool_t
ilang_gen_check_true(ilang_gen_expr_t *expr,
					 ilang_gen_check_flag_t flag);

#define COMMON_EXPR(name, fname, init, ...) \
	ivm_bool_t                                                                       \
	ilang_gen_##name##_check(ilang_gen_expr_t *expr,                                 \
							 ilang_gen_check_flag_t flag);                           \
	ilang_gen_value_t                                                                \
	ilang_gen_##name##_eval(ilang_gen_expr_t *expr,                                  \
							ilang_gen_flag_t flag,                                   \
							ilang_gen_env_t *env);                                   \
                                                                                     \
	IVM_INLINE                                                                       \
	ilang_gen_expr_t *                                                               \
	ilang_gen_##name##_new(ilang_gen_trans_unit_t *trans_unit,                       \
						   ilang_gen_pos_t pos,                                      \
						   __VA_ARGS__)                                              \
	{                                                                                \
		ilang_gen_##name##_t *ret = ivm_heap_alloc(trans_unit->heap, sizeof(*ret));  \
                                                                                     \
		ilang_gen_expr_list_push(trans_unit->expr_log, ret);                         \
		ilang_gen_expr_init(IVM_AS(ret, ilang_gen_expr_t),                           \
							pos, ilang_gen_##name##_eval,                            \
							ilang_gen_check_true);                                   \
		init;                                                                        \
                                                                                     \
		return IVM_AS(ret, ilang_gen_expr_t);                                        \
	} int dummy()

/* block */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_list_t *list;
} ilang_gen_expr_block_t;

COMMON_EXPR(expr_block, "expression block", {
	ret->list = list;
}, ilang_gen_expr_list_t *list);

/* none expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
} ilang_gen_none_expr_t;

COMMON_EXPR(none_expr, "none expression", { }, int dummy);

/* int expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_token_value_t val;
} ilang_gen_int_expr_t;

COMMON_EXPR(int_expr, "integer expression", {
	ret->val = val;
}, ilang_gen_token_value_t val);

/* float expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_token_value_t val;
} ilang_gen_float_expr_t;

COMMON_EXPR(float_expr, "float expression", {
	ret->val = val;
}, ilang_gen_token_value_t val);

/* string expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_token_value_t val;
} ilang_gen_string_expr_t;

COMMON_EXPR(string_expr, "string expression", {
	ret->val = val;
}, ilang_gen_token_value_t val);

/* pa expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
} ilang_gen_pa_expr_t;

COMMON_EXPR(pa_expr, "partial applied expression", {
}, ivm_int_t dummy);

/* varg expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *bondee;
} ilang_gen_varg_expr_t;

COMMON_EXPR(varg_expr, "varg expression", {
	ret->bondee = bondee;
}, ilang_gen_expr_t *bondee);

/* id expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_token_value_t val;
} ilang_gen_id_expr_t;

COMMON_EXPR(id_expr, "id expression", {
	ret->val = val;
}, ilang_gen_token_value_t val);

/* import expr */
typedef ivm_list_t ilang_gen_token_value_list_t;
typedef IVM_LIST_ITER_TYPE(ilang_gen_token_value_t) ilang_gen_token_value_list_iterator_t;

IVM_INLINE
ilang_gen_token_value_list_t *
ilang_gen_token_value_list_new(ilang_gen_trans_unit_t *unit)
{
	ivm_list_t *list = ivm_list_new(sizeof(ilang_gen_token_value_t));
	
	ivm_ptlist_push(unit->list_log, list);

	return list;
}

#define ilang_gen_token_value_list_push ivm_list_push
#define ilang_gen_token_value_list_size ivm_list_size
#define ilang_gen_token_value_list_reverse ivm_list_reverse

#define ILANG_GEN_TOKEN_VALUE_LIST_ITER_SET(iter, val) IVM_LIST_ITER_SET((iter), (val), ilang_gen_token_value_t)
#define ILANG_GEN_TOKEN_VALUE_LIST_ITER_GET(iter) IVM_LIST_ITER_GET((iter), ilang_gen_token_value_t)
#define ILANG_GEN_TOKEN_VALUE_LIST_ITER_GET_PTR(iter) IVM_LIST_ITER_GET_PTR((iter), ilang_gen_token_value_t)
#define ILANG_GEN_TOKEN_VALUE_LIST_EACHPTR(list, iter) IVM_LIST_EACHPTR((list), iter, ilang_gen_token_value_t)
#define ILANG_GEN_TOKEN_VALUE_LIST_EACHPTR_R(list, iter) IVM_LIST_EACHPTR_R((list), iter, ilang_gen_token_value_t)

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_token_value_list_t *mod;
} ilang_gen_import_expr_t;

COMMON_EXPR(import_expr, "import expression", {
	ret->mod = mod;
}, ilang_gen_token_value_list_t *mod);

/* table expr */
typedef struct {
	ilang_gen_pos_t pos;
	ilang_gen_token_value_t name;
	ivm_int_t oop; // -1 for not oop
	ilang_gen_expr_list_t *index;
	ilang_gen_expr_t *expr;
	ivm_bool_t no_parse;
} ilang_gen_table_entry_t;

#define ilang_gen_table_entry_build(pos, name, expr) \
	((ilang_gen_table_entry_t) { (pos), (name), -1, IVM_NULL, (expr) })

#define ilang_gen_table_entry_build_np(pos, name, expr) \
	((ilang_gen_table_entry_t) { (pos), (name), -1, IVM_NULL, (expr), IVM_TRUE })

#define ilang_gen_table_entry_build_oop(pos, oop, expr) \
	((ilang_gen_table_entry_t) { (pos), ilang_gen_token_value_build(IVM_NULL, 0), (oop), IVM_NULL, (expr) })

#define ilang_gen_table_entry_build_index(pos, id, expr) \
	((ilang_gen_table_entry_t) { (pos), ilang_gen_token_value_build(IVM_NULL, 0), -1, (id), (expr) })

typedef ivm_list_t ilang_gen_table_entry_list_t;
typedef IVM_LIST_ITER_TYPE(ilang_gen_table_entry_t) ilang_gen_table_entry_list_iterator_t;

IVM_INLINE
ilang_gen_table_entry_list_t *
ilang_gen_table_entry_list_new(ilang_gen_trans_unit_t *unit)
{
	ivm_list_t *list = ivm_list_new(sizeof(ilang_gen_table_entry_t));
	
	ivm_ptlist_push(unit->list_log, list);

	return list;
}

#define ilang_gen_table_entry_list_push ivm_list_push
#define ilang_gen_table_entry_list_size ivm_list_size
#define ilang_gen_table_entry_list_reverse ivm_list_reverse

#define ILANG_GEN_TABLE_ENTRY_LIST_ITER_SET(iter, val) IVM_LIST_ITER_SET((iter), (val), ilang_gen_table_entry_t)
#define ILANG_GEN_TABLE_ENTRY_LIST_ITER_GET(iter) IVM_LIST_ITER_GET((iter), ilang_gen_table_entry_t)
#define ILANG_GEN_TABLE_ENTRY_LIST_ITER_GET_PTR(iter) IVM_LIST_ITER_GET_PTR((iter), ilang_gen_table_entry_t)
#define ILANG_GEN_TABLE_ENTRY_LIST_EACHPTR(list, iter) IVM_LIST_EACHPTR((list), iter, ilang_gen_table_entry_t)
#define ILANG_GEN_TABLE_ENTRY_LIST_EACHPTR_R(list, iter) IVM_LIST_EACHPTR_R((list), iter, ilang_gen_table_entry_t)

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_table_entry_list_t *list;
} ilang_gen_table_expr_t;

COMMON_EXPR(table_expr, "table expression", {
	ret->list = list;
}, ilang_gen_table_entry_list_t *list);

/* list expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_list_t *elems;
} ilang_gen_list_expr_t;

COMMON_EXPR(list_expr, "list expression", {
	ret->elems = elems;
}, ilang_gen_expr_list_t *elems);

/* list comp expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *core;
	ivm_int_t cblock;
} ilang_gen_list_comp_core_expr_t;

COMMON_EXPR(list_comp_core_expr, "list comprehension core expression", {
	ret->core = core;
	ret->cblock = cblock;
}, ilang_gen_expr_t *core,
   ivm_int_t cblock);

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *expr;
} ilang_gen_list_comp_expr_t;

COMMON_EXPR(list_comp_expr, "list comprehension expression", {
	ret->expr = expr;
}, ilang_gen_expr_t *expr);

/* call expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *callee;
	ilang_gen_expr_list_t *args;
} ilang_gen_call_expr_t;

COMMON_EXPR(call_expr, "call expression", {
	ret->callee = callee;
	ret->args = args;
}, ilang_gen_expr_t *callee, ilang_gen_expr_list_t *args);

/* slot expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *obj;
	ilang_gen_token_value_t slot;
	ivm_bool_t parse; // parse the ID string or not
} ilang_gen_slot_expr_t;

COMMON_EXPR(slot_expr, "slot expression", {
	ret->obj = obj;
	ret->slot = slot;
	ret->parse = parse;
}, ilang_gen_expr_t *obj,
   ilang_gen_token_value_t slot,
   ivm_bool_t parse);

/* oop expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *obj;
	ivm_int_t oop;
} ilang_gen_oop_expr_t;

COMMON_EXPR(oop_expr, "oop expression", {
	ret->obj = obj;
	ret->oop = oop;
}, ilang_gen_expr_t *obj,
   ivm_int_t oop);

/* unary expr */
enum {
	ILANG_GEN_UNIOP_REF = IVM_UNIOP_COUNT + 1,
	ILANG_GEN_UNIOP_DEREF
};

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *opr;
	ivm_int_t type;
} ilang_gen_unary_expr_t;

COMMON_EXPR(unary_expr, "unary expression", {
	ret->opr = opr;
	ret->type = type;
}, ilang_gen_expr_t *opr, ivm_int_t type);

/* binary expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *op1;
	ilang_gen_expr_t *op2;
	ivm_int_t type;
} ilang_gen_binary_expr_t;

COMMON_EXPR(binary_expr, "binary expression", {
	ret->op1 = op1;
	ret->op2 = op2;
	ret->type = type;
}, ilang_gen_expr_t *op1, ilang_gen_expr_t *op2, ivm_int_t type);

/* cop expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *op1;
	ilang_gen_expr_t *op2;
	ilang_gen_token_value_t op;
} ilang_gen_cop_expr_t;

COMMON_EXPR(cop_expr, "custom operator expression", {
	ret->op1 = op1;
	ret->op2 = op2;
	ret->op = op;
}, ilang_gen_expr_t *op1, ilang_gen_expr_t *op2, ilang_gen_token_value_t op);

/* cmp expr */
enum {
	ILANG_GEN_CMP_LT = -2,
	ILANG_GEN_CMP_LE = -1,
	ILANG_GEN_CMP_EQ = 0,
	ILANG_GEN_CMP_GE = 1,
	ILANG_GEN_CMP_GT = 2,
	ILANG_GEN_CMP_NE = 3,
	ILANG_GEN_CMP_IS
};

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *op1;
	ilang_gen_expr_t *op2;
	ivm_int_t cmp_type; /* -2: lt, -1: le, 0: eq, 1: ge, 2: gt, 3: ne */
} ilang_gen_cmp_expr_t;

COMMON_EXPR(cmp_expr, "compare expression", {
	ret->op1 = op1;
	ret->op2 = op2;
	ret->cmp_type = cmp_type;
}, ilang_gen_expr_t *op1, ilang_gen_expr_t *op2, ivm_int_t cmp_type);

/* index expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *op;
	ilang_gen_expr_list_t *idx;
} ilang_gen_index_expr_t;

COMMON_EXPR(index_expr, "index expression", {
	ret->op = op;
	ret->idx = idx;
}, ilang_gen_expr_t *op, ilang_gen_expr_list_t *idx);

/* logic expr */
enum {
	ILANG_GEN_LOGIC_AND = 0,
	ILANG_GEN_LOGIC_OR = 1
};

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *lhe;
	ilang_gen_expr_t *rhe;
	ivm_int_t type;
} ilang_gen_logic_expr_t;

COMMON_EXPR(logic_expr, "logic expression", {
	ret->lhe = lhe;
	ret->rhe = rhe;
	ret->type = type;
}, ilang_gen_expr_t *lhe, ilang_gen_expr_t *rhe, ivm_int_t type);

typedef struct {
	ilang_gen_token_value_t name;
	ilang_gen_expr_t *def;
	ivm_bool_t is_varg;
} ilang_gen_param_t;

#define ilang_gen_param_build(varg, n, d) \
	((ilang_gen_param_t) { .name = (n), .def = (d) , .is_varg = (varg) })

typedef ivm_list_t ilang_gen_param_list_t;
typedef IVM_LIST_ITER_TYPE(ilang_gen_param_t) ilang_gen_param_list_iterator_t;

IVM_INLINE
ilang_gen_param_list_t *
ilang_gen_param_list_new(ilang_gen_trans_unit_t *unit)
{
	ivm_list_t *list = ivm_list_new(sizeof(ilang_gen_param_t));
	
	ivm_ptlist_push(unit->list_log, list);

	return list;
}

#define ilang_gen_param_list_push ivm_list_push
#define ilang_gen_param_list_size ivm_list_size

#define ILANG_GEN_PARAM_LIST_ITER_SET(iter, val) IVM_LIST_ITER_SET((iter), (val), ilang_gen_param_t)
#define ILANG_GEN_PARAM_LIST_ITER_GET(iter) IVM_LIST_ITER_GET((iter), ilang_gen_param_t)
#define ILANG_GEN_PARAM_LIST_ITER_GET_PTR(iter) IVM_LIST_ITER_GET_PTR((iter), ilang_gen_param_t)
#define ILANG_GEN_PARAM_LIST_EACHPTR_R(list, iter) IVM_LIST_EACHPTR_R((list), iter, ilang_gen_param_t)
#define ILANG_GEN_PARAM_LIST_EACHPTR(list, iter) IVM_LIST_EACHPTR((list), iter, ilang_gen_param_t)

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_param_list_t *params;
	ilang_gen_expr_t *body;
} ilang_gen_fn_expr_t;

COMMON_EXPR(fn_expr, "function expression", {
	ret->params = params;
	ret->body = body;
}, ilang_gen_param_list_t *params, ilang_gen_expr_t *body);

typedef struct {
	ilang_gen_expr_t *cond;
	ilang_gen_expr_t *body;
} ilang_gen_branch_t;

#define ilang_gen_branch_build(cond, body) ((ilang_gen_branch_t) { (cond), (body) })

typedef ivm_list_t ilang_gen_branch_list_t;
typedef IVM_LIST_ITER_TYPE(ilang_gen_branch_t) ilang_gen_branch_list_iterator_t;

IVM_INLINE
ilang_gen_branch_list_t *
ilang_gen_branch_list_new(ilang_gen_trans_unit_t *unit)
{
	ivm_list_t *list = ivm_list_new(sizeof(ilang_gen_branch_t));

	ivm_ptlist_push(unit->list_log, list);

	return list;
}

#define ilang_gen_branch_list_push ivm_list_push
#define ilang_gen_branch_list_size ivm_list_size

#define ILANG_GEN_BRANCH_LIST_ITER_SET(iter, val) IVM_LIST_ITER_SET((iter), (val), ilang_gen_branch_t)
#define ILANG_GEN_BRANCH_LIST_ITER_GET(iter) IVM_LIST_ITER_GET((iter), ilang_gen_branch_t)
#define ILANG_GEN_BRANCH_LIST_ITER_GET_PTR(iter) IVM_LIST_ITER_GET_PTR((iter), ilang_gen_branch_t)
#define ILANG_GEN_BRANCH_LIST_ITER_IS_LAST(list, iter) IVM_LIST_ITER_IS_LAST((list), (iter), ilang_gen_branch_t)
#define ILANG_GEN_BRANCH_LIST_ITER_IS_FIRST(list, iter) IVM_LIST_ITER_IS_FIRST((list), (iter), ilang_gen_branch_t)
#define ILANG_GEN_BRANCH_LIST_EACHPTR(list, iter) IVM_LIST_EACHPTR((list), iter, ilang_gen_branch_t)
#define ILANG_GEN_BRANCH_LIST_EACHPTR_R(list, iter) IVM_LIST_EACHPTR_R((list), iter, ilang_gen_branch_t)

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_branch_t main;
	ilang_gen_branch_list_t *elifs;
	ilang_gen_branch_t last;
} ilang_gen_if_expr_t;

COMMON_EXPR(if_expr, "if expression", {
	ret->main = main;
	ret->elifs = elifs;
	ret->last = last;
}, ilang_gen_branch_t main,
   ilang_gen_branch_list_t *elifs,
   ilang_gen_branch_t last);

#define ilang_gen_if_expr_new_c(unit, pos, cond, body) \
	ilang_gen_if_expr_new(                             \
		(unit), (pos),                                 \
		ilang_gen_branch_build((cond), (body)),        \
		ilang_gen_branch_list_new(unit),               \
		ilang_gen_branch_build(IVM_NULL, IVM_NULL)     \
	)

#define ilang_gen_if_expr_setMainBody(expr, nbody) \
	(IVM_AS((expr), ilang_gen_if_expr_t)->main.body = (nbody))

#define ilang_gen_if_expr_getMainBody(expr) \
	(IVM_AS((expr), ilang_gen_if_expr_t)->main.body)

/* while expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *cond;
	ilang_gen_expr_t *body;
} ilang_gen_while_expr_t;

COMMON_EXPR(while_expr, "while expression", {
	ret->cond = cond;
	ret->body = body;
}, ilang_gen_expr_t *cond,
   ilang_gen_expr_t *body);

/* for expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *var;
	ilang_gen_expr_t *iteree;
	ilang_gen_expr_t *body;
} ilang_gen_for_expr_t;

COMMON_EXPR(for_expr, "for expression", {
	ret->var = var;
	ret->iteree = iteree;
	ret->body = body;
}, ilang_gen_expr_t *var,
   ilang_gen_expr_t *iteree,
   ilang_gen_expr_t *body);

#define ilang_gen_for_expr_setBody(expr, nbody) \
	(IVM_AS((expr), ilang_gen_for_expr_t)->body = (nbody))

#define ilang_gen_for_expr_getBody(expr) \
	(IVM_AS((expr), ilang_gen_for_expr_t)->body)

/* try expr */
typedef struct {
	ilang_gen_expr_t *arg;
	ilang_gen_expr_t *body;
} ilang_gen_catch_branch_t;

#define ilang_gen_catch_branch_build(arg, body) ((ilang_gen_catch_branch_t) { (arg), (body) })

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *try_body;
	ilang_gen_catch_branch_t catch_body;
	ilang_gen_expr_t *final_body;
} ilang_gen_try_expr_t;

COMMON_EXPR(try_expr, "try expression", {
	ret->try_body = try_body;
	ret->catch_body = catch_body;
	ret->final_body = final_body;
}, ilang_gen_expr_t *try_body,
   ilang_gen_catch_branch_t catch_body,
   ilang_gen_expr_t *final_body);

/* fork expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *forkee;
	ivm_bool_t is_group;
} ilang_gen_fork_expr_t;

COMMON_EXPR(fork_expr, "fork expression", {
	ret->forkee = forkee;
	ret->is_group = is_group;
}, ilang_gen_expr_t *forkee,
   ivm_bool_t is_group);

/* assert expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *cond;
} ilang_gen_assert_expr_t;

COMMON_EXPR(assert_expr, "assert expression", {
	ret->cond = cond;
}, ilang_gen_expr_t *cond);

/* intr expr */
enum {
	ILANG_GEN_INTR_RET = 1,
	ILANG_GEN_INTR_CONT,
	ILANG_GEN_INTR_BREAK,
	ILANG_GEN_INTR_RAISE,
	ILANG_GEN_INTR_YIELD,
	ILANG_GEN_INTR_RESUME
};

typedef struct {
	ILANG_GEN_EXPR_HEADER
	ivm_int_t sig;
	ilang_gen_expr_t *val;
	ilang_gen_expr_t *with;
} ilang_gen_intr_expr_t;

COMMON_EXPR(intr_expr, "intr expression", {
	ret->sig = sig;
	ret->val = val;
	ret->with = with;
}, ivm_int_t sig,
   ilang_gen_expr_t *val,
   ilang_gen_expr_t *with);

/* assign expr */
typedef struct {
	ILANG_GEN_EXPR_HEADER
	ilang_gen_expr_t *lhe;
	ilang_gen_expr_t *rhe;
	ivm_int_t inp_op; // -1 means no inplace op
} ilang_gen_assign_expr_t;

COMMON_EXPR(assign_expr, "assign expression", {
	ret->lhe = lhe;
	ret->rhe = rhe;
	ret->inp_op = inp_op;
}, ilang_gen_expr_t *lhe,
   ilang_gen_expr_t *rhe,
   ivm_int_t inp_op);

#undef COMMON_EXPR

IVM_INLINE
ilang_gen_trans_unit_t *
ilang_gen_trans_unit_new(const ivm_char_t *file)
{
	ilang_gen_trans_unit_t *ret = STD_ALLOC(sizeof(*ret));

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("translate unit"));

	ret->heap = ivm_heap_new(IVM_DEFAULT_PARSER_INIT_HEAP_SIZE);
	ret->file = IVM_STRDUP(file);

	ret->ptlist_log = ivm_ptlist_new();
	ret->list_log = ivm_ptlist_new();
	ret->expr_log = ilang_gen_expr_list_new(ret);
	ret->top_level = IVM_NULL;

	return ret;
}

IVM_INLINE
void
ilang_gen_trans_unit_setTopLevel(ilang_gen_trans_unit_t *unit,
								 ilang_gen_expr_list_t *expr_list)
{
	unit->top_level = ilang_gen_expr_block_new(
		unit, ilang_gen_pos_build(0, 0), expr_list
	);

	return;
}

IVM_INLINE
void
ilang_gen_trans_unit_free(ilang_gen_trans_unit_t *unit)
{
	IVM_PTLIST_ITER_TYPE(ivm_ptlist_t *) piter;
	IVM_PTLIST_ITER_TYPE(ivm_list_t *) liter;

	if (unit) {
		ivm_heap_free(unit->heap);
		STD_FREE(unit->file);

		IVM_PTLIST_EACHPTR(unit->ptlist_log, piter, ivm_ptlist_t *) {
			ivm_ptlist_free(IVM_PTLIST_ITER_GET(piter));
		}

		IVM_PTLIST_EACHPTR(unit->list_log, liter, ivm_list_t *) {
			ivm_list_free(IVM_PTLIST_ITER_GET(liter));
		}

		ivm_ptlist_free(unit->ptlist_log);
		ivm_ptlist_free(unit->list_log);

		STD_FREE(unit);
	}

	return;
}

ivm_exec_unit_t *
ilang_gen_generateExecUnit_c(ilang_gen_trans_unit_t *unit,
							 ivm_string_pool_t *str_pool);

IVM_INLINE
ivm_exec_unit_t *
ilang_gen_generateExecUnit(ilang_gen_trans_unit_t *unit)
{
	return ilang_gen_generateExecUnit_c(unit, IVM_NULL);
}

IVM_COM_END

#endif
