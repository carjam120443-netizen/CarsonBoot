.text
.global cb_cpuid
.type cb_cpuid, @function

/*
 * void cb_cpuid(uint32_t leaf, uint32_t subleaf, uint32_t out[4]);
 *
 * System V AMD64 calling convention:
 *   EDI = leaf
 *   ESI = subleaf
 *   RDX = output array [EAX, EBX, ECX, EDX]
 */
cb_cpuid:
    movq %rdx, %r8
    movl %edi, %eax
    movl %esi, %ecx
    pushq %rbx
    cpuid
    movl %eax, 0(%r8)
    movl %ebx, 4(%r8)
    movl %ecx, 8(%r8)
    movl %edx, 12(%r8)
    popq %rbx
    ret

.size cb_cpuid, .-cb_cpuid
.section .note.GNU-stack,"",@progbits
