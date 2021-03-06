#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "pub/type.h"
#include "pub/vm.h"
#include "pub/inlines.h"

#include "std/time.h"

#include "vm/native/native.h"
#include "vm/native/priv.h"

#ifdef __FAST_MATH__
	#error math module will not work correctly with GCC -ffast-math option open
#endif

#ifdef IVM_OS_WIN32
	#define MATH_RAND_MAX (RAND_MAX + 1)
#else
	#define MATH_RAND_MAX RAND_MAX
#endif

#define C_PI 3.14159265358979323846

#define C_NAN (((union { ivm_sint64_t i; ivm_double_t d; }){ .i = ~0 }).d)
#define C_INF (1.0 / 0.0)

#define DEG_TO_RAD(d) ((d) / 180 * C_PI)
#define RAD_TO_DEG(r) ((r) / C_PI * 180)

#define MATH_ERR_MSG_VAL1_DOMAIN_ERROR(func, val, domain)		"%s(x) function domain error(" domain ", %f given)", (func), (val)
#define MATH_ERR_MSG_RANDOM_MIN_MAX_ERROR						"the min argument is greater than the max in random()"

#define MATH_ASSERT_ABS_VAL_LESS_THAN_ONE(func) \
	RTM_ASSERT(val <= 1.0 && val >= -1.0, MATH_ERR_MSG_VAL1_DOMAIN_ERROR(func, val, "|x| <= 1"))

#define MATH_ASSERT_POS_VAL(func) \
	RTM_ASSERT(val > 0, MATH_ERR_MSG_VAL1_DOMAIN_ERROR(func, val, "x > 0"))

#define MATH_ASSERT_NON_NEG_VAL(func) \
	RTM_ASSERT(val >= 0, MATH_ERR_MSG_VAL1_DOMAIN_ERROR(func, val, "x >= 0"))

#define VAL1F_C(name, f) \
	IVM_NATIVE_FUNC(_math_##name)                       \
	{                                                   \
		ivm_number_t val;                               \
		MATCH_ARG("n", &val);                           \
		return ivm_numeric_new(NAT_STATE(), f(val));    \
	}

#define VAL1F(f) \
	IVM_NATIVE_FUNC(_math_##f)                          \
	{                                                   \
		ivm_number_t val;                               \
		MATCH_ARG("n", &val);                           \
		return ivm_numeric_new(NAT_STATE(), f(val));    \
	}

#define VAL1F_E(f, e) \
	IVM_NATIVE_FUNC(_math_##f)                          \
	{                                                   \
		ivm_number_t val;                               \
		MATCH_ARG("n", &val);                           \
		e;                                              \
		return ivm_numeric_new(NAT_STATE(), f(val));    \
	}

#define VAL2F(f) \
	IVM_NATIVE_FUNC(_math_##f)                          \
	{                                                   \
		ivm_number_t v1, v2;                            \
		MATCH_ARG("nn", &v1, &v2);                      \
		return ivm_numeric_new(NAT_STATE(), f(v1, v2)); \
	}

VAL1F_C(rad, DEG_TO_RAD)
VAL1F_C(deg, RAD_TO_DEG)

VAL1F(sin)
VAL1F(cos)
VAL1F(tan)

VAL1F_E(asin, { MATH_ASSERT_ABS_VAL_LESS_THAN_ONE("asin"); })
VAL1F_E(acos, { MATH_ASSERT_ABS_VAL_LESS_THAN_ONE("acos"); })
VAL1F(atan)
VAL2F(atan2)

VAL1F(sinh)
VAL1F(cosh)
VAL1F(tanh)

VAL1F_C(abs, fabs)

VAL1F_E(log, { MATH_ASSERT_POS_VAL("log") })
VAL1F_E(log10, { MATH_ASSERT_POS_VAL("log10") })
VAL1F(exp)

VAL2F(pow)
VAL1F_E(sqrt, { MATH_ASSERT_NON_NEG_VAL("sqrt") })

IVM_NATIVE_FUNC(_math_random)
{
	ivm_number_t min = 0, max = 1;

	MATCH_ARG("*nn", &min, &max);
	RTM_ASSERT(max >= min, MATH_ERR_MSG_RANDOM_MIN_MAX_ERROR);
	
	return ivm_numeric_new(NAT_STATE(), min + ((ivm_number_t)rand() / MATH_RAND_MAX) * (max - min));
}

ivm_object_t *
ivm_mod_main(ivm_vmstate_t *state,
			 ivm_coro_t *coro,
			 ivm_context_t *context)
{
	ivm_object_t *mod = ivm_object_new_c(state, 22);

	#define DEF_FUNC(name) \
		ivm_object_setSlot_r(mod, state, #name, IVM_NATIVE_WRAP(state, _math_##name))

	#define DEF_CONST(name, val) \
		ivm_object_setSlot_r(mod, state, (name), ivm_numeric_new(state, (val)))

	DEF_FUNC(rad); // means `to radians`
	DEF_FUNC(deg); // means `to degrees`

	DEF_FUNC(sin); DEF_FUNC(cos); DEF_FUNC(tan);
	DEF_FUNC(asin); DEF_FUNC(acos); DEF_FUNC(atan); DEF_FUNC(atan2);
	DEF_FUNC(sinh); DEF_FUNC(cosh); DEF_FUNC(tanh);

	DEF_FUNC(abs);
	
	DEF_FUNC(log); DEF_FUNC(log10); DEF_FUNC(exp);

	DEF_FUNC(pow); DEF_FUNC(sqrt);

	srand(ivm_time_hwclock());
	DEF_FUNC(random);

	DEF_CONST("pi", C_PI);
	DEF_CONST("inf", C_INF);
	DEF_CONST("nan", C_NAN);

	return mod;
}
