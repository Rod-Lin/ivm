#include "priv.h"

ilang_gen_value_t
ilang_gen_unary_expr_eval(ilang_gen_expr_t *expr,
						  ilang_gen_flag_t flag,
						  ilang_gen_env_t *env)
{
	ilang_gen_unary_expr_t *unary_expr = IVM_AS(expr, ilang_gen_unary_expr_t);

	GEN_ASSERT_NOT_LEFT_VALUE(expr, "unary expression", flag);

	if (flag.is_top_level &&
		!expr->check(expr, CHECK_SE())) {
		return NORET();
	}

	unary_expr->opr->eval(
		unary_expr->opr,
		FLAG(.is_top_level = flag.is_top_level),
		env
	);

	if (flag.is_top_level) {
		return NORET();
	}

	switch (unary_expr->type) {
		case IVM_UNIOP_ID(NOT):
			ivm_exec_addInstr(env->cur_exec, NOT);
			break;
		case IVM_UNIOP_ID(CLONE):
			ivm_exec_addInstr(env->cur_exec, CLONE);
			break;
		default:
			IVM_FATAL(GEN_ERR_MSG_UNSUPPORTED_UNARY_OP(unary_expr->type));
	}

	return NORET();
}

ilang_gen_value_t
ilang_gen_binary_expr_eval(ilang_gen_expr_t *expr,
						   ilang_gen_flag_t flag,
						   ilang_gen_env_t *env)
{
	ilang_gen_binary_expr_t *binary_expr = IVM_AS(expr, ilang_gen_binary_expr_t);
	ilang_gen_expr_t *op1, *op2;

	GEN_ASSERT_NOT_LEFT_VALUE(expr, "binary expression", flag);

	op1 = binary_expr->op1;
	op2 = binary_expr->op2;

	if (flag.is_top_level &&
		!expr->check(expr, CHECK_SE())) {
		/* is top level and has no side effect */
		return NORET();
	}

	op1->eval(op1, FLAG(.is_top_level = flag.is_top_level), env);
	op2->eval(op2, FLAG(.is_top_level = flag.is_top_level), env);

	if (flag.is_top_level) {
		return NORET();
	}

#define BR(op) \
	case IVM_BINOP_ID(op):                      \
		ivm_exec_addInstr(env->cur_exec, op);   \
		break;

	switch (binary_expr->type) {
		BR(ADD)
		BR(SUB)
		BR(MUL)
		BR(DIV)
		BR(MOD)
		BR(AND)
		BR(EOR)
		BR(IOR)
		default:
			IVM_FATAL(GEN_ERR_MSG_UNSUPPORTED_BINARY_OP(binary_expr->type));
	}

#undef BR

	return NORET();
}

ilang_gen_value_t
ilang_gen_cmp_expr_eval(ilang_gen_expr_t *expr,
						ilang_gen_flag_t flag,
						ilang_gen_env_t *env)
{
	ilang_gen_cmp_expr_t *cmp_expr = IVM_AS(expr, ilang_gen_cmp_expr_t);
	ilang_gen_expr_t *op1, *op2;

	GEN_ASSERT_NOT_LEFT_VALUE(expr, "compare expression", flag);

	op1 = cmp_expr->op1;
	op2 = cmp_expr->op2;

	if (flag.is_top_level &&
		!expr->check(expr, CHECK_SE())) {
		/* is top level and has no side effect */
		return NORET();
	}

	op1->eval(op1, FLAG(.is_top_level = flag.is_top_level), env);
	op2->eval(op2, FLAG(.is_top_level = flag.is_top_level), env);

	if (flag.is_top_level) {
		return NORET();
	}

#define BR(op) \
	case ILANG_GEN_CMP_##op:                             \
		if (flag.if_use_cond_reg) {                      \
			ivm_exec_addInstr(env->cur_exec, op##_R);    \
			return RETVAL(.use_cond_reg = IVM_TRUE);     \
		} else {                                         \
			ivm_exec_addInstr(env->cur_exec, op);        \
		}                                                \
		break;

	switch (cmp_expr->cmp_type) {
		BR(LT)
		BR(LE)
		BR(EQ)
		BR(GE)
		BR(GT)
		BR(NE)
		default:
			IVM_FATAL(GEN_ERR_MSG_UNSUPPORTED_CMP_TYPE(cmp_expr->cmp_type));
	}

#undef BR

	return NORET();
}
