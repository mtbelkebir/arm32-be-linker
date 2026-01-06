.section .text,"ax",%progbits
.arm
.global Target

Target:
    mov r1, #2
    bx lr
    