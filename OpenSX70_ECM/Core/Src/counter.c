#include "counter.h"
#include "opensx70.h"

counter_device current_counter_state;

uint8_t counter_uart_buffer[1];
volatile bool counter_response_received = false;

void initialize_counter_device(counter_device *device)
{
    device->empty = false;
    device->selfTimer = false;
    device->tMode = false;
    device->manualMode = false;
    device->manualSpeed = 0;
    //would like to add device->count but they will be the same as manualSpeeds so will need mask the counts or shift them up to say 20-28 before sending them and then decode them on receive.
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
        case 0xFE: //signal that counter is at 0 or empty
            device->empty = true;
            modeSelection = true; //mode selection flag true turns off polling the light meter helper 
            break;

        case 0xFF: //signal that new pack is loaded
            device->empty = false;
            modeSelection = false; //mode selection flag false turns on polling the light meter helper 

            HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
            break;

        case 0xFA: //self-timer 
            device->selfTimer = true; //self timer mode
            break;

        case 0xFB: //double exposure 
            multiple_exposure_flag = true; //double exposure mode
            break;

        case 0xFC: //T-mode
            device->tMode = true; // T-mode
            break;

        default:
            if (b >= 1 && b <= 12) //shutter speed from 1 - 12 (1 = 1/2000.... 12 = 1s) have it sending the manual speed + 1 because the default state of b is 0
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
uint8_t enable = 0xAF; //takes over control of the counter display
uint8_t disable = 0XAB; //gives the control of the display back to the counter mcu

    if (0 <= tx && tx <= 9) {
    pattern = convertNumberToPattern(tx); //sending a number between 0-9 will get converted to its binary pattern to display on the 7-segments
    } else if (tx == 0xF){
    pattern = 0x00; //0xF will turn off the display

    } else{
    pattern = tx; //will display whatever binary pattern sent      
    }
    if (response){ //if checking a eeprom memory location set response flag
    enable = 3; //instead of taking control of display identify the message is a request
    pattern = memaddress; //**would not use this except for checking if the camera is empty with 0xCE as it is now when it returns its memory contents and if they are between 1-9 that will get interpreted as being manual speeds. 
    //Better to have the coutner return a specific HEX for a specific situation  mask a range of values beign returned by a shift +10, +20 etc. and deconde them once received
    //or have the open change the conditions on which it receives --- counter needs to be set up to do this first.
    // 0xCE check if empty is set up to send a response of 0xFE returned if empty, 0xFF if not empty. 
    

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