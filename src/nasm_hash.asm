section .text
    global safe_nasm_spatial_hash

safe_nasm_spatial_hash:
    mov rax, rdi
    imul rax, 73856093
    mov rcx, rsi
    imul rcx, 19349663
    xor rax, rcx
    
    mov r8, rdx
    xor rdx, rdx
    div r8
    mov rax, rdx
    ret
