bits 16
org 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [drive], dl

    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc fail
    mov dl, [drive]
    jmp 0:0x7E00

fail:
    mov si, msg
.print:
    lodsb
    test al, al
    jz .hang
    mov ah, 0x0E
    int 0x10
    jmp .print
.hang:
    hlt
    jmp .hang

drive: db 0
msg: db "subverge: can't read stage 2", 0

align 4
dap:
    db 0x10, 0
    dw 16                           ; sectors
    dw 0x7E00, 0                    ; offset, segment
    dq 1                            ; starting lba

times 510 - ($ - $$) db 0
dw 0xAA55
