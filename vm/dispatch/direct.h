#ifndef _IVM_VM_DISPATCH_DIRECT_H_
#define _IVM_VM_DISPATCH_DIRECT_H_

#define _CORO (coro)
#define _RUNTIME (tmp_runtime)
#define _CONTEXT (tmp_context)
#define _STATE (state)
#define _STRING_POOL (tmp_exec->pool)
#define _INSTR (tmp_ip)

#define NEXT_INSTR() \
	if (++tmp_ip != tmp_ip_end) { \
		if (ivm_vmstate_checkGC(state)) { \
			SAVE_STACK(); \
			ivm_vmstate_doGC(state); \
		} \
		goto *(tmp_ip->entry); \
	} else { \
		goto END_EXEC; \
	}

#define NEXT_N_INSTR(n) \
	if ((tmp_ip += (n)) != tmp_ip_end) { \
		goto *(tmp_ip->entry); \
	} else { \
		goto END_EXEC; \
	} \

#define YIELD() tmp_ip++; SAVE_RUNTIME(_RUNTIME, tmp_ip); goto ACTION_YIELD
#define RETURN() tmp_ip++; SAVE_RUNTIME(_RUNTIME, tmp_ip); goto ACTION_RETURN
#define INVOKE() goto ACTION_INVOKE

#define _ARG (_INSTR->arg)

#define STACK_TOP_NOCACHE() (ivm_vmstack_at(_STACK, tmp_sp - 1))
#define STACK_PUSH_NOCACHE(obj) (ivm_vmstack_pushAt(_STACK, tmp_sp, (obj)), ++tmp_sp)
#define STACK_POP_NOCACHE() (tmp_sp--, ivm_vmstack_at(_STACK, tmp_sp))

#define _STACK (tmp_stack)
#define STACK_SIZE() (tmp_sp + cst)

#define STACK_INC(i) (tmp_sp += (i))

#if IVM_STACK_CACHE_N_TOS == 1

	#define STC_PUSHBACK() \
		(cst ? (STACK_PUSH_NOCACHE(stc0), cst = 0) : 0)

	#define STACK_TOP() \
		(cst ? stc0 : STACK_TOP_NOCACHE())

	#define STACK_POP() \
		(cst ? ((cst = 0), stc0) : STACK_POP_NOCACHE())

	#define STACK_PUSH(obj) \
		(((cst) ? STC_PUSHBACK() : 0), (stc0 = (obj)), (cst = 1))

	#define STACK_BEFORE(i) \
		((i) < cst ? stc0 : ivm_vmstack_at(_STACK, tmp_sp - 1 - (i) + cst))

	#define STACK_CUT(i) \
		((i) ? (STC_PUSHBACK(), (tmp_sp -= (i)), ivm_vmstack_ptrAt(_STACK, tmp_sp)) : IVM_NULL)

#elif IVM_STACK_CACHE_N_TOS == 2

	/* stack cache */
	#define STC_PUSHBACK() \
		({if (cst) { \
			if (cst == 1) { \
				STACK_PUSH_NOCACHE(stc0); \
			} else { /* cst == 2 */ \
				STACK_PUSH_NOCACHE(stc0); \
				STACK_PUSH_NOCACHE(stc1); \
			} \
			cst = 0; \
		}}) \

	#define _if		((
	#define _then	)?(
	#define _else	):(
	#define _end	))

	#define STACK_TOP() \
		_if (cst == 2) _then                 \
			stc1                             \
		_else                                \
			_if (cst == 1) _then             \
				stc0                         \
			_else                            \
				STACK_TOP_NOCACHE()          \
			_end                             \
		_end

	#define STACK_POP() \
		_if (cst == 2) _then                 \
			(cst = 1),                       \
			stc1                             \
		_else                                \
			_if (cst == 1) _then             \
				(cst = 0),                   \
				stc0                         \
			_else                            \
				STACK_POP_NOCACHE()          \
			_end                             \
		_end

	#define STACK_PUSH(obj) \
		_if (cst == 2) _then                \
			STC_PUSHBACK(),                 \
			(cst = 1),                      \
			(stc0 = (obj))                  \
		_else                               \
		  	_if (cst == 1) _then            \
		  		(cst = 2),                  \
		  		(stc1 = (obj))              \
		  	_else                           \
		  		(cst = 1),                  \
		  		(stc0 = (obj))              \
		  	_end                            \
		_end

	#define STACK_BEFORE(i) \
		_if ((i) < cst) _then                                \
			_if (!(i)) _then                                 \
				STACK_TOP()                                  \
			_else                                            \
				stc0                                         \
			_end                                             \
		_else                                                \
			ivm_vmstack_at(_STACK, tmp_sp - 1 - (i) + cst)   \
		_end

	#define STACK_CUT(i) \
		_if (i) _then                                        \
			STC_PUSHBACK(),                                  \
			(tmp_sp -= (i)),                                 \
			ivm_vmstack_ptrAt(_STACK, tmp_sp)                \
		_else                                                \
			IVM_NULL                                         \
		_end

#else
	#error unsupported stack cache number
#endif

#define AVAIL_STACK (tmp_sp - tmp_bp + cst)
#define CHECK_STACK(req) \
	IVM_ASSERT(AVAIL_STACK >= (req), \
			   IVM_ERROR_MSG_INSUFFICIENT_STACK((req), AVAIL_STACK))

#define SAVE_RUNTIME(runtime, ip) \
	(STC_PUSHBACK(), \
	 IVM_RUNTIME_SET((runtime), IP, (ip)), \
	 IVM_RUNTIME_SET((runtime), BP, tmp_bp), \
	 IVM_RUNTIME_SET((runtime), SP, tmp_sp))

#define SAVE_STACK() \
	(STC_PUSHBACK(), \
	 IVM_RUNTIME_SET(_RUNTIME, BP, tmp_bp), \
	 IVM_RUNTIME_SET(_RUNTIME, SP, tmp_sp))

#define UPDATE_STACK() \
	(tmp_bp = IVM_RUNTIME_GET(_RUNTIME, BP), \
	 tmp_sp = IVM_RUNTIME_GET(_RUNTIME, SP))

#endif
