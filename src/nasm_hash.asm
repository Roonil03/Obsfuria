section .text
    global safe_nasm_spatial_hash

safe_nasm_spatial_hash:
    ; SysV leaves the upper halves of 32-bit arguments unspecified. Preserve
    ; the existing zero-extended input convention before 64-bit multiplication.
    mov eax, edi
    imul rax, 73856093
    mov ecx, esi
    imul rcx, 19349663
    xor rax, rcx
    
    mov r8d, edx
    xor rdx, rdx
    div r8
    mov rax, rdx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
