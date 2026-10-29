/* SPDX-License-Identifier: GPL-3.0-or-later */
.syntax unified
.cpu cortex-m0plus
.thumb
.global isr_pendsv
.type isr_pendsv,%function
.thumb_func
isr_pendsv:
    mov r3, lr
    movs r2, #4
    tst r3, r2
    beq 1f
    mrs r0, psp
    subs r0, #32
    stmia r0!, {r4-r7}
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    stmia r0!, {r4-r7}
    subs r0, #32
    b 2f
1:
    movs r0, #0
2:
    push {r3, lr}
    bl os_schedule
    pop {r3}
    pop {r2}
    mov lr, r2
    ldr r4, [r0, #16]
    ldr r5, [r0, #20]
    ldr r6, [r0, #24]
    ldr r7, [r0, #28]
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    ldmia r0!, {r4-r7}
    adds r0, #16
    msr psp, r0
    ldr r1, =0xFFFFFFFD
    bx r1
.size isr_pendsv, .-isr_pendsv

.global isr_svcall
.type isr_svcall,%function
.thumb_func
isr_svcall:
    mov r2, lr
    movs r1, #4
    tst r2, r1
    beq 1f
    mrs r0, psp
    b 2f
1:
    mrs r0, msp
2:
    push {r2, lr}
    bl os_svc_dispatch
    pop {r2, pc}
.size isr_svcall, .-isr_svcall
