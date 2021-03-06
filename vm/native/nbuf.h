#ifndef _IVM_VM_NATIVE_NBUF_H_
#define _IVM_VM_NATIVE_NBUF_H_

#include "pub/com.h"
#include "pub/type.h"
#include "pub/vm.h"

IVM_COM_HEADER

IVM_NATIVE_FUNC(_buffer_cons);
IVM_NATIVE_FUNC(_buffer_size);
IVM_NATIVE_FUNC(_buffer_to_s);
IVM_NATIVE_FUNC(_buffer_init);

IVM_COM_END

#endif
