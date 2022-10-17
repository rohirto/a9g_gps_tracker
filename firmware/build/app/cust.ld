
/* Take into account assert bug in binutils-2.15*/






/*Mandatory to have the BB side irq handler linked */
ENTRY (user_Main)
/*ENTRY (aiuser_init)*/

SECTIONS
{
    /***************************************

    **               ROM                  **

    ***************************************/
    /* flash start address*/
    . = 0xFFFFFFFF88000000+0x00240000;
    __rom_start = .;
    /* RO */
    .section_init . : AT (ADDR(.section_init))
    {
        KEEP(*(.section_init))
        . = ALIGN(0x10);
    }
    .cust_rotext . : AT (ADDR(.cust_rotext))
    {
        *(.text)
        *(.text.*)
        *(.stub .gnu.linkonce.t.*)
        . = ALIGN(0x20);
    }
    .cust_rodata . : AT (ADDR(.cust_rodata))
    {
        *(.rdata)
        *(.rodata)
        *(.rodata.*)
        *(.rodata.*_png)
        *(.gnu.linkonce.r.*)
        *(.rodata1)
        *(.roresdata)
        *(.robsdata)
        . = ALIGN(0x20);
    }
 __user_rw_lma = .;
    __rom_size = __user_rw_lma - __rom_start;
    /* RAM */
    . = 0xFFFFFFFF82000000+0x00300000;
    /* RW */
    .cust_data . : AT ( __user_rw_lma )
    {
        __user_rw_start = .;
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
        *(.sdata .sdata.* .gnu.linkonce.s.*)
        *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
        . = ALIGN(0x10);
    }
    __user_rw_end = .;
    __user_rw_size = __user_rw_end - __user_rw_start;

    __user_bss_start = .;
    /* ZI */
    .cust_bss . : AT (ADDR(.cust_bss))
    {
        *(.sbss .sbss.* .gnu.linkonce.sb.*)
        *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*)
        *(.scommon)
        *(.dynbss)
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        *(.rambss)
        . = ALIGN(0x10);
    }
    __user_bss_end = .;
    __user_bss_size = __user_bss_end - __user_bss_start;
/* All sections we want to discard from the final linked elf.*/
    /*========================================= */
    /* Stabs debugging sections.                */
    /*========================================= */
    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .stab.excl 0 : { *(.stab.excl) }
    .stab.exclstr 0 : { *(.stab.exclstr) }
    .stab.index 0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment 0 : { *(.comment) }
    /* DWARF debug sections.

       Symbols in the DWARF debugging sections are relative to the beginning

       of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug 0 : { *(.debug) }
    .line 0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo 0 : { *(.debug_srcinfo) }
    .debug_sfnames 0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges 0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info 0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_line 0 : { *(.debug_line) }
    .debug_frame 0 : { *(.debug_frame) }
    .debug_str 0 : { *(.debug_str) }
    .debug_loc 0 : { *(.debug_loc) }
    .debug_macinfo 0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames 0 : { *(.debug_varnames) }
    /* These must appear regardless of  .  */
    .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
    .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
    /DISCARD/ :
    {
        /* We don't build a relocatable object */
        *(.rel.dyn)
        *(.note.GNU-stack)
        /* The eh_frame data is used to unwind the stack when an exception is

         * thrown. It records things like where to find the return address and

         * which registers are stored where.

         * (From: http://archives.devshed.com/forums/development-94/how-is-the-eh-frame-section-used-2316212.html

         */
        *(.eh_frame)
     /* C++ constructors. It should not be used. */
     /*(.ctors)*/
    }
}
