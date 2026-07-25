#ifndef COUNTER_H
#define COUNTER_H

#include "settings.h"
#include "opensx70.h"

typedef struct counter_device {
    bool empty;
    bool selfTimer;
    bool tMode;
    bool manualMode;
    uint8_t manualSpeed;
    } counter_device;

void initialize_counter_device(counter_device *device);
void update_counter(counter_device *device);
void send_counter(uint8_t tx, bool display_control_enable, bool response, uint8_t memaddress);
uint8_t convertNumberToPattern(uint8_t number);
void counter_uart_rx_complete(void);

extern counter_device current_counter_state;
extern volatile bool counter_response_received;
extern uint8_t counter_uart_buffer[1];

#endif