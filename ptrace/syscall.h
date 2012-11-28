/* 
   stolen from: http://hssl.cs.jhu.edu/~neal/woodchuck/src/commits/c8a6c6c6d28c87e2ef99e21cf76c4ea90a7e11ad/src/process-monitor-ptrace.c.raw.html
 */

#if defined(__x86_64__)
# define REGS_STRUCT struct user_regs_struct
# define REGS_IP rip
# define REGS_SYSCALL rax
#elif defined(__i386__)
# define REGS_STRUCT struct user_regs_struct
# define REGS_IP eip
# define REGS_SYSCALL eax
#elif defined(__arm__)
# define REGS_STRUCT struct pt_regs
# define REGS_IP ARM_pc
# define REGS_SYSCALL ARM_r7
#endif

#if defined(__x86_64__)
# define SYSCALL (regs.orig_rax)
# define ARG1 (regs.rdi)
# define ARG2 (regs.rsi)
# define ARG3 (regs.rdx)
# define ARG4 (regs.r10)
# define ARG5 (regs.r8)
# define ARG6 (regs.r9)
# define RET (regs.rax)
# define REGS_FMT "rip: %"PRIxPTR"; rsp: %"PRIxPTR"; rax (ret): %"PRIxPTR"; " \
	"rbx: %"PRIxPTR"; rcx: %"PRIxPTR"; rdx (3): %"PRIxPTR"; "	\
	"rdi (0): %"PRIxPTR"; rsi (1): %"PRIxPTR"; r8 (5): %"PRIxPTR"; " \
	"r9 (6): %"PRIxPTR"; r10 (4): %"PRIxPTR"; r11: %"PRIxPTR"; "	\
	"r12: %"PRIxPTR"; r13: %"PRIxPTR"; r14: %"PRIxPTR"; "		\
 	"r15: %"PRIxPTR"; orig rax (syscall): %"PRIxPTR
# define REGS_PRINTF(regs) (regs)->rip, (regs)->rsp, (regs)->rax,	\
	(regs)->rbx, (regs)->rcx, (regs)->rdx,				\
	(regs)->rdi, (regs)->rsi, (regs)->r8,				\
	(regs)->r9, (regs)->r10, (regs)->r11,				\
	(regs)->r12, (regs)->r13, (regs)->r14,				\
	(regs)->r15, (regs)->orig_rax

/* On x86-64, RAX is set to -ENOSYS on system call entry.  How
   do we distinguish this from a system call that returns
   ENOSYS?  (marek: we don't) */
# define SYSCALL_ENTRY (RET == -ENOSYS)

#elif defined(__i386__)
# define SYSCALL (regs.orig_eax)
# define ARG1 (regs.ebx)
# define ARG2 (regs.ecx)
# define ARG3 (regs.edx)
# define ARG4 (regs.esi)
# define ARG5 (regs.edi)
# define ARG6 (regs.ebp)
# define RET (regs.eax)
# define SYSCALL_ENTRY (RET == -ENOSYS)

#elif defined(__arm__)
# define REGS_FMT "pc: %lx; sp: %lx; r0 (ret): %lx; " \
	"r1 (2): %lx; r2 (3): %lx; r3 (4): %lx; "	\
	"r4 (5): %lx; r5 (6): %lx; r6: %lx; "	\
	"r7: %lx; r8: %lx; r9: %lx; "	\
	"r10: %lx; orig r0 (1): %lx; ip: %lx"
# define REGS_PRINTF(regs) (regs)->ARM_pc, (regs)->ARM_sp, (regs)->ARM_r0, \
	(regs)->ARM_r0, (regs)->ARM_r1, (regs)->ARM_r2,			\
	(regs)->ARM_r3, (regs)->ARM_r4, (regs)->ARM_r5, \
	(regs)->ARM_r7, (regs)->ARM_r8, (regs)->ARM_r9, \
	(regs)->ARM_r10, (regs)->ARM_ORIG_r0, (regs)->ARM_ip

      /* This layout assumes that there are no 64-bit parameters.  See
	 http://lkml.org/lkml/2006/1/12/175 for the complications.  */
      /* r7 */
# define SYSCALL (regs.ARM_r7)
      /* r0..r6 */
# define ARG1 (regs.ARM_ORIG_r0)
# define ARG2 (regs.ARM_r1)
# define ARG3 (regs.ARM_r2)
# define ARG4 (regs.ARM_r3)
# define ARG5 (regs.ARM_r4)
# define ARG6 (regs.ARM_r5)
# define RET (regs.ARM_r0)
/* ip is set to 0 on system call entry, 1 on exit.  */
# define SYSCALL_ENTRY (regs.ARM_ip == 0)
#else
# error Not ported to your architecture.
#endif

