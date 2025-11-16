#ifndef A2560_STRUCT_H
#define A2560_STRUCT_H

/* System info from GAVIN and VICKY */
struct foenix_system_info_t
{
    char     *model_name;
    uint8_t  cpu_id;
    uint32_t cpu_speed_hz;
    uint32_t vram_size;
    char     *cpu_name;
    char     pcb_revision_name[4];
    uint32_t fpga_date; /* ddddmmyy, BCD */
    uint16_t fpga_major;
    uint16_t fpga_minor;
    uint32_t fpga_partnumber;
};

/* Keyboard / mouse */
typedef void (*scancode_handler_t)(uint8_t);
typedef void (*mouse_packet_handler_t)(int8_t*);

/* Handler for the Real Time Clock ticking, must save registers and return with RTE */
typedef void (*tick_handler_t)(void);

#endif