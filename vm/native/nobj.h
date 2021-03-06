#ifndef _IVM_VM_NATIVE_NOBJ_H_
#define _IVM_VM_NATIVE_NOBJ_H_

#include "pub/com.h"
#include "pub/type.h"
#include "pub/vm.h"

IVM_COM_HEADER

IVM_NATIVE_FUNC(_object_cons);
IVM_NATIVE_FUNC(_object_merge);
IVM_NATIVE_FUNC(_object_clone);
IVM_NATIVE_FUNC(_object_call);
IVM_NATIVE_FUNC(_object_to_s);
IVM_NATIVE_FUNC(_object_slots);
// IVM_NATIVE_FUNC(_object_type);

IVM_COM_END

#endif
