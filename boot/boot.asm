
MB_MAGIC equ 0x1BADB002
MB_FLAGS equ (1 << 0) | (1 << 1)

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd -(MB_MAGIC + MB_FLAGS)

section .bss
align 4096
pml4:   resb 4096
pdpt:   resb 4096
pd:     resb 4096 * 4
align 16
boot_stack: resb 262144
boot_stack_top:

section .data
align 8
gdt64:
    dq 0
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt64_ptr:
    dw $ - gdt64 - 1
    dq gdt64

section .text
bits 32
global _start
_start:
    mov esp, boot_stack_top
    mov edi, eax
    mov esi, ebx

    mov eax, pdpt
    or eax, 3
    mov [pml4], eax
    xor ecx, ecx
.pdpt:
    mov eax, ecx
    shl eax, 12
    add eax, pd
    or eax, 3
    mov [pdpt + ecx * 8], eax
    inc ecx
    cmp ecx, 4
    jne .pdpt

    xor ecx, ecx
.pd:
    mov eax, ecx
    shl eax, 21
    or eax, 0x83
    mov [pd + ecx * 8], eax
    mov eax, ecx
    shr eax, 11
    mov [pd + ecx * 8 + 4], eax
    inc ecx
    cmp ecx, 2048
    jne .pd

    mov eax, pml4
    mov cr3, eax
    mov eax, cr4
    or eax, 1 << 5                   ; PAE
    mov cr4, eax
    mov ecx, 0xC0000080              ; EFER
    rdmsr
    or eax, 1 << 8                   ; long mode enable
    wrmsr
    mov eax, cr0
    or eax, (1 << 31) | 1
    mov cr0, eax
    lgdt [gdt64_ptr]
    jmp 0x08:long_mode

bits 64
extern kmain
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, boot_stack_top
    mov edi, edi
    mov esi, esi
    call kmain
.hang:
    cli
    hlt
    jmp .hang

global idt_load
idt_load:
    lidt [rdi]
    ret

%assign i 0
%rep 49
global isr %+ i
isr %+ i:
%if i = 8 || i = 10 || i = 11 || i = 12 || i = 13 || i = 14 || i = 17 || i = 21 || i = 29 || i = 30
%else
    push qword 0
%endif
    push qword i
    jmp isr_common
%assign i i + 1
%endrep

extern isr_handler
isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, rsp
    call isr_handler
    mov rsp, rax
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16
    iretq

section .data
global isr_table
isr_table:
%assign i 0
%rep 49
    dq isr %+ i
%assign i i + 1
%endrep

section .note.GNU-stack noalloc noexec nowrite progbits
