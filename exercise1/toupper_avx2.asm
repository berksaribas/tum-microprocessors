global toupper_avx2
section .text
toupper_avx2:
    push rbx
    mov rbx,rdi
    mov rcx,rsi
    test rsi,rsi
    jle p_exit
    and rcx,31 ; get module 31
    shr rsi,5 ; divide by vector size
    jz uwu
    vpcmpeqb ymm3,ymm3,ymm3 ; set 1's. ymm's are 256 bit(32 byte) long 
    vmovdqa ymm4, [lc_a] ; as
    vmovdqa ymm5, [lc_z] ; z's
    vmovdqa ymm6, [lc_sub_32] ; -32's
loop:
    vmovdqa ymm7, [rbx]
    vpcmpgtb ymm0, ymm7, ymm4 ; > 'a' -1
    vpcmpgtb ymm1, ymm7, ymm5 ; > 'z' 
    vpandn  ymm1, ymm3 ; invert z's i.e <= 'z'
    vpand ymm1, ymm0 ; > 'a' -1 & <= 'z'
    vpand ymm1, ymm6 ; -32 mask 
    vpsubb ymm7, ymm1
    vmovdqa [rbx], ymm7
    add rbx,32 ; add 32 bytes
    dec rsi
    jnz loop
uwu:
    movzx eax, byte [rbx]
    test al, al
    je p_exit
loop2:
    lea edx, [rax - 97]
    cmp dl, 25
    ja loop3
    sub eax, 32
    mov byte  [rbx], al
loop3:
    add rbx, 1
    movzx eax, byte [rbx]
    test al, al
    jne loop2
p_exit:
    pop rbx
    ret

section .data
 align 16
lc_a : 
dq 0x6060606060606060
dq 0x6060606060606060
dq 0x6060606060606060
dq 0x6060606060606060
lc_z :
dq 0x7A7A7A7A7A7A7A7A
dq 0x7A7A7A7A7A7A7A7A
dq 0x7A7A7A7A7A7A7A7A
dq 0x7A7A7A7A7A7A7A7A
lc_sub_32 :
dq 0x2020202020202020
dq 0x2020202020202020
dq 0x2020202020202020
dq 0x2020202020202020
