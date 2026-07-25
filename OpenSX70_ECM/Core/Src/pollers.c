#include "pollers.h"
#include "peripheralport.h"
#include "counter.h"

volatile int last_time = 0; //stores the last time of the tick counter to check elapsed time without using delays.

typedef poller_state (*poller_state_funct)(void);

poller_state do_state_poll_wait(void);
poller_state do_state_poll_dongle(void);
poller_state do_state_poll_meter(void);

static const poller_state_funct POLLER_MACHINE [STATE_POLL_N] = {
    &do_state_poll_wait,
    &do_state_poll_dongle,
    &do_state_poll_counter,
    &do_state_poll_meter

};

volatile bool init_complete = false;
poller_state poller = STATE_POLL_WAIT;

void poll(){
    update_button_state(&S1_state);
    poller = POLLER_MACHINE[poller]();
}

poller_state do_state_poll_wait(){
    if(init_complete){
        return STATE_POLL_DONGLE;
    }
    return STATE_POLL_WAIT;
}

poller_state do_state_poll_dongle(){
    update_peripheral_status(&current_dongle_state);
    return STATE_POLL_COUNTER;
}

poller_state do_state_poll_meter(){
    if(!LIGHMETER_HELPER || modeSelection){
        return STATE_POLL_DONGLE;
    }
    switch(current_dongle_state.type){
        case PERIPHERAL_NONE:
            approximate_exposure_time(LOW_LIGHT);
            break;
        case PERIPHERAL_DONGLE:
            approximate_exposure_time(OFF);
            break;
        case PERIPHERAL_FLASHBAR:
            approximate_exposure_time(OFF);
            break;
        default:
            break;
    }
    return STATE_POLL_DONGLE;
}

poller_state do_state_poll_counter(void)
{
    update_counter(&current_counter_state);
    int delay_ms = 50;
    //flashes LED on empty without causing delay.
        if (current_counter_state.empty){
        if ((HAL_GetTick() - last_time) >= delay_ms){
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        }

        if ((HAL_GetTick() - last_time) >= delay_ms*2){
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        last_time = HAL_GetTick();
        }       
    }

    return STATE_POLL_METER;
}