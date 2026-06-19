section .text
    global nasm_xor_encrypt_decrypt

nasm_xor_encrypt_decrypt:
    test rsi, rsi
    jz .done
.loop:
    mov al, byte [rdi]
    xor al, dl
    mov byte [rdi], al
    inc rdi
    dec rsi
    jnz .loop
.done:
    ret
