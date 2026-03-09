.global _main
.extern _puts, _exit

_main:
    adr x0, 1f
    bl _printf

    b 2f
1:
    .asciz "Hello, world!\n"
    .align 4
2:
    ldr x0, =0
    bl _exit
