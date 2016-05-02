#include <time.h>
#include "pub/mem.h"
#include "pub/const.h"
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

#define INC_PERIOD(collector) ((collector)->period++, \
							   !(collector)->period \
							   ? (collector)->period++ \
							   : 0)

ivm_collector_t *
ivm_collector_new()
{
	ivm_collector_t *ret = MEM_ALLOC(sizeof(*ret));

	IVM_ASSERT(ret, IVM_ERROR_MSG_FAILED_ALLOC_NEW("garbage collector"));

	ret->des_log = ivm_cell_set_new();
	ret->period = 1;

	return ret;
}

void
ivm_collector_free(ivm_collector_t *collector, ivm_vmstate_t *state)
{
	ivm_cell_set_destruct(collector->des_log, state);
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
	ivm_mark_period_t period = IVM_COLLECTOR_GET(arg->collector, PERIOD);

	if (!obj
		|| IVM_OBJECT_GET(obj, MARK) == period) {
		/* NULL object or has been marked */
		return obj ? IVM_OBJECT_GET(obj, COPY) : IVM_NULL;
	}

	IVM_OBJECT_SET(obj, MARK, period);

	ret = ivm_heap_addCopy(arg->heap, obj, IVM_OBJECT_GET(obj, TYPE_SIZE));

	IVM_OBJECT_SET(ret, COPY, ret);
	IVM_OBJECT_SET(obj, COPY, ret);

	IVM_OBJECT_SET(ret, SLOTS, ivm_slot_table_copy(IVM_OBJECT_GET(ret, SLOTS), arg->heap));

	ivm_slot_table_foreach(IVM_OBJECT_GET(ret, SLOTS), 
						   (ivm_slot_table_foreach_proc_t)
						   ivm_collector_travSlot,
						   arg);

	trav = IVM_OBJECT_GET(obj, TYPE_TRAV);
	if (trav) {
		trav(ret, arg);
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
		ivm_collector_travContextChain(IVM_CALLER_INFO_GET(info, CONTEXT), arg);
	}

	return;
}

static void
ivm_collector_travRuntime(ivm_runtime_t *runtime,
						  ivm_traverser_arg_t *arg)
{
	if (runtime) {
		ivm_collector_travContextChain(IVM_RUNTIME_GET(runtime, CONTEXT), arg);
	}

	return;
}

static void
ivm_collector_travCoro(ivm_coro_t *coro,
					   ivm_traverser_arg_t *arg)
{
	ivm_vmstack_t *stack = IVM_CORO_GET(coro, STACK);
	ivm_call_stack_t *call_st = IVM_CORO_GET(coro, CALL_STACK);
	ivm_runtime_t *runtime = IVM_CORO_GET(coro, RUNTIME);
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
	ivm_coro_list_t *coros = IVM_VMSTATE_GET(arg->state, CORO_LIST);

	ivm_coro_list_foreach_arg(coros,
							  (ivm_ptlist_foreach_proc_arg_t)
							  ivm_collector_travCoro,
							  arg);

	return;
}

static
void
ivm_collector_destructCell(ivm_cell_t *cell, ivm_cell_set_t *set,
						   ivm_collector_t *collector, ivm_vmstate_t *state)
{
	ivm_object_t *obj = IVM_CELL_GET(cell, OBJ);

	if (obj) {
		if (IVM_OBJECT_GET(obj, MARK)
			!= IVM_COLLECTOR_GET(collector, PERIOD)) {
			ivm_cell_removeFrom(cell, set);
			ivm_cell_destruct(cell, state);
		} else {
			/* update reference */
			IVM_CELL_SET(cell, OBJ, obj->copy);
		}
	}

	return;
}

void
ivm_collector_triggerDestructor(ivm_collector_t *collector, ivm_vmstate_t *state)
{
	ivm_cell_set_foreach(collector->des_log,
						 (ivm_cell_set_foreach_proc_t)ivm_collector_destructCell,
						 collector, state);
	return;
}

#if IVM_PERF_PROFILE

clock_t ivm_perf_gc_time = 0;

#endif

void
ivm_collector_collect(ivm_collector_t *collector,
					  ivm_vmstate_t *state,
					  ivm_heap_t *heap)
{

#if IVM_PERF_PROFILE
	clock_t time_st = clock(), time_taken;
#endif

	ivm_traverser_arg_t arg;

	arg.state = state;
	arg.heap = IVM_VMSTATE_GET(state, EMPTY_HEAP);
	arg.collector = collector;

	ivm_heap_reset(arg.heap);

	printf("***collecting***\n");

	ivm_collector_travState(collector, &arg);
	ivm_collector_triggerDestructor(collector, state);
	ivm_vmstate_swapHeap(state);
	ivm_heap_compact(IVM_VMSTATE_GET(state, CUR_HEAP));
	INC_PERIOD(collector);

	ivm_vmstate_closeGCFlag(state);

#if IVM_PERF_PROFILE
	time_taken = clock() - time_st;
	ivm_perf_gc_time += time_taken;
#endif

	/* printf("***collection end: %ld ticks taken***\n", time_taken); */

	return;
}
