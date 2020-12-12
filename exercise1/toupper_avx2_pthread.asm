global toupper_avx2_pthread
section .text
toupper_avx2_pthread:
    push rbx
    mov rbx,  [rdi]
    mov esi,  [rdi +8] 
    test esi,esi
    jle _p_exit
    shr esi,5 ; divide by vector size
    vpcmpeqb ymm3,ymm3,ymm3 ; set 1's. ymm's are 256 bit(32 byte) long 
    vmovdqa ymm4, [_lc_a] ; as
    vmovdqa ymm5, [_lc_z] ; z's
    vmovdqa ymm6, [_lc_sub_32] ; -32's
_loop:
    vmovdqa ymm7, [rbx]
    vpcmpgtb ymm0, ymm7, ymm4 ; > 'a' -1
    vpcmpgtb ymm1, ymm7, ymm5 ; > 'z' 
    vpandn  ymm1, ymm3 ; invert z's i.e <= 'z'
    vpand ymm1, ymm0 ; > 'a' -1 & <= 'z'
    vpand ymm1, ymm6 ; -32 mask 
    vpsubb ymm7, ymm1
    vmovdqa [rbx], ymm7
    add rbx,32 ; add 32 bytes
    dec esi
    jnz _loop
_p_exit:
    pop rbx
    ret
section .data
 align 16
_lc_a : 
dq 0x6060606060606060
dq 0x6060606060606060
dq 0x6060606060606060
dq 0x6060606060606060
_lc_z :
dq 0x7A7A7A7A7A7A7A7A
dq 0x7A7A7A7A7A7A7A7A
dq 0x7A7A7A7A7A7A7A7A
dq 0x7A7A7A7A7A7A7A7A
_lc_sub_32 :
dq 0x2020202020202020
dq 0x2020202020202020
dq 0x2020202020202020
dq 0x2020202020202020
