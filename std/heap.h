#ifndef _IVM_STD_HEAP_H_
#define _IVM_STD_HEAP_H_

#include "pub/com.h"
#include "pub/type.h"
#include "pub/err.h"

#include "mem.h"

IVM_COM_HEADER

typedef struct ivm_heap_t_tag {
	ivm_int_t bcount;
	ivm_int_t btop; /* current block */
	ivm_size_t bsize;

	ivm_byte_t *bendp; /* current block end pointer */
	ivm_byte_t *bcurp; /* current pointer on the current block */

	ivm_byte_t **blocks;
} ivm_heap_t;

#define IVM_HEAP_GET_BLOCK_SIZE(heap) ((heap)->bsize)
#define IVM_HEAP_SET_BLOCK_SIZE(heap, val) ((heap)->bsize = (val))
#define IVM_HEAP_GET_BLOCK_COUNT(heap) ((heap)->bcount)
#define IVM_HEAP_GET_BLOCK_TOP(heap) ((heap)->btop)
#define IVM_HEAP_GET_CUR_PTR(heap) ((heap)->bcurp)

#define IVM_HEAP_GET_BLOCK_USED(heap) \
	((heap)->bsize * (heap)->btop \
	 + (ivm_ptr_t)(heap)->bcurp \
	 - (ivm_ptr_t)(heap)->blocks[(heap)->btop])

#define IVM_HEAP_GET_BLOCK_TOTAL(heap) ((heap)->bsize * (heap)->bcount)

#define IVM_HEAP_GET(obj, member) IVM_GET((obj), IVM_HEAP, member)
#define IVM_HEAP_SET(obj, member, val) IVM_SET((obj), IVM_HEAP, member, (val))

ivm_heap_t *
ivm_heap_new(ivm_size_t bsize);

void
ivm_heap_free(ivm_heap_t *heap);

void
ivm_heap_init(ivm_heap_t *heap,
			  ivm_size_t bsize);

void
ivm_heap_dump(ivm_heap_t *heap);

/* assert: size < bsize */
void *
_ivm_heap_addBlock(ivm_heap_t *heap, ivm_size_t size);

IVM_INLINE
ivm_bool_t
ivm_heap_isIllegalSize(ivm_heap_t *heap, ivm_size_t size)
{
	return size && size <= heap->bsize;
}

IVM_INLINE
ivm_size_t
ivm_heap_align4(ivm_size_t size)
{
	return size & 3 ? (size & ~((ivm_size_t)3)) + 4 : size;
}

IVM_INLINE
void *
ivm_heap_alloc_c(ivm_heap_t *heap, ivm_size_t size, ivm_bool_t *add_block)
{
	// IVM_ASSERT(size, IVM_ERROR_MSG_ILLEGAL_ALLOC_SIZE(size));
	register void *ret = heap->bcurp;

	size = ivm_heap_align4(size);

	if ((heap->bcurp += size) <= heap->bendp) {
		return ret;
	}
	
	*add_block = IVM_TRUE;
	
	return _ivm_heap_addBlock(heap, size);
}

IVM_INLINE
void *
ivm_heap_alloc(ivm_heap_t *heap, ivm_size_t size)
{
	register void *ret = heap->bcurp;

	size = ivm_heap_align4(size);

	if ((heap->bcurp += size) <= heap->bendp) {
		return ret;
	}

	return _ivm_heap_addBlock(heap, size);
}

IVM_INLINE
ivm_bool_t
ivm_heap_isIn(ivm_heap_t *heap, void *ptr)
{
	register ivm_size_t bsize = heap->bsize;
	register ivm_byte_t **i, **end;
	register ivm_ptrdiff_t tmp;

	for (i = heap->blocks,
		 end = i + heap->bcount;
		 i != end; i++) {
		tmp = (ivm_ptr_t)ptr - (ivm_ptr_t)*i;
		if (tmp >= 0 && tmp < bsize) return IVM_TRUE;
	}

	return IVM_FALSE;
}

IVM_INLINE
void *
ivm_heap_addCopy(ivm_heap_t *heap, void *ptr, ivm_size_t size)
{
	register void *new_ptr = ivm_heap_alloc(heap, size);

	STD_MEMCPY(new_ptr, ptr, size);

	return new_ptr;
}

IVM_INLINE
void *
ivm_heap_addCopy_c(ivm_heap_t *heap,
				   void *ptr,
				   ivm_size_t size,
				   ivm_bool_t *add_block)
{
	register void *new_ptr = ivm_heap_alloc_c(heap, size, add_block);

	STD_MEMCPY(new_ptr, ptr, size);

	return new_ptr;
}

void
ivm_heap_reset(ivm_heap_t *heap);

void
ivm_heap_compact(ivm_heap_t *heap);

IVM_COM_END

#endif
