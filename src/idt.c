#include "header/cpu/idt.h"
#include "header/cpu/gdt.h"

// Global interrupt descriptor table
struct InterruptDescriptorTable interrupt_descriptor_table;

// IDTR untuk memuat IDT
struct IDTR _idt_idtr = {
    .size = sizeof(struct InterruptDescriptorTable) - 1,
    .address = &interrupt_descriptor_table,
};

// Menginisialisasi Interrupt Descriptor Table (IDT)
void initialize_idt(void)
{
    int i;
    // Menginisialisasi interrupt gates untuk ISR stubs
    for (i = 0; i < ISR_STUB_TABLE_LIMIT; i++)
    {
        set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 0);
    }

    // Set interrupt gate untuk syscalls
    set_interrupt_gate(0x30, isr_stub_table[0x30], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 3);

    // Memuat IDT dan melakukan enable untuk interrupts
    __asm__ volatile("lidt %0" : : "m"(_idt_idtr));
    __asm__ volatile("sti");
}

// Set interrupt gate di IDT
void set_interrupt_gate(
    uint8_t int_vector,
    void *handler_address,
    uint16_t gdt_seg_selector,
    uint8_t privilege)
{
    struct IDTGate *idt_int_gate = &interrupt_descriptor_table.table[int_vector];
    // Menggunakan &-bitmask, bitshift, dan casting untuk offset
    idt_int_gate->offset_low = (uint32_t)handler_address & 0xFFFF;
    idt_int_gate->offset_high = ((uint32_t)handler_address >> 16) & 0xFFFF;
    idt_int_gate->segment = gdt_seg_selector;
    idt_int_gate->_reserved = 0;
    idt_int_gate->dpl = privilege;

    // Target system 32-bit dan membuat flag sebagai valid interrupt gate
    idt_int_gate->_r_bit_1 = INTERRUPT_GATE_R_BIT_1;
    idt_int_gate->_r_bit_2 = INTERRUPT_GATE_R_BIT_2;
    idt_int_gate->_r_bit_3 = INTERRUPT_GATE_R_BIT_3;
    idt_int_gate->gate_32 = 1;
    idt_int_gate->valid_bit = 1;
}
