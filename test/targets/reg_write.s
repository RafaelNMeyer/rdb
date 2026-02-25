.global main

.section .data

hex_format: .asciz "%#x"
float_format: .asciz "%.2f"
long_float_format: .asciz "%.2Lf"

.section .text

.macro trap
	movq $62, %rax 	# kill syscall
	movq %r12, %rdi # pid to rdi
	movq $5, %rsi 	# sigtrap signal
	syscall
.endm

main:
	push %rbp
	movq %rsp, %rbp

	# Get PID
	movq $39, %rax
	syscall
	movq %rax, %r12

	trap # after the trap we assume that rsi register has been written

	# Print rsi contents
	leaq hex_format(%rip), %rdi # -pie flag uses RIP-relative address
	movq $0, %rax # 0 to rax since printf requires a varargs, we do not use it, so 0 varargs
	call printf@plt
	movq $0, %rdi # 0 to rdi, fflush expect it
	call fflush@plt

	trap

	# after trap we assume that mm0 has been written and move it to rsi
	movq %mm0, %rsi
	leaq hex_format(%rip), %rdi
	movq $0, %rax
	call printf@plt
	movq $0, %rdi # 0 to rdi, fflush expect it
	call fflush@plt

	trap

	# now we are writing to a floating pointer register
	# xmm0 is a vector register and sysv abi printf expect it to receive from varargs
	# so we put 1 to rax register
	leaq float_format(%rip), %rdi
	movq $1, %rax
	call printf@plt
	movq $0, %rdi
	call fflush@plt

	trap

	# here we assume that the FPU (Float Pointer stack unit) has a value
	# on top of it
	# print contents of st0

	subq $16, %rsp
	fstpt (%rsp) # send from st0 (FPU) to top of stack frame
	leaq long_float_format(%rip), %rdi
	movq $0, %rax
	call printf@plt
	movq $0, %rdi
	call fflush@plt
	addq $16, %rsp

	popq %rbp
	movq $0, %rax
	ret
