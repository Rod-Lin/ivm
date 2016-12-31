#ifndef _IVM_PUB_CONST_H_
#define _IVM_PUB_CONST_H_

#define IVM_USE_PERF_PROFILE 1

/* triggers of pools */
#define IVM_USE_FUNCTION_POOL 1
#define IVM_USE_CORO_POOL 0
#define IVM_USE_INLINE_CACHE 1
#define IVM_USE_BLOCK_POOL 0

#ifndef IVM_USE_MULTITHREAD
	#define IVM_USE_MULTITHREAD 1
#endif

#define IVM_INSTR_CACHE_UID_MAX (1l << 50)

#define IVM_DEFAULT_CONTEXT_POOL_SIZE 32

#define IVM_DEFAULT_FUNCTION_POOL_SIZE 32
#define IVM_DEFAULT_FRAME_POOL_SIZE 32
#define IVM_DEFAULT_CORO_POOL_SIZE 32
#define IVM_DEFAULT_CTHREAD_POOL_SIZE 32

#define IVM_DEFAULT_BLOCK_POOL_CACHE_LEN 4
#define IVM_DEFAULT_BLOCK_POOL_BUFFER_SIZE 8

#define IVM_DEFAULT_BLOCK_STACK_BUFFER_SIZE 8

#define IVM_DEFAULT_PARSER_INIT_HEAP_SIZE (2 << 15)

/* list buffer sizes */
#define IVM_DEFAULT_PTLIST_BUFFER_SIZE 8
#define IVM_DEFAULT_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_LIST_OBJECT_BUFFER_SIZE 8
#define IVM_DEFAULT_PTHASH_BUFFER_SIZE 8
#define IVM_DEFAULT_PTHASH_MAX_CONF_COUNT 1

#define IVM_DEFAULT_TYPE_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_VMSTACK_BUFFER_SIZE 16
#define IVM_DEFAULT_FRAME_STACK_BUFFER_SIZE 32
#define IVM_DEFAULT_CORO_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_CGROUP_LIST_BUFFER_SIZE 2
#define IVM_DEFAULT_FUNC_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_EXEC_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_CGROUP_STACK_BUFFER_SIZE 4
#define IVM_DEFAULT_EXEC_BUFFER_SIZE 16
#define IVM_DEFAULT_CORO_INT_BUFFER_SIZE 32

#define IVM_DEFAULT_FILE_READ_BUFFER_SIZE 128

#define IVM_DEFAULT_EXCEPTION_BUFFER_SIZE 256

#define IVM_DEFAULT_RAWSTR_LIST_BUFFER_SIZE 4
#define IVM_DEFAULT_DLL_LIST_BUFFER_SIZE 4

#define IVM_DEFAULT_DESTRUCT_LIST_BUFFER_SIZE 32
#define IVM_DEFAULT_WBOBJ_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_WBSLOT_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_WBCTX_LIST_BUFFER_SIZE 8
#define IVM_DEFAULT_WBCORO_LIST_BUFFER_SIZE 8

/* string pool block size */
#define IVM_DEFAULT_STRING_POOL_BLOCK_SIZE (2 << 15)
#define IVM_DEFAULT_STRING_POOL_BUFFER_SIZE 32
#define IVM_DEFAULT_STRING_POOL_MAX_CONF_COUNT 2

#define IVM_DEFAULT_MAX_CONST_STRING_SIZE 256
#define IVM_DEFAULT_MAX_OBJECT_SIZE IVM_DEFAULT_INIT_HEAP_SIZE

#define IVM_DEFAULT_SLOT_TABLE_SIZE 4
/* when the number of elements is greater than this value, the slot table will be turned into hash table */
#define IVM_DEFAULT_SLOT_TABLE_TO_HASH_THRESHOLD 6
#define IVM_DEFAULT_SLOT_TABLE_MAX_CONF_COUNT 2

/* GC config */
#define IVM_DEFAULT_MAX_STD_LIMIT (2 << 30)
#define IVM_DEFAULT_INIT_HEAP_SIZE (2 << 20)
#define IVM_DEFAULT_HEAP_MAX_COMPACT_BC 2
#define IVM_DEFAULT_GC_MAX_LIVE_RATIO 30 // only when live ratio is under 15%
#define IVM_DEFAULT_GC_BC_RESTORE_RATIO 30
#define IVM_DEFAULT_GC_BC_WEIGHT 5
#define IVM_DEFAULT_GC_MAX_SKIP 200
#define IVM_DEFAULT_GC_WILD_THRESHOLD IVM_DEFAULT_INIT_HEAP_SIZE

/* only one of the following definition can be 1 */
#define IVM_DISPATCH_METHOD_DIRECT_THREAD 1

/* the number of stack element(s) cached */
#define IVM_STACK_CACHE_N_TOS 0

#define IVM_PER_INSTR_DBG(runtime) // ivm_dbg_printRuntime(runtime)
#define IVM_GC_DBG 0

#define IVM_COPYRIGHT_HELP "this project is released under the MIT license"
#define IVM_TAB "   "

#define IVM_ASM_FILE_SUFFIX ".ias"
#define IVM_CACHE_FILE_SUFFIX ".ivc"

#define IVM_MAX_MOD_NAME_LEN 128

#endif
