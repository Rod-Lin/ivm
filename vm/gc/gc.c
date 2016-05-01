#include "pub/mem.h"
#include "gc.h"
#include "cell.h"
#include "../slot.h"
#include "../obj.h"
#include "../vmstack.h"
#include "../coro.h"
#include "../call.h"
#include "../context.h"
#include "../vm.h"
#include "../err.h"

#define INC_PERIOD(collector) ((collector)->period++)

ivm_collector_t *
ivm_collector_new()
{
	ivm_collector_t *ret = MEM_ALLOC(sizeof(*ret));

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("garbage collector"));

	ret->period = 1;

	return ret;
}

void
ivm_collector_free(ivm_collector_t *collector)
{
	MEM_FREE(collector);
	return;
}

static ivm_object_t *
ivm_collector_copyObject(ivm_object_t *obj,
						 ivm_traverser_arg_t *arg);

static void
ivm_collector_travSlot(ivm_slot_t *slot,
					   ivm_traverser_arg_t *arg)
{
	ivm_slot_setValue(slot, arg->state,
					  ivm_collector_copyObject(ivm_slot_getValue(slot, arg->state), arg));
	return;
}

static ivm_object_t *
ivm_collector_copyObject(ivm_object_t *obj,
						 ivm_traverser_arg_t *arg)
{
	ivm_object_t *ret = IVM_NULL;
	ivm_traverser_t trav;

	if (!obj
		|| IVM_OBJECT_MARK(obj)
		   == IVM_COLLECTOR_PERIOD(arg->collector)) {
		/* NULL object or has been marked */
		return obj ? obj->copy : IVM_NULL;
	}

	IVM_OBJECT_MARK(obj)
	= IVM_COLLECTOR_PERIOD(arg->collector);

	ret = ivm_heap_addCopy(arg->heap, obj, IVM_TYPE_SIZE_OF(obj));
	ret->copy = obj->copy = ret;

	IVM_OBJECT_SLOTS(ret) = ivm_slot_table_copy(IVM_OBJECT_SLOTS(ret), arg->heap);

	ivm_slot_table_foreach(IVM_OBJECT_SLOTS(ret), 
						   (ivm_slot_table_foreach_proc_t)
						   ivm_collector_travSlot,
						   arg);

	trav = IVM_TYPE_TRAV_OF(obj);
	if (trav) {
		trav(obj, arg);
	}

	return ret;
}

static void
ivm_collector_travContextChain(ivm_ctchain_t *chain,
							   ivm_traverser_arg_t *arg)
{
	ivm_ctchain_iterator_t *iter;

	if (chain) {
		IVM_CTCHAIN_EACHPTR(chain, iter) {
			IVM_CTCHAIN_ITER_SET(iter,
								 ivm_collector_copyObject(IVM_CTCHAIN_ITER_GET(iter), arg));
		}
	}

	return;
}

static void
ivm_collector_travCallerInfo(ivm_caller_info_t *info,
							 ivm_traverser_arg_t *arg)
{
	if (info) {
		ivm_collector_travContextChain(IVM_CALLER_INFO_CONTEXT(info), arg);
	}

	return;
}

static void
ivm_collector_travRuntime(ivm_runtime_t *runtime,
						  ivm_traverser_arg_t *arg)
{
	if (runtime) {
		ivm_collector_travContextChain(IVM_RUNTIME_CONTEXT(runtime), arg);
	}

	return;
}

static void
ivm_collector_travCoro(ivm_coro_t *coro,
					   ivm_traverser_arg_t *arg)
{
	ivm_vmstack_t *stack = IVM_CORO_STACK(coro);
	ivm_call_stack_t *call_st = IVM_CORO_CALL_STACK(coro);
	ivm_runtime_t *runtime = IVM_CORO_RUNTIME(coro);
	ivm_object_t **tmp;

	IVM_VMSTACK_EACHPTR(stack, tmp) {
		*tmp = ivm_collector_copyObject(*tmp, arg);
	}

	ivm_call_stack_foreach_arg(call_st,
							   (ivm_ptlist_foreach_proc_arg_t)
	 						   ivm_collector_travCallerInfo,
	 						   arg);
	ivm_collector_travRuntime(runtime, arg);

	return;
}

static void
ivm_collector_travState(ivm_collector_t *collector,
						ivm_traverser_arg_t *arg)
{
	ivm_coro_list_t *coros = IVM_VMSTATE_CORO_LIST(arg->state);

	ivm_coro_list_foreach_arg(coros,
							  (ivm_ptlist_foreach_proc_arg_t)
							  ivm_collector_travCoro,
							  arg);

	return;
}

void
ivm_collector_collect(ivm_collector_t *collector,
					  struct ivm_vmstate_t_tag *state,
					  ivm_heap_t *heap)
{
	ivm_traverser_arg_t arg;

	arg.state = state;
	arg.heap = IVM_VMSTATE_EMPTY_HEAP(state);
	arg.collector = collector;

	ivm_heap_reset(arg.heap);

	printf("***collecting***\n");

	ivm_collector_travState(collector, &arg);
	ivm_vmstate_swapHeap(state);
	INC_PERIOD(collector);

	return;
}
