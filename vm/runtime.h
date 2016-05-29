#ifndef _IVM_VM_RUNTIME_H_
#define _IVM_VM_RUNTIME_H_

#include "pub/com.h"
#include "pub/mem.h"
#include "type.h"
#include "exec.h"
#include "func.h"
#include "context.h"
#include "call.h"

IVM_COM_HEADER

struct ivm_coro_t_tag;
struct ivm_frame_t_tag;
struct ivm_vmstate_t_tag;

typedef struct ivm_runtime_t_tag {
	IVM_EXEC_INFO_HEAD
} ivm_runtime_t;

#define IVM_RUNTIME_GET_IS_NATIVE(runtime) ((runtime)->exec == IVM_NULL)
#define IVM_RUNTIME_GET_PC(runtime) ((runtime)->pc)
#define IVM_RUNTIME_GET_EXEC(runtime) ((runtime)->exec)
#define IVM_RUNTIME_GET_CONTEXT(runtime) ((runtime)->context)

#define IVM_RUNTIME_GET_PC_PTR(runtime) (&(runtime)->pc)
#define IVM_RUNTIME_GET_EXEC_PTR(runtime) (&(runtime)->exec)
#define IVM_RUNTIME_GET_CONTEXT_PTR(runtime) (&(runtime)->context)

#define IVM_RUNTIME_GET(obj, member) IVM_GET((obj), IVM_RUNTIME, member)
#define IVM_RUNTIME_SET(obj, member, val) IVM_SET((obj), IVM_RUNTIME, member, (val))

ivm_runtime_t *
ivm_runtime_new(struct ivm_vmstate_t_tag *state);

void
ivm_runtime_free(ivm_runtime_t *runtime,
				 struct ivm_vmstate_t_tag *state);

/* just rewrite new environment */
/* NOTICE: this function will NOT copy the context chain,
 *		   clone the context chain by yourself
 */
void
ivm_runtime_invoke(ivm_runtime_t *runtime,
				   struct ivm_vmstate_t_tag *state,
				   ivm_exec_t *exec,
				   ivm_ctchain_t *context);

/* pack current state to frame */
ivm_frame_t *
ivm_runtime_getFrame(ivm_runtime_t *runtime,
					 struct ivm_vmstate_t_tag *state,
					 struct ivm_coro_t_tag *coro);

/* restore frame */
void
ivm_runtime_restore(ivm_runtime_t *runtime,
					struct ivm_vmstate_t_tag *state,
					struct ivm_coro_t_tag *coro,
					struct ivm_frame_t_tag *frame);

typedef ivm_ptpool_t ivm_runtime_pool_t;

#define ivm_runtime_pool_new(count) (ivm_ptpool_new((count), sizeof(ivm_runtime_t)))
#define ivm_runtime_pool_free ivm_ptpool_free
#define ivm_runtime_pool_alloc(pool) ((ivm_runtime_t *)ivm_ptpool_alloc(pool))
#define ivm_runtime_pool_dump ivm_ptpool_dump

IVM_COM_END

#endif
