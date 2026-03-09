.global _main
.extern _puts, _exit

_main:
    adrp x0, _1.str@PAGE
    add x0, x0, _1.str@PAGEOFF
    bl _puts

    ldr x0, =0
    bl _exit

.text
_1.str:
    .asciz "Hello, world\n"
