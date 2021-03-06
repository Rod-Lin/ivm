#ifndef _IVM_VM_SLOT_H_
#define _IVM_VM_SLOT_H_

#include "pub/com.h"
#include "pub/type.h"

#include "std/hash.h"
#include "std/string.h"
#include "std/uid.h"
#include "std/bit.h"

#include "instr.h"
#include "oprt.h"

IVM_COM_HEADER

struct ivm_object_t_tag;
struct ivm_vmstate_t_tag;
struct ivm_slot_t_tag;
struct ivm_heap_t_tag;

typedef struct ivm_slot_t_tag {
	const ivm_string_t *k;
	struct ivm_object_t_tag *v;
} ivm_slot_t;

IVM_INLINE
void
_ivm_slot_setValue(ivm_slot_t *slot,
				   struct ivm_object_t_tag *obj)
{
	slot->v = obj;
	return;
}

IVM_INLINE
struct ivm_object_t_tag *
ivm_slot_getValue(ivm_slot_t *slot)
{
	return slot ? slot->v : IVM_NULL;
}

#define _IVM_SLOT_TABLE_MARK_HEADER_BITS 5

typedef struct ivm_slot_table_t_tag {
	ivm_size_t size;
	ivm_slot_t *tabl;
	// struct ivm_object_t_tag **oops;
	ivm_uint64_t block_oop; // number of bits of it should be GE the max count of oop(64)
	union {
		struct {
			ivm_int_t dummy1;
			ivm_int_t dummy2: 32 - _IVM_SLOT_TABLE_MARK_HEADER_BITS;
			ivm_uint_t is_hash: 1;
			ivm_uint_t is_shared: 1; // shared by multiple objects, need to copy on write
			ivm_uint_t no_cow: 1;
			ivm_uint_t wb: 1;
			ivm_uint_t gen: 1;
		} sub;
		struct ivm_slot_table_t_tag *copy;
	} mark;
	ivm_uid_t uid;
	// ivm_int_t cid; // collect id
} ivm_slot_table_t;

IVM_INLINE
ivm_slot_table_t *
ivm_slot_table_getCopy(ivm_slot_table_t *table)
{
	if (IVM_IS64) {
		return (ivm_slot_table_t *)((((ivm_uptr_t)table->mark.copy)
									  << _IVM_SLOT_TABLE_MARK_HEADER_BITS)
									  >> _IVM_SLOT_TABLE_MARK_HEADER_BITS);
	}

	return table->mark.copy;
}

IVM_INLINE
void
ivm_slot_table_setCopy(ivm_slot_table_t *table,
					   ivm_slot_table_t *copy)
{
	if (IVM_IS64) {
		table->mark.copy = (ivm_slot_table_t *)
						   ((((ivm_uptr_t)table->mark.copy
						   	>> (sizeof(ivm_ptr_t) * 8 - _IVM_SLOT_TABLE_MARK_HEADER_BITS))
							<< (sizeof(ivm_ptr_t) * 8 - _IVM_SLOT_TABLE_MARK_HEADER_BITS))
							| (ivm_uptr_t)copy);
	} else {
		table->mark.copy = copy;
	}

	return;
}

// #undef _IVM_SLOT_TABLE_MARK_HEADER_BITS

#define ivm_slot_table_getWB(table) ((table)->mark.sub.wb)
#define ivm_slot_table_setWB(table, val) ((table)->mark.sub.wb = (val))
#define ivm_slot_table_getGen(table) ((table)->mark.sub.gen)
#define ivm_slot_table_setGen(table, val) ((table)->mark.sub.gen = (val))
#define ivm_slot_table_incGen(table) (++(table)->mark.sub.gen)

// #define ivm_slot_table_setCID(table, id) ((table)->cid = (id))
// #define ivm_slot_table_checkCID(table, id) ((table)->cid == (id))

#define ivm_slot_table_updateUID(table, state) \
	((table)->uid = ivm_vmstate_genSTUID(state))

ivm_slot_table_t *
ivm_slot_table_new(struct ivm_vmstate_t_tag *state);

ivm_slot_table_t *
ivm_slot_table_newAt(struct ivm_vmstate_t_tag *state, ivm_int_t gen);

ivm_slot_table_t *
ivm_slot_table_new_c(struct ivm_vmstate_t_tag *state,
					 ivm_size_t prealloc);

ivm_slot_table_t *
ivm_slot_table_newAt_c(struct ivm_vmstate_t_tag *state,
					   ivm_size_t prealloc,
					   ivm_int_t gen);

ivm_slot_table_t *
ivm_slot_table_copy(ivm_slot_table_t *table,
					struct ivm_vmstate_t_tag *state,
					struct ivm_heap_t_tag *heap);

/* merge tb to tas */
void
ivm_slot_table_merge(ivm_slot_table_t *ta,
					 struct ivm_vmstate_t_tag *state,
					 ivm_slot_table_t *tb,
					 ivm_bool_t overw);

ivm_slot_table_t *
ivm_slot_table_copy_state(ivm_slot_table_t *table,
						  struct ivm_vmstate_t_tag *state);

#define ivm_slot_table_isShared(table) ((table)->mark.sub.is_shared)
#define ivm_slot_table_setNoCOW(table) ((table)->mark.sub.no_cow = IVM_TRUE)
#define ivm_slot_Table_isNoCOW(table) ((table)->mark.sub.no_cow)

IVM_INLINE
ivm_slot_table_t *
ivm_slot_table_copyOnWrite(ivm_slot_table_t *table,
						   struct ivm_vmstate_t_tag *state)
{
	// IVM_TRACE("COW!!\n");
	if (ivm_slot_table_isShared(table)) {
		return ivm_slot_table_copy_state(table, state);
	}

	return table;
}

IVM_INLINE
ivm_slot_table_t *
ivm_slot_table_copyShared(ivm_slot_table_t *table,
						  struct ivm_vmstate_t_tag *state)
{
	if (table) {
		if (ivm_slot_Table_isNoCOW(table)) {
			table = ivm_slot_table_copy_state(table, state);
		} else {
			table->mark.sub.is_shared = IVM_TRUE;
		}

		return table;
	}
	
	return IVM_NULL;
}

void
ivm_slot_table_expandTo(ivm_slot_table_t *table,
						struct ivm_vmstate_t_tag *state,
						ivm_size_t to);

void
_ivm_slot_table_expandOopTo(ivm_slot_table_t *table,
							struct ivm_vmstate_t_tag *state,
							ivm_size_t size);

typedef ivm_slot_t *ivm_slot_table_iterator_t;

#define IVM_SLOT_TABLE_ITER_SET_KEY(iter, key) ((iter)->k = (key))
#define IVM_SLOT_TABLE_ITER_SET_VAL(iter, val) ((iter)->v = (val))
#define IVM_SLOT_TABLE_ITER_SET(iter, key, val) ((iter)->k = (key), (iter)->v = (val))
#define IVM_SLOT_TABLE_ITER_GET_KEY(iter) ((iter)->k)
#define IVM_SLOT_TABLE_ITER_GET_VAL(iter) ((iter)->v)
#define IVM_SLOT_TABLE_EACHPTR(table, iter) \
	ivm_slot_t *__sl_end_##iter##__; \
	for ((iter) = (table)->tabl, \
		 __sl_end_##iter##__ = (iter) + (table)->size; \
		 (iter) != __sl_end_##iter##__; \
		 (iter)++) if ((iter)->k && (iter)->v)

/* be sure table is not null */
IVM_INLINE
ivm_bool_t
ivm_slot_table_checkCacheValid(ivm_slot_table_t *table,
							   ivm_instr_t *instr)
{
	return ivm_instr_cacheID(instr) == table->uid;
}

#define ivm_slot_table_getCacheSlot(state, instr) \
	((ivm_slot_t *)ivm_instr_cacheData(instr))

#define ivm_slot_table_setCacheSlot(state, instr, value) \
	(_ivm_slot_setValue((ivm_slot_t *)ivm_instr_cacheData(instr), (value)))

// #define ivm_slot_table_getOops(table) ((table)->oops)
// #define ivm_slot_table_getOopCount(table) ((table)->mark.sub.oop_count)

void
ivm_slot_table_setOop(ivm_slot_table_t *table,
					  struct ivm_vmstate_t_tag *state,
					  ivm_int_t op,
					  struct ivm_object_t_tag *func);

struct ivm_object_t_tag *
ivm_slot_table_getOop(ivm_slot_table_t *table,
					  struct ivm_vmstate_t_tag *state,
					  ivm_int_t op);

#define ivm_slot_table_hasBlockedOop(table, op) (((table)->block_oop & (1 << (op))) != 0)

IVM_COM_END

#endif
