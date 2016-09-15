TYPE_GEN(IVM_NONE_T, none, sizeof(ivm_object_t), {
}, .const_bool = IVM_FALSE)

TYPE_GEN(IVM_OBJECT_T, object, sizeof(ivm_object_t), {

	ivm_object_t *tmp = ivm_object_new(_STATE);
	ivm_type_setProto(_TYPE, tmp);

	ivm_object_setSlot_r(tmp, _STATE, "merge", IVM_NATIVE_WRAP(_STATE, _object_merge));

}, .const_bool = IVM_TRUE)

TYPE_GEN(IVM_NUMERIC_T, numeric, sizeof(ivm_numeric_t), {
	
	ivm_object_t *tmp = ivm_numeric_new(_STATE, IVM_NUM(0));
	ivm_type_setProto(_TYPE, tmp);
	ivm_object_setProto(tmp, _STATE, ivm_vmstate_getTypeProto(_STATE, IVM_OBJECT_T));

	ivm_object_setSlot_r(tmp, _STATE, "ceil", IVM_NATIVE_WRAP(_STATE, _numeric_ceil));
	ivm_object_setSlot_r(tmp, _STATE, "floor", IVM_NATIVE_WRAP(_STATE, _numeric_floor));
	ivm_object_setSlot_r(tmp, _STATE, "round", IVM_NATIVE_WRAP(_STATE, _numeric_round));

	ivm_object_setSlot_r(tmp, _STATE, "isnan", IVM_NATIVE_WRAP(_STATE, _numeric_isnan));
	ivm_object_setSlot_r(tmp, _STATE, "isinf", IVM_NATIVE_WRAP(_STATE, _numeric_isinf));
	ivm_object_setSlot_r(tmp, _STATE, "isposinf", IVM_NATIVE_WRAP(_STATE, _numeric_isposinf));
	ivm_object_setSlot_r(tmp, _STATE, "isneginf", IVM_NATIVE_WRAP(_STATE, _numeric_isneginf));

	ivm_object_setSlot_r(tmp, _STATE, "char", IVM_NATIVE_WRAP(_STATE, _numeric_char));

}, .to_bool = ivm_numeric_isTrue)

TYPE_GEN(IVM_STRING_OBJECT_T, string, sizeof(ivm_string_object_t), {
	
	ivm_object_t *tmp = ivm_string_object_new(_STATE, IVM_VMSTATE_CONST(_STATE, C_EMPTY));
	ivm_type_setProto(_TYPE, tmp);
	ivm_object_setProto(tmp, _STATE, ivm_vmstate_getTypeProto(_STATE, IVM_OBJECT_T));

	ivm_object_setSlot_r(tmp, _STATE, "len", IVM_NATIVE_WRAP(_STATE, _string_len));
	ivm_object_setSlot_r(tmp, _STATE, "ord", IVM_NATIVE_WRAP(_STATE, _string_ord));

}, .trav = ivm_string_object_traverser,
   .const_bool = IVM_TRUE)

TYPE_GEN(IVM_LIST_OBJECT_T, list, sizeof(ivm_list_object_t), {
	
	ivm_object_t *tmp = ivm_list_object_new(_STATE, 0);
	ivm_type_setProto(_TYPE, tmp);
	ivm_object_setProto(tmp, _STATE, ivm_vmstate_getTypeProto(_STATE, IVM_OBJECT_T));

	ivm_object_setSlot(tmp, _STATE, IVM_VMSTATE_CONST(_STATE, C_SIZE), IVM_NATIVE_WRAP(_STATE, _list_size));
	ivm_object_setSlot_r(tmp, _STATE, "push", IVM_NATIVE_WRAP(_STATE, _list_push));
	ivm_object_setSlot_r(tmp, _STATE, "slice", IVM_NATIVE_WRAP(_STATE, _list_slice));
	ivm_object_setSlot(tmp, _STATE, IVM_VMSTATE_CONST(_STATE, C_ITER), IVM_NATIVE_WRAP(_STATE, _list_iter));

}, .des = ivm_list_object_destructor,
   .clone = ivm_list_object_cloner,
   .trav = ivm_list_object_traverser,
   .const_bool = IVM_TRUE)

TYPE_GEN(IVM_FUNCTION_OBJECT_T, function, sizeof(ivm_function_object_t), {
	
	ivm_object_t *tmp = ivm_function_object_new(_STATE, IVM_NULL, IVM_NULL);
	ivm_type_setProto(_TYPE, tmp);
	ivm_object_setProto(tmp, _STATE, ivm_vmstate_getTypeProto(_STATE, IVM_OBJECT_T));

}, .des = ivm_function_object_destructor,
   .clone = ivm_function_object_cloner,
   .trav = ivm_function_object_traverser,
   .const_bool = IVM_TRUE)

TYPE_GEN(IVM_LIST_OBJECT_ITER_T, list_iter, sizeof(ivm_list_object_iter_t), {
	
	ivm_object_t *tmp = ivm_list_object_iter_new(_STATE, 0);
	ivm_type_setProto(_TYPE, tmp);
	ivm_object_setProto(tmp, _STATE, ivm_vmstate_getTypeProto(_STATE, IVM_OBJECT_T));

	ivm_object_setSlot(tmp, _STATE, IVM_VMSTATE_CONST(_STATE, C_NEXT), IVM_NATIVE_WRAP(_STATE, _list_iter_next));

}, .trav = ivm_list_object_iter_traverser,
   .const_bool = IVM_TRUE)
