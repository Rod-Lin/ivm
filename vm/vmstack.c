#include "pub/type.h"
#include "pub/err.h"

#include "std/mem.h"

#include "vmstack.h"
#include "obj.h"
#include "coro.h"
#include "call.h"

void
ivm_vmstack_init(ivm_vmstack_t *stack)
{
	stack->size = IVM_DEFAULT_VMSTACK_BUFFER_SIZE;
	stack->edge = (
		(stack->bottom = STD_ALLOC(
			sizeof(*stack->bottom)
			* IVM_DEFAULT_VMSTACK_BUFFER_SIZE
		)) + IVM_DEFAULT_VMSTACK_BUFFER_SIZE
	);

	IVM_MEMCHECK(stack->bottom);

	return;
}

ivm_vmstack_t *
ivm_vmstack_new()
{
	ivm_vmstack_t *ret = STD_ALLOC(sizeof(*ret));

	IVM_MEMCHECK(ret);

	ivm_vmstack_init(ret);

	return ret;
}

void
ivm_vmstack_free(ivm_vmstack_t *stack)
{
	if (stack) {
		STD_FREE(stack->bottom);
		STD_FREE(stack);
	}

	return;
}

void
ivm_vmstack_dump(ivm_vmstack_t *stack)
{
	if (stack) {
		STD_FREE(stack->bottom);
	}

	return;
}

void
ivm_vmstack_inc(ivm_vmstack_t *stack,
				ivm_frame_stack_t *fstack)
{
	ivm_object_t **nst, **ost, **tmp_bp, **tmp_rbp;
	ivm_frame_stack_iterator_t siter;
	ivm_frame_t *tmp;

	stack->size <<= 1;
	ost = stack->bottom;
	nst = STD_REALLOC(ost, sizeof(*ost) * stack->size);

	IVM_MEMCHECK(nst);

	IVM_FRAME_STACK_EACHPTR(fstack, siter) {
		tmp = IVM_FRAME_STACK_ITER_GET(siter);
		tmp_bp = nst + ivm_vmstack_offset(stack, IVM_FRAME_GET(tmp, BP));
		tmp_rbp = nst + ivm_vmstack_offset(stack, IVM_FRAME_GET(tmp, RBP));
		IVM_FRAME_SET(tmp, BP, tmp_bp);
		IVM_FRAME_SET(tmp, RBP, tmp_rbp);
	}

	stack->bottom = nst;
	stack->edge = nst + stack->size;

	return;
}

void
ivm_vmstack_inc_c(ivm_vmstack_t *stack,
				  ivm_coro_t *coro)
{
	ivm_object_t **nst, **ost, **tmp_bp, **tmp_sp, **tmp_rbp;
	ivm_frame_stack_iterator_t siter;
	ivm_frame_t *tmp;
	ivm_runtime_t *runtime = IVM_CORO_GET(coro, RUNTIME);

	stack->size <<= 1;
	ost = stack->bottom;
	nst = STD_REALLOC(ost, sizeof(*ost) * stack->size);

	IVM_MEMCHECK(nst);

	// update stack reference
	IVM_FRAME_STACK_EACHPTR(IVM_CORO_GET(coro, FRAME_STACK), siter) {
		tmp = IVM_FRAME_STACK_ITER_GET(siter);
		tmp_bp = nst + ivm_vmstack_offset(stack, IVM_FRAME_GET(tmp, BP));
		tmp_rbp = nst + ivm_vmstack_offset(stack, IVM_FRAME_GET(tmp, RBP));
		IVM_FRAME_SET(tmp, BP, tmp_bp);
		IVM_FRAME_SET(tmp, RBP, tmp_rbp);
	}

	if (runtime) {
		tmp_bp = nst + ivm_vmstack_offset(stack, IVM_RUNTIME_GET(runtime, BP));
		tmp_rbp = nst + ivm_vmstack_offset(stack, IVM_RUNTIME_GET(runtime, RBP));
		tmp_sp = nst + ivm_vmstack_offset(stack, IVM_RUNTIME_GET(runtime, SP));
		IVM_RUNTIME_SET(runtime, BP, tmp_bp);
		IVM_RUNTIME_SET(runtime, RBP, tmp_rbp);
		IVM_RUNTIME_SET(runtime, SP, tmp_sp);
	}

	stack->bottom = nst;
	stack->edge = nst + stack->size;

	return;
}

void
ivm_vmstack_ensure(ivm_vmstack_t *stack,
				   ivm_coro_t *coro,
				   ivm_size_t size)
{
	ivm_object_t **nst, **ost, **tmp_bp, **tmp_sp, **tmp_rbp;
	ivm_frame_stack_iterator_t siter;
	ivm_frame_t *tmp;
	ivm_runtime_t *runtime = IVM_CORO_GET(coro, RUNTIME);

	if (stack->size < size) {
		stack->size <<= 1;
		stack->size += size;
	} else {
		stack->size <<= 1;
	}

	ost = stack->bottom;
	nst = STD_REALLOC(ost, sizeof(*ost) * stack->size);

	IVM_MEMCHECK(nst);

	IVM_FRAME_STACK_EACHPTR(IVM_CORO_GET(coro, FRAME_STACK), siter) {
		tmp = IVM_FRAME_STACK_ITER_GET(siter);
		tmp_bp = nst + ivm_vmstack_offset(stack, IVM_FRAME_GET(tmp, BP));
		tmp_rbp = nst + ivm_vmstack_offset(stack, IVM_FRAME_GET(tmp, RBP));
		IVM_FRAME_SET(tmp, BP, tmp_bp);
		IVM_FRAME_SET(tmp, RBP, tmp_rbp);
	}

	if (runtime) {
		tmp_bp = nst + ivm_vmstack_offset(stack, IVM_RUNTIME_GET(runtime, BP));
		tmp_rbp = nst + ivm_vmstack_offset(stack, IVM_RUNTIME_GET(runtime, RBP));
		tmp_sp = nst + ivm_vmstack_offset(stack, IVM_RUNTIME_GET(runtime, SP));
		IVM_RUNTIME_SET(runtime, BP, tmp_bp);
		IVM_RUNTIME_SET(runtime, RBP, tmp_rbp);
		IVM_RUNTIME_SET(runtime, SP, tmp_sp);
	}

	stack->bottom = nst;
	stack->edge = nst + stack->size;

	return;
}
