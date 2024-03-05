#include "header/cpu/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // TODO : Implement
            // Null Descriptor
        },
        {
            // TODO : Implement
            .segment_low = 0xFFFFFF,
            .segment_high = 0xFFFFFF,
            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,
            .type_bit = 0xA / 0b1010,
            .non_system = 1,
            .descriptor_privilege_level = 0,
            .present = 1,
            .long_mode = 0,
            .default_operation_size = 1,
            .granularity = 1,

        },
        {
            // TODO : Implement
            .segment_low = 0xFFFFFF,
            .segment_high = 0xFFFFFF,
            .base_low = 0,
            .base_mid = 0,
            .base_high = 0,
            .type_bit = 0x2 / 0b0010,
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
    // TODO : Implement, this GDTR will point to global_descriptor_table.
    //        Use sizeof operator

};
