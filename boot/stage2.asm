bits 16
org 0x7E00

    jmp short start
    nop
align 8
header:
magic:          db "SUBVERGE"
kernel_lba:     dd 0
kernel_sectors: dd 0
load_addr:      dd 0x100000
bss_end:        dd 0
entry:          dd 0

LOW_BUF   equ 0x10000
MB_INFO   equ 0x5000
MMAP      equ 0x6000

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [drive], dl

    in al, 0x92
    or al, 2
    and al, 0xFE
    out 0x92, al

    mov eax, [kernel_lba]
    mov [dap_lba], eax
    mov eax, [kernel_sectors]
    mov [remain], eax
    mov eax, [load_addr]
    mov [dst], eax
    mov word [dap_seg], LOW_BUF >> 4
.read:
    mov eax, [remain]
    test eax, eax
    jz .read_done
    cmp eax, 64
    jbe .n
    mov eax, 64
.n:
    mov [dap_count], ax
    mov [chunk], eax
    mov si, dap
    mov ah, 0x42
    mov dl, [drive]
    int 0x13
    jc fail
    mov eax, [chunk]
    sub [remain], eax
    add [dap_lba], eax
    call unreal
    mov esi, LOW_BUF
    mov edi, [dst]
    mov ecx, [chunk]
    shl ecx, 7
.cp:
    mov eax, [fs:esi]
    mov [fs:edi], eax
    add esi, 4
    add edi, 4
    dec ecx
    jnz .cp
    mov [dst], edi
    jmp .read
.read_done:

    xor ebx, ebx
    mov di, MMAP + 4
    xor bp, bp
.e820:
    mov eax, 0xE820
    mov ecx, 20
    mov edx, 0x534D4150           ; 'SMAP'
    int 0x15
    jc .e820_done
    cmp eax, 0x534D4150
    jne .e820_done
    mov dword [di - 4], 20
    add di, 24
    inc bp
    test ebx, ebx
    jz .e820_done
    cmp bp, 32
    jb .e820
.e820_done:
    mov ax, bp
    mov cx, 24
    mul cx
    mov [mb_mmap_len], ax

    mov si, MMAP
    mov cx, bp
.find:
    test cx, cx
    jz .found
    cmp dword [si + 4], 0x100000
    jne .next
    mov eax, [si + 12]
    shr eax, 10
    mov [mb_mem_upper], eax
.next:
    add si, 24
    dec cx
    jmp .find
.found:

    mov di, MB_INFO
    mov cx, 88
    xor al, al
    rep stosb
    mov dword [MB_INFO], (1 << 0) | (1 << 6) | (1 << 9)
    mov dword [MB_INFO + 4], 640
    mov eax, [mb_mem_upper]
    mov [MB_INFO + 8], eax
    movzx eax, word [mb_mmap_len]
    mov [MB_INFO + 44], eax
    mov dword [MB_INFO + 48], MMAP
    mov dword [MB_INFO + 64], loader_name

    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

; unreal mode copy
unreal:
    cli
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp short .p
.p:
    mov ax, 0x10
    mov fs, ax
    mov eax, cr0
    and eax, 0xFFFFFFFE
    mov cr0, eax
    jmp 0:.back
.back:
    xor ax, ax
    mov ds, ax
    mov es, ax
    sti
    ret

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

bits 32
pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, 0x7C00

    mov edi, [dst]
    cld
    mov ecx, [bss_end]
    sub ecx, edi
    xor eax, eax
    rep stosb

    mov eax, 0x2BADB002
    mov ebx, MB_INFO
    jmp [entry]

align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF          ; 32-bit code
    dq 0x00CF92000000FFFF          ; data
gdt_ptr:
    dw $ - gdt - 1
    dd gdt

drive: db 0
align 4
remain: dd 0
dst: dd 0
chunk: dd 0
mb_mem_upper: dd 0
mb_mmap_len: dw 0
msg: db "subverge: can't read the kernel", 0
loader_name: db "subverge stage2", 0

align 4
dap:
    db 0x10, 0
dap_count: dw 0
dap_off:   dw 0
dap_seg:   dw 0
dap_lba:   dq 0

times 8192 - ($ - $$) db 0
