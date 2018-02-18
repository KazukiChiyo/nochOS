# paging_asm.S: implementation of paging.h function
# author: Zhengcheng Huang
# date: 10/19/2017
# external source: wiki.osdev.org/Paging

.global enable_paging
.global flush_tlb

flush_tlb:
    # Loading page directory to cr3
    leal    page_directory,%eax
    movl    %eax,%cr3
    ret

#
# Enables paging by
#   1. Setting CR3 to the address of the page directory
#   2. Enabling 4MB paging by setting CR4 flag
#   3. Enabling paging by setting CR0 flag
#
# - author: Zhengcheng Huang
# - side effects: Sets CR0, CR3, and CR4.
#
enable_paging:
    # Loading page directory to cr3
    leal    page_directory,%eax
    movl    %eax,%cr3

    # Enable 4MB pages
    movl    %cr4,%eax
    orl     $0x10,%eax
    movl    %eax,%cr4

    # Enable paging
    movl    %cr0,%eax
    orl     $0x80000000,%eax
    movl    %eax,%cr0

    ret

