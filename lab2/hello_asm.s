.data
    hello:
        .ascii "Hello world!\n"
        len = . - hello

.text
    .global _start
    _start:
        mov     $1, %rax
        mov     $1, %rdi
        mov     $hello, %rsi
        mov     $len, %rdx
        syscall

        mov     $60, %rax
        xor     %rdi, %rdi
        syscall

