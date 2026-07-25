#include "counter.h"
#include "opensx70.h"

counter_device current_counter_state;

uint8_t counter_uart_buffer[1];
volatile bool counter_response_received = false;

void initialize_counter_device(counter_device *device)
{
    device->empty = false;
    device->modeSelection = false;
    device->selfTimer = false;
    device->tMode = false;
    device->manualMode = false;
    device->manualSpeed = 0;
}

void update_counter(counter_device *device)
{
    if (!counter_response_received)
    {
        return;
    }

    counter_response_received = false;

    uint8_t b = counter_uart_buffer[0];

    HAL_UART_Receive_IT(&huart1, counter_uart_buffer, 1);

    switch (b)
    {
        case 0xFE:
            device->empty = true;
            device->modeSelection = true;
            break;

        case 0xFF:
            device->empty = false;
            device->modeSelection = false;

            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
            break;

        case 0xFA:
            device->selfTimer = true;
            break;

        case 0xFB:
            multiple_exposure_flag = true;
            break;

        case 0xFC:
            device->tMode = true;
            break;

        default:
            if (b >= 1 && b <= 12)
            {
                device->manualSpeed = b - 1;
                device->manualMode = true;
                isoBlinked = true;
            }
            break;
    }
}


void send_counter(uint8_t tx, bool display_control_enable, bool response, uint8_t memaddress){ // if display_control_enable is true the OPEN ECM will take control of the display, if false the counter will continue to control where left off.
HAL_HalfDuplex_EnableTransmitter(&huart1);
uint8_t pattern = 0;
uint8_t enable = 0xAF;
uint8_t disable = 0XAB; 

    if (0 <= tx && tx <= 9) {
    pattern = convertNumberToPattern(tx);  
    } else if (tx == 0xF){
    pattern = 0x00;

    } else if (tx == 0xDD){

    pattern = 2;
    } else{
    pattern = tx;     
    }

    if (response){
    enable = 3;
    pattern = memaddress; 
    }

    HAL_Delay(10);
    if (!display_control_enable){
    HAL_UART_Transmit(&huart1, &disable, 1, HAL_MAX_DELAY); 
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
    } else {
    HAL_UART_Transmit(&huart1, &enable, 1, HAL_MAX_DELAY);
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
    HAL_Delay(10);
    HAL_UART_Transmit(&huart1, &pattern, 1, HAL_MAX_DELAY);
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
    }

    HAL_UART_Receive_IT(&huart1, counter_uart_buffer, 1);
    HAL_HalfDuplex_EnableReceiver(&huart1);

}

uint8_t convertNumberToPattern (uint8_t number) {
    
    switch (number) {
  
        // bit sequence of numerals to send to display shift
        case 0: return 0b11111100;
        case 1: return 0b01100000;
        case 2: return 0b11011010;
        case 3: return 0b11110010;
        case 4: return 0b01100110;
        case 5: return 0b10110110;
        case 6: return 0b10111110;
        case 7: return 0b11100000;
        case 8: return 0b11111110;
        case 9: return 0b11100110;
        default: return 0b00000000;
    }
    
}