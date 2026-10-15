ENTRY(_start)

SECTIONS
{
    . = 1M;
    _kernel_start = .;
    .text BLOCK(4K) : ALIGN(4K) { *(.multiboot) *(.text*) }
    .rodata BLOCK(4K) : ALIGN(4K) { *(.rodata*) }
    .data BLOCK(4K) : ALIGN(4K) { _data_start = .; *(.data*) _data_end = .; }
    .bss BLOCK(4K) : ALIGN(4K) { _bss_start = .; *(COMMON) *(.bss*) }
    _kernel_end = .;
    /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame) }
}
