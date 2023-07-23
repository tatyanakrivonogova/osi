.data
    hello:
        .ascii "Hello world!\n"
        len = . - hello

.text
    .global _start
    _start:
        mov     $4, %rax
        mov     $1, %rbx
        mov     $hello, %rcx
        mov     $len, %rdx
        int $0x80

        mov     $1, %rax
        xor     %rdi, %rdi
        int $0x80

