#ifndef PIC_FIRMWARE_UPDATE_H
#define PIC_FIRMWARE_UPDATE_H

#include <stdint.h>
#include <stdbool.h>

// Byte the PC sends to tell the STM32 to go passive on UART1 so the
// PC can talk directly to the PIC's bootloader (see pic_bootloader.h
// on the PIC side for the FW_CMD_* protocol that takes over from here).
// Distinct from both counter.c's range (0xFA-0xFF, 0-12, 0xAF/0xAB/3)
// and the PIC bootloader's range (0xB0-0xB9, 0x06, 0x15, 0x99, 0x6B, 0xAA, 0xCC).
#define PC_UPDATE_TRIGGER_BYTE 0xF8

extern volatile bool fw_update_in_progress;

void pic_firmware_enter_update_mode(void);

#endif