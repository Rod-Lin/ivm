#include <stdarg.h>

#include "pub/com.h"
#include "pub/err.h"
#include "pub/vm.h"
#include "pub/inlines.h"

#include "std/mem.h"
#include "std/string.h"
#include "std/ref.h"
#include "std/sys.h"

#include "exec.h"

ivm_source_pos_t *
ivm_source_pos_new(const ivm_char_t *file)
{
	ivm_source_pos_t *ret = STD_ALLOC(sizeof(*ret));

	ivm_ref_init(ret);
	ret->file = IVM_STRDUP(file);

	return ret;
}

void
ivm_source_pos_free(ivm_source_pos_t *pos)
{
	if (pos && !ivm_ref_dec(pos)) {
		STD_FREE(pos->file);
		STD_FREE(pos);
	}

	return;
}

void
ivm_param_list_init(ivm_param_list_t *plist,
					ivm_size_t count)
{
	if (count) {
		plist->param = STD_ALLOC_INIT(sizeof(*plist->param) * count);
		plist->count = count;
		IVM_MEMCHECK(plist->param);
	} else {
		plist->param = IVM_NULL;
		plist->count = 0;
	}

	plist->legacy = IVM_FALSE;
	plist->has_varg = IVM_FALSE;

	return;
}

void
ivm_param_list_initLegacy(ivm_param_list_t *plist)
{
	plist->param = IVM_NULL;
	plist->count = 0;
	plist->legacy = IVM_TRUE;

	return;
}

void
ivm_param_list_dump(ivm_param_list_t *plist)
{
	if (plist) {
		STD_FREE(plist->param);
	}

	return;
}

void
ivm_param_list_setParam(ivm_param_list_t *plist,
						ivm_size_t idx, const ivm_string_t *name,
						ivm_bool_t is_varg)
{
	IVM_ASSERT(idx < plist->count, "impossible");

	plist->param[idx].name = name;
	plist->param[idx].is_varg = is_varg;

	plist->has_varg |= is_varg;

	return;
}

IVM_INLINE
void
_ivm_param_list_copy(ivm_param_list_t *from,
					 ivm_param_list_t *to)
{
	if (!from->legacy) {
		ivm_param_list_init(to, from->count);

		if (from->param) {
			STD_MEMCPY(to->param, from->param, sizeof(*to->param) * from->count);
			to->has_varg = from->has_varg;
		}
	} else {
		ivm_param_list_initLegacy(to);
	}

	return;
}

IVM_INLINE
void
_ivm_exec_init(ivm_exec_t *exec,
			   ivm_string_pool_t *pool,
			   ivm_size_t argc)
{
	ivm_ref_init(exec);
	exec->offset = 0;
	exec->pool = pool;
	ivm_ref_inc(pool);

	exec->pos = IVM_NULL;

	// exec->max_stack = 0;
	// exec->fin_stack = 0;
	exec->alloc = IVM_DEFAULT_EXEC_BUFFER_SIZE;
	exec->next = 0;
	exec->instrs = STD_ALLOC(sizeof(*exec->instrs) * IVM_DEFAULT_EXEC_BUFFER_SIZE);

	IVM_MEMCHECK(exec->instrs);

	if (argc != (ivm_size_t)-1){
		ivm_param_list_init(&exec->param, argc);
	} else {
		ivm_param_list_initLegacy(&exec->param);
	}

	exec->cached = IVM_FALSE;

	return;
}

ivm_exec_t *
ivm_exec_new(ivm_string_pool_t *pool,
			 ivm_size_t argc)
{
	ivm_exec_t *ret = STD_ALLOC(sizeof(*ret));

	IVM_MEMCHECK(ret);

	_ivm_exec_init(ret, pool, argc);

	return ret;
}

void
ivm_exec_free(ivm_exec_t *exec)
{
	if (exec && !ivm_ref_dec(exec)) {
		ivm_string_pool_free(exec->pool);
		ivm_source_pos_free(exec->pos);
		ivm_param_list_dump(&exec->param);
		STD_FREE(exec->instrs);
		STD_FREE(exec);
	}

	return;
}

void
ivm_exec_dump(ivm_exec_t *exec)
{
	if (exec) {
		ivm_string_pool_free(exec->pool);
		ivm_source_pos_free(exec->pos);
		ivm_param_list_dump(&exec->param);
		STD_FREE(exec->instrs);
	}

	return;
}

void
ivm_exec_copy(ivm_exec_t *exec,
			  ivm_exec_t *dest)
{
	ivm_size_t size;

	STD_MEMCPY(dest, exec, sizeof(*dest));

	ivm_ref_init(dest);
	ivm_ref_inc(dest->pool);
	if (dest->pos) {
		ivm_ref_inc(dest->pos);
	}

	size = sizeof(*exec->instrs) * exec->alloc;
	dest->instrs = STD_ALLOC(size);
	
	IVM_MEMCHECK(dest->instrs);

	STD_MEMCPY(dest->instrs, exec->instrs, size);

	_ivm_param_list_copy(&exec->param, &dest->param);

	return;
}

IVM_PRIVATE
void
_ivm_exec_expand(ivm_exec_t *exec)
{
	exec->alloc <<= 1;
	exec->instrs = STD_REALLOC(exec->instrs, sizeof(*exec->instrs) * exec->alloc);

	IVM_MEMCHECK(exec->instrs);

	return;
}

ivm_size_t
ivm_exec_addInstr_c(ivm_exec_t *exec,
					ivm_instr_t instr)
{
	// ivm_int_t inc;

	if (exec->next >= exec->alloc) {
		_ivm_exec_expand(exec);
	}

	exec->instrs[exec->next] = instr;

	return exec->next++;
}

void
ivm_exec_preproc(ivm_exec_t *exec,
				 ivm_vmstate_t *state)
{
	ivm_size_t addr, end;
	ivm_instr_t *i, *j;
	ivm_source_pos_t *pos = ivm_exec_getSourcePos(exec);

	if (!exec->cached) {
		exec->cached = IVM_TRUE;
		for (addr = 0, end = exec->next;
			 addr != end; addr++) {
			i = exec->instrs + addr;
			ivm_instr_setPos(i, pos);
			
			switch (ivm_opcode_table_getParam(ivm_instr_opcode(i))[0]) {
				case 'S':
					/* string pool idx -> string pointer */
					ivm_instr_setArg(i,
						ivm_opcode_arg_fromPointer(
							ivm_exec_getString(exec,
								ivm_opcode_arg_toInt(
									ivm_instr_arg(i)
								)
							)
						)
					);
					break;
				case 'A':
					if (ivm_opcode_arg_toInt(ivm_instr_arg(i)) + addr >= end) {
						ivm_exec_addInstr(exec, RETURN);
						end = exec->next; // update end
					}
					break;
			}
		}

		if (!exec->next
			|| (ivm_instr_opcode(exec->instrs + (exec->next - 1))
				!= IVM_OPCODE(RETURN))) {
			// avoid empty address
			ivm_exec_addInstr(exec, RETURN);
		}

		for (i = exec->instrs, j = i + exec->next;
			 i != j; i++) {
			// cache final jump addr of jump instr
			
			if (ivm_opcode_table_getParam(ivm_instr_opcode(i))[0]
				== 'A') {
				ivm_instr_setArg(i,
					ivm_opcode_arg_fromPointer(
						i + ivm_opcode_arg_toInt(
							ivm_instr_arg(i)
						)
					)
				);
			}
		}
	}

	return;
}

ivm_exec_unit_t *
ivm_exec_unit_new(ivm_size_t root,
				  ivm_exec_list_t *execs)
{
	ivm_exec_unit_t *ret = STD_ALLOC(sizeof(*ret));

	IVM_MEMCHECK(ret);

	ret->root = root;
	ret->pos = IVM_NULL;
	ret->execs = execs;

	return ret;
}

void
ivm_exec_unit_free(ivm_exec_unit_t *unit)
{
	if (unit) {
		ivm_exec_list_free(unit->execs);
		ivm_source_pos_free(unit->pos);
		STD_FREE(unit);
	}

	return;
}

ivm_function_t * /* root function */
ivm_exec_unit_mergeToVM(ivm_exec_unit_t *unit,
						ivm_vmstate_t *state)
{
	ivm_function_t *func, *ret = IVM_NULL;
	ivm_exec_t *exec;
	ivm_exec_list_iterator_t eiter;
	ivm_size_t i = 0, root = unit->root;

	ivm_uint_t offset = ivm_vmstate_getLinkOffset(state);
	ivm_source_pos_t *pos = unit->pos;

	IVM_EXEC_LIST_EACHPTR(unit->execs, eiter) {
		exec = IVM_EXEC_LIST_ITER_GET(eiter);
		
		ivm_exec_setOffset(exec, offset);
		ivm_exec_setSourcePos(exec, pos);

		func = ivm_function_new(state, exec);
		ivm_vmstate_registerFunc(state, func);

		if (i++ == root) {
			ret = func;
		}
	}

	return ret;
}

ivm_vmstate_t *
ivm_exec_unit_generateVM(ivm_exec_unit_t *unit)
{
	ivm_string_pool_t *pool;
	ivm_vmstate_t *state;
	ivm_function_t *root;
	ivm_char_t *str;
	ivm_coro_t *coro;

	IVM_ASSERT(ivm_exec_list_size(unit->execs), IVM_ERROR_MSG_MERGE_EMPTY_EXEC_UNIT);

	pool = ivm_exec_pool(ivm_exec_list_at(unit->execs, 0));
	state = ivm_vmstate_new(pool);
	root = ivm_exec_unit_mergeToVM(unit, state);

	if (unit->pos) {
		str = ivm_sys_getBasePath(ivm_source_pos_getFile(unit->pos));
		ivm_vmstate_setCurPath(state, str);
		STD_FREE(str);
	}

	if (root) {
		coro = ivm_coro_new(state);
		ivm_coro_setRoot(coro, state, IVM_AS(ivm_function_object_new(
			state, IVM_NULL, root
		), ivm_function_object_t));

		ivm_vmstate_setMainCoro(state, coro);
		// ivm_vmstate_setCurCoro(state, coro);
	}

	return state;
}
