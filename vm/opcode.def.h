OPCODE_GEN(NOP, "nop", N, {
	NEXT_INSTR();
})

OPCODE_GEN(NEW_NULL, "new_null", N, {
	STACK_PUSH(IVM_NULL_OBJ(_STATE));
	NEXT_INSTR();
})

OPCODE_GEN(NEW_OBJ, "new_obj", N, {
	STACK_PUSH(ivm_object_new(_STATE));
	NEXT_INSTR();
})

OPCODE_GEN(NEW_NUM_I, "new_num_i", I, {
	STACK_PUSH(ivm_numeric_new(_STATE, IARG()));
	NEXT_INSTR();
})

OPCODE_GEN(NEW_NUM_F, "new_num_f", F, {
	STACK_PUSH(ivm_numeric_new(_STATE, FARG()));
	NEXT_INSTR();
})

/*
OPCODE_GEN(NEW_NUM_S, "new_num_s", S, {
	NEXT_INSTR();
})
*/

OPCODE_GEN(NEW_STR, "new_str", S, {
	STACK_PUSH(ivm_string_object_new(_STATE, SARG()));
	NEXT_INSTR();
})

OPCODE_GEN(NEW_FUNC, "new_func", X, {
	STACK_PUSH(
		ivm_function_object_new(
			_STATE, _CONTEXT,
			ivm_vmstate_getFunc(_STATE, XARG())
		)
	);
	NEXT_INSTR();
})

/* unary operations */
OPCODE_GEN(NOT, "not", N, DEFAULT_UNIOP_HANDLER(NOT, "!"))

/* binary operations */
OPCODE_GEN(ADD, "add", N, DEFAULT_BINOP_HANDLER(ADD, "+"))
OPCODE_GEN(SUB, "sub", N, DEFAULT_BINOP_HANDLER(SUB, "-"))
OPCODE_GEN(MUL, "mul", N, DEFAULT_BINOP_HANDLER(MUL, "*"))
OPCODE_GEN(DIV, "div", N, DEFAULT_BINOP_HANDLER(DIV, "/"))
OPCODE_GEN(MOD, "mod", N, DEFAULT_BINOP_HANDLER(MOD, "%"))

OPCODE_GEN(LT, "lt", N, CMP_BINOP_HANDLER(
	STACK_PUSH(ivm_numeric_new(_STATE, _RETVAL < 0));
	NEXT_INSTR();
))

OPCODE_GEN(JUMP_LT, "jump_lt", I, CMP_BINOP_HANDLER(
	if (_RETVAL < 0) {
		NEXT_N_INSTR(IARG());
	} else {
		NEXT_INSTR();
	}
))

OPCODE_GEN(GET_SLOT, "get_slot", S, {
	CHECK_STACK(1);

	_TMP_OBJ = STACK_POP();
	GET_SLOT(_TMP_OBJ, SARG());
	STACK_PUSH(_TMP_OBJ ? _TMP_OBJ : IVM_UNDEFINED(_STATE));

	NEXT_INSTR();
})

OPCODE_GEN(SET_SLOT, "set_slot", S, {
	CHECK_STACK(2);

	_TMP_OBJ = STACK_POP();
	SET_SLOT(_TMP_OBJ, SARG(), STACK_POP());
	STACK_PUSH(_TMP_OBJ);

	NEXT_INSTR();
})

OPCODE_GEN(GET_CONTEXT_SLOT, "get_context_slot", S, {
	_TMP_OBJ = ivm_ctchain_search_cc(_CONTEXT, _STATE,
									 SARG(), _INSTR_CACHE);

	STACK_PUSH(_TMP_OBJ ? _TMP_OBJ : IVM_UNDEFINED(_STATE));
	NEXT_INSTR();
})

OPCODE_GEN(SET_CONTEXT_SLOT, "set_context_slot", S, {
	const ivm_string_t *key = SARG();

	CHECK_STACK(1);

	_TMP_OBJ = STACK_POP();

	if (!ivm_ctchain_setSlotIfExist_cc(
			_CONTEXT, _STATE, key,
			_TMP_OBJ, _INSTR_CACHE
		)) {
		ivm_ctchain_setLocalSlot_cc(
			_CONTEXT, _STATE, key,
			_TMP_OBJ, _INSTR_CACHE
		);
	}

	NEXT_INSTR();
})

OPCODE_GEN(SET_ARG, "set_arg", S, {
	ivm_ctchain_setLocalSlot_cc(_CONTEXT, _STATE,
								SARG(),
								AVAIL_STACK >= 1
								? STACK_POP()
								: IVM_UNDEFINED(_STATE),
								_INSTR_CACHE);

	NEXT_INSTR();
})

OPCODE_GEN(POP, "pop", N, {
	CHECK_STACK(1);
	STACK_POP();

	NEXT_INSTR();
})

OPCODE_GEN(DUP_N, "dup_n", I, {
	ivm_size_t i = IARG();

	CHECK_STACK(i + 1);

	_TMP_OBJ = STACK_BEFORE(i);
	STACK_PUSH(_TMP_OBJ);

	NEXT_INSTR();
})

OPCODE_GEN(DUP, "dup", N, {
	CHECK_STACK(1);

	_TMP_OBJ = STACK_TOP();
	STACK_PUSH(_TMP_OBJ);

	NEXT_INSTR();
})

OPCODE_GEN(PRINT_OBJ, "print_obj", N, {
	CHECK_STACK(1);

	_TMP_OBJ = STACK_POP();
	IVM_OUT("print: %p\n", (void *)_TMP_OBJ);

	NEXT_INSTR();
})

OPCODE_GEN(PRINT_NUM, "print_num", N, {
	CHECK_STACK(1);

	_TMP_OBJ = STACK_POP();
	if (IVM_OBJECT_GET(_TMP_OBJ, TYPE_TAG) == IVM_NUMERIC_T)
		IVM_TRACE("print num: %f\n", IVM_AS(_TMP_OBJ, ivm_numeric_t)->val);
	else
		IVM_TRACE("cannot print number of object of type <%s>\n", IVM_OBJECT_GET(_TMP_OBJ, TYPE_NAME));

	NEXT_INSTR();
})

OPCODE_GEN(PRINT_TYPE, "print_type", N, {
	CHECK_STACK(1);

	_TMP_OBJ = STACK_POP();
	IVM_TRACE("type: %s\n", _TMP_OBJ ? IVM_OBJECT_GET(_TMP_OBJ, TYPE_NAME) : "empty pointer");
	
	NEXT_INSTR();
})

OPCODE_GEN(PRINT_STR, "print_str", N, {
	ivm_string_object_t *str;

	CHECK_STACK(1);

	str = IVM_AS(STACK_POP(), ivm_string_object_t);

	IVM_ASSERT(IVM_IS_TYPE(str, IVM_STRING_OBJECT_T),
			   IVM_ERROR_MSG_NOT_TYPE("string", IVM_OBJECT_GET(str, TYPE_NAME)));

	IVM_OUT("%s\n", ivm_string_trimHead(str->val));

	NEXT_INSTR();
})

OPCODE_GEN(PRINT_STACK, "print_stack", N, {
	ivm_dbg_stackState(_CORO, stderr);
	NEXT_INSTR();
})

OPCODE_GEN(OUT, "out", S, {
	IVM_TRACE("%s\n", ivm_string_trimHead(SARG()));
	NEXT_INSTR();
})

OPCODE_GEN(OUT_NUM, "out_num", N, {
	CHECK_STACK(1);

	_TMP_OBJ = STACK_POP();
	if (IVM_OBJECT_GET(_TMP_OBJ, TYPE_TAG) == IVM_NUMERIC_T)
		IVM_TRACE("print num: %f\n", IVM_AS(_TMP_OBJ, ivm_numeric_t)->val);
	else
		IVM_TRACE("cannot print number of object of type <%s>\n", IVM_OBJECT_GET(_TMP_OBJ, TYPE_NAME));

	NEXT_INSTR();
})

OPCODE_GEN(INVOKE, "invoke", I, {
	ivm_function_t *func;
	ivm_sint32_t arg_count = IARG();
	ivm_object_t **args;
	ivm_object_t *ret;

	CHECK_STACK(arg_count + 1);

	_TMP_OBJ = STACK_POP();
	// IVM_TRACE("%p\n", obj);

	IVM_ASSERT(IVM_IS_TYPE(_TMP_OBJ, IVM_FUNCTION_OBJECT_T),
			   IVM_ERROR_MSG_NOT_TYPE("function", IVM_OBJECT_GET(_TMP_OBJ, TYPE_NAME)));
	
	func = ivm_function_object_getFunc(IVM_AS(_TMP_OBJ, ivm_function_object_t));
	args = STACK_CUT(arg_count);

	/* IVM_RUNTIME_SET(_RUNTIME, IP, _INSTR + 1); */
	SAVE_RUNTIME(_INSTR + 1);

	ivm_function_invoke(func, _STATE,
						ivm_function_object_getClosure(
							IVM_AS(_TMP_OBJ, ivm_function_object_t)
						),
						_CORO);
	
	UPDATE_STACK();

	if (ivm_function_isNative(func)) {
		IVM_PER_INSTR_DBG(DBG_RUNTIME_ACTION(INVOKE, 1 /* native invoke */));

		ret = ivm_function_callNative(func, _STATE, _CONTEXT,
									  IVM_FUNCTION_SET_ARG_2(arg_count, args));
		STACK_PUSH(ret ? ret : IVM_NULL_OBJ(_STATE));
	} else {
		IVM_PER_INSTR_DBG(DBG_RUNTIME_ACTION(INVOKE, IVM_NULL));

		STACK_INC(arg_count);
	}

	INVOKE();
})

OPCODE_GEN(FORK, "fork", N, {
	CHECK_STACK(1);
	_TMP_OBJ = STACK_POP();

	IVM_ASSERT(IVM_IS_TYPE(_TMP_OBJ, IVM_FUNCTION_OBJECT_T),
			   IVM_ERROR_MSG_NOT_TYPE("function", IVM_OBJECT_GET(_TMP_OBJ, TYPE_NAME)));

	ivm_vmstate_addCoro(
		_STATE,
		IVM_AS(_TMP_OBJ, ivm_function_object_t)
	);

	NEXT_INSTR();
})

OPCODE_GEN(YIELD, "yield", N, {
	YIELD();
})

OPCODE_GEN(RETURN, "return", N, {
	RETURN();
})

OPCODE_GEN(JUMP, "jump", I, {
	NEXT_N_INSTR(IARG());
})

OPCODE_GEN(JUMP_TRUE, "jump_true", I, {
	if (ivm_object_toBool(STACK_POP(), _STATE)) {
		NEXT_N_INSTR(IARG());
	} else {
		NEXT_INSTR();
	}
})

OPCODE_GEN(JUMP_FALSE, "jump_false", I, {
	if (!ivm_object_toBool(STACK_POP(), _STATE)) {
		NEXT_N_INSTR(IARG());
	} else {
		NEXT_INSTR();
	}
})

OPCODE_GEN(TEST1, "test1", N, {
	IVM_OUT("test1\n");
	NEXT_INSTR();
})

OPCODE_GEN(TEST2, "test2", I, { 
	NEXT_INSTR();
})

OPCODE_GEN(TEST3, "test3", S, {
	IVM_OUT("morning! this is test3\n");
	IVM_OUT("string argument: %s\n", ivm_string_trimHead(SARG()));
	NEXT_INSTR();
})
