#ifndef _IVM_VM_NATIVE_NSTROBJ_H_
#define _IVM_VM_NATIVE_NSTROBJ_H_

#include "pub/com.h"
#include "pub/type.h"
#include "pub/vm.h"

IVM_COM_HEADER

IVM_NATIVE_FUNC(_string_cons);
IVM_NATIVE_FUNC(_string_len);
IVM_NATIVE_FUNC(_string_ord);
IVM_NATIVE_FUNC(_string_ords);
IVM_NATIVE_FUNC(_string_chars);
IVM_NATIVE_FUNC(_string_to_s);
IVM_NATIVE_FUNC(_string_split);

IVM_COM_END

#endif
