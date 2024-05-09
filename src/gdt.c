#include "header/cpu/gdt.h"
#include "header/cpu/interrupt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
static struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {/* TODO: Null Descriptor */},
        {/* TODO: Kernel Code Descriptor */},
        {/* TODO: Kernel Data Descriptor */},
        {/* TODO: User   Code Descriptor */},
        {/* TODO: User   Data Descriptor */},
        {
            .segment_high      = (sizeof(struct TSSEntry) & (0xF << 16)) >> 16,
            .segment_low       = sizeof(struct TSSEntry),
            .base_high         = 0,
            .base_mid          = 0,
            .base_low          = 0,
            .non_system        = 0,    // S bit
            .type_bit          = 0x9,
            .privilege         = 0,    // DPL
            .valid_bit         = 1,    // P bit
            .opr_32_bit        = 1,    // D/B bit
            .long_mode         = 0,    // L bit
            .granularity       = 0,    // G bit
        },
        {0}
    },

    .table = {
        {
            // Null Descriptor
            .segment_low = 0,
            .segment_high = 0,
            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,
            .type_bit = 0,
            .non_system = 0,
            .descriptor_privilege_level = 0,
            .present = 0,
            .long_mode = 0,
            .default_operation_size = 0,
            .granularity = 0,
        },
        {
            .segment_low = 0xFFFF,
            .segment_high = 0xF,
            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,
            .type_bit = 0xA,
            .non_system = 1,
            .descriptor_privilege_level = 0,
            .present = 1,
            .long_mode = 0,
            .default_operation_size = 1,
            .granularity = 1,

        },
        {
            .segment_low = 0xFFFF,
            .segment_high = 0xF,
            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,
            .type_bit = 0x2,
            .non_system = 1,
            .descriptor_privilege_level = 0,
            .present = 1,
            .long_mode = 0,
            .default_operation_size = 1,
            .granularity = 1,
        }}};

/**
 * _gdt_gdtr, predefined system GDTR.
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    .size = sizeof(global_descriptor_table) - 1,
    .address = &global_descriptor_table,
};
