#include "pic_firmware_update.h"
#include "opensx70.h"   // for huart1

volatile bool fw_update_in_progress = false;

void pic_firmware_enter_update_mode(void){
    if (fw_update_in_progress) {
        return;
    }
    fw_update_in_progress = true;

    // Release UART1 entirely -- no more receive interrupts armed after
    // this point. The PC now talks directly to the PIC's bootloader.
    // Recovery is a manual STM32 reset once the PIC update is done.
    HAL_UART_AbortReceive(&huart1);
}