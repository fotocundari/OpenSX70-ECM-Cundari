#include "opensx70.h"

meter_iso savedISO;

volatile bool isoBlinked = false;
volatile bool modeSelection = true; //tells the poller to stop polling the light meter helper (when trying to use the LED for selecting modes etc)
bool multiple_exposure_first_run = true;
bool loopexit = false;
bool firstrun = false;
bool sendonce = true;

int mexp_count = 0;
int delay_ms = 50;

uint8_t tx = 0x0; //byte to send to counter
uint32_t global_start_time = 0;

typedef camera_state (*camera_state_funct)(void);

camera_state do_state_darkslide (void);
camera_state do_state_noDongle (void);
camera_state do_state_dongle (void);
camera_state do_state_flashBar (void);
camera_state do_state_multi_exp (void);

static const camera_state_funct STATE_MACHINE [STATE_N] = {
    &do_state_init,
    &do_state_darkslide,
    &do_state_noDongle,
    &do_state_dongle,
    &do_state_flashBar,
    &do_state_multi_exp
};

camera_state state = STATE_INIT;

void opensx70_run_state_machine (void){
    state = STATE_MACHINE[state]();
    sonar_focus();
}

camera_state do_state_init (void){
    global_start_time = uwTick;
    savedISO = read_iso();
    solenoid_init();
    initialize_peripheral_device(&current_dongle_state);
    initialize_counter_device(&current_counter_state);
    HAL_TIM_Base_Start_IT(&htim14);
    __HAL_ADC_DISABLE_IT(&hadc1, ADC_IT_AWD1);
    HAL_GPIO_WritePin(LM_RESET_GPIO_Port, LM_RESET_Pin, 1);
    if(HAL_GPIO_ReadPin(S5_GPIO_Port, S5_Pin)){
        shutter_close();
        mirror_down();
        shutter_open();
    }
    s1_iso_swap();
    integrator_init(&savedISO);
    HAL_Delay(50);
    send_counter(0, 1, 1, 0xCE); //check the counter's memory to see if its empty or at 0
    HAL_Delay(10);
    send_counter(0, 0, 0, 0); //release the counter from opensx70s control (if opens70 reset while it was under control)
    return STATE_DARKSLIDE;
}

camera_state do_state_darkslide (void){
    camera_state next_state = STATE_DARKSLIDE;

    if (HAL_GPIO_ReadPin(S8_GPIO_Port, S8_Pin) ){ //original had && !HAL_GPIO_ReadPin(S9_GPIO_Port, S9_Pin) but S9 used for communicator now
        #if SHUTTERDARKSLIDE
        if (S1_state.S1T_state){  
        #endif
            darkslide_eject();
            next_state = return_state(&current_dongle_state);
        #if SHUTTERDARKSLIDE
        }
        #endif
    }
    else{
        next_state = return_state(&current_dongle_state);
    }

    if(!isoBlinked){
        ISOBlink(&savedISO);
    } 
    init_complete = true;
    return next_state;
}

camera_state do_state_noDongle (void){
    if(S1_state.S1T_state){
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        
      if(multiple_exposure_flag){
            mexp_count++;
            if (mexp_count >= 2){
                multiple_exposure_flag = false;
                mexp_count = 0;
            }
        }
            

        if(current_counter_state.selfTimer){
            self_timer();
            current_counter_state.selfTimer = false;
        }

        begin_exposure();
        sendonce = true;
        if (current_counter_state.tMode){
            time_mode_noflash();
            current_counter_state.tMode = false;
        } else {
        
            
            if (current_counter_state.manualMode){
                manual_exposure_noflash(ShutterSpeedTiming[current_counter_state.manualSpeed]);
                
               #if !MANUAL_SPEED_LOCK 
                current_counter_state.manualMode = false;
               #endif
            }
            else{
                


                
                auto_exposure(&savedISO);
            }   
    
    }
    }

    dongleless_display(500);
    
    return return_state(&current_dongle_state);
}

camera_state do_state_flashBar (void){
    if(S1_state.S1T_state){
    if(multiple_exposure_flag){
            mexp_count++;
            if (mexp_count >= 2){
                multiple_exposure_flag = false;
                mexp_count = 0;
            }
        }
          
       
        if(current_counter_state.selfTimer){
            self_timer();
            current_counter_state.selfTimer = false;
        }

        begin_exposure();
        if (current_counter_state.tMode){
            time_mode();
            current_counter_state.tMode = false;
        } else {

            if (current_counter_state.manualMode){
                manual_exposure(&ShutterSpeedTiming[current_counter_state.manualSpeed]);
                current_counter_state.manualMode = false;
            }
            else{
                auto_exposure_flashbar(&savedISO);
            }   

        }
    }
    dongleless_display(500);
    return return_state(&current_dongle_state);
}

camera_state do_state_dongle (void){
    const uint32_t delay_ms = 500;


       
        if(get_switch_state(SELF_TIMER)){
             tx = 0b00011110;


            if (firstrun == false){
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }

            if(uwTick - global_start_time >= delay_ms){
            send_counter(0, 0, 0, 0);
            if(uwTick - global_start_time >= (2*delay_ms)){
            global_start_time = uwTick;
            firstrun = false;
            }
            }    

        } else if (get_switch_state(SELF_TIMER) == false){
         if (firstrun == true){
            send_counter(0, 0, 0, 0);
            global_start_time = uwTick;
            firstrun = false; 
            }
        }

    

    if(S1_state.S1T_state){
        if(get_switch_state(SELF_TIMER)){
            self_timer();
        }
        begin_exposure();
        dongle_functions();

    }

    return return_state(&current_dongle_state);
}

camera_state do_state_multi_exp (void){
    bool mexpSwitchStatus = get_switch_state(MEXP_MODE);
        int delay_ms = 500;

             tx = 0b01111010;

        if (multiple_exposure_first_run){
            if (firstrun == false){
           send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }
            




            if(uwTick - global_start_time >= delay_ms){

            if(get_switch_state(SELF_TIMER) & (uwTick - global_start_time <= (2*delay_ms))){
            tx = 0b00011110;
            send_counter(tx, 1, 0, 0);

            } else if (get_switch_state(SELF_TIMER) == false){

            send_counter(0, 0, 0, 0);
            if(uwTick - global_start_time >= (2*delay_ms)){
            global_start_time = uwTick;
            firstrun = false;
            }

            } else{   
            
            send_counter(0, 0, 0, 0);
            if(uwTick - global_start_time >= (3*delay_ms)){
            global_start_time = uwTick;
            firstrun = false;
            }
            }
            }
        }


    if(S1_state.S1T_state){
        if(mexpSwitchStatus){
            if(get_switch_state(SELF_TIMER)){
                self_timer();
            }
            if(multiple_exposure_first_run){
                multiple_exposure_first_run = false;
                begin_exposure();
            }
            dongle_functions();
        }
        else if(!mexpSwitchStatus && !multiple_exposure_first_run){
            multiple_exposure_flag = false;
            exposure_finish();
            return STATE_DONGLE;
        }
    }

    if(!mexpSwitchStatus && multiple_exposure_first_run){
        multiple_exposure_flag = false;
        return STATE_DONGLE;
    }

    return STATE_MULTI_EXP;
}

camera_state return_state(peripheral_device *device){
    switch(device->type){
        case PERIPHERAL_NONE:
            return STATE_NODONGLE;
        case PERIPHERAL_DONGLE:

            if(get_switch_state(MEXP_MODE)){
                multiple_exposure_flag = true;
                multiple_exposure_first_run = true;
                return STATE_MULTI_EXP;
            }
            return STATE_DONGLE;
        case PERIPHERAL_FLASHBAR:
            return STATE_FLASHBAR;
        default:
            return STATE_NODONGLE;
    }
}

void dongle_functions(void){
    #if FUZZY_MANUAL_MODE
    switch (FuzzyShutterSpeedTiming[current_dongle_state.selector].type)
    #else
    switch (ShutterSpeedTiming[current_dongle_state.selector].type)
    #endif
    {
        case MANUAL_SPEED:
            #if FUZZY_MANUAL_MODE
            fuzzy_manual_exposure(&FuzzyShutterSpeedTiming[current_dongle_state.selector], &savedISO);
            #else
            manual_exposure(&ShutterSpeedTiming[current_dongle_state.selector]);
            #endif
            break;
        case T_MODE:
            time_mode();
            break;
        case B_MODE:
            bulb_mode();
            break;
        case AUTO_MODE:
            auto_exposure(&savedISO);
            break;
        case AUTO_F_MODE:
            auto_exposure_flashbar(&savedISO);
            break;
        default:
            break;
    }
}

void self_timer(void){
   
    HAL_GPIO_WritePin(S1F_FBW_GPIO_Port, S1F_FBW_Pin, GPIO_PIN_RESET);
    send_command(PERIPHERAL_SELF_TIMER_CMD);
    send_counter(0, 0, 0, 0 );

    
    for (int i = 9; i >= 0; i--){
        send_counter(i, 1, 0, 0);
        HAL_Delay(1000);
    
        if (i == 3){
        shutter_close();
        } else if (i == 2) {
        HAL_GPIO_WritePin(S1F_FBW_GPIO_Port, S1F_FBW_Pin, GPIO_PIN_SET);

        } else if (i == 0) {
            mirror_up();
        }

}
        if(MEXP_MODE){
        tx = 0b10000000;
        send_counter(tx, 1, 0, 0);
        } else { 
        send_counter(0, 0, 0, 0);
        }
}

void ISOBlink(meter_iso *savedISO){
    switch(*savedISO){
        case ISO_640:
            for(uint8_t i=0; i<2; i++){
                send_command(BLUE_ON);
                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
                HAL_Delay(100);
                send_command(BLUE_OFF);
                HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
            }
            break;
        case ISO_125:
            for(uint8_t i=0; i<2; i++){
                send_command(RED_ON);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_Delay(100);
                send_command(RED_OFF);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
            }
            break;
    }
}

void save_iso(meter_iso *iso) {
    HAL_FLASH_Unlock();
    
    FLASH_EraseInitTypeDef eraseInit = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .Page = (FLASH_USER_DATA_ADDR - 0x08000000) / FLASH_PAGE_SIZE,
        .NbPages = 1
    };
    uint32_t pageError;
    HAL_FLASHEx_Erase(&eraseInit, &pageError);
    
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, FLASH_USER_DATA_ADDR, (uint64_t)*iso);
    
    HAL_FLASH_Lock();
    savedISO = *iso;
}

meter_iso read_iso(void) {
    uint32_t data = *(uint32_t*)FLASH_USER_DATA_ADDR;

    if (data != ISO_640 && data != ISO_125) {
        return ISO_640;
    }
    return (meter_iso)data;
}

void s1_iso_swap(void){
    meter_iso currentISO = savedISO;
    int modeflip = 0;
    uint32_t start_time = uwTick;
    const uint32_t delay_ms = 2000;
    firstrun = false;
    
  
 //Manual mode and speed selection   
    if(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET){
        isoBlinked = true;  
    
    #if DONGLELESS_MANUAL_SPEEDS_ONSHUTTERBUTTON
        int speedzoneflip = 0;
        int speedflip = 0;
        int flashspeed = 500;
        bool led1_state = false;
        bool led2_state = false;  
        modeSelection = true;
        while(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET){
                if (!current_counter_state.manualMode) current_counter_state.manualMode = true;
        if(speedzoneflip == 0){
                current_counter_state.manualSpeed = 11;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); 
                led1_state = true;
                led2_state = false;
                

        }
        else if (speedzoneflip == 1){
                current_counter_state.manualSpeed = 7;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET); 
                led1_state = false;
                led2_state = true;
     
            }
         else if (speedzoneflip == 2){
                current_counter_state.manualSpeed = 3;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
                led1_state = true;
                led2_state = true;
        }


        if(uwTick - start_time >= delay_ms){
        speedzoneflip = (speedzoneflip + 1) % 3;
        start_time = uwTick;
        

        }

        }
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        HAL_Delay(250);
        start_time = uwTick;
        
        while(HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET){

        
        if(uwTick - start_time >= delay_ms){
        speedflip = (speedflip + 1) % 4;
        start_time = uwTick;

            switch (speedflip) {
            case 0:
                flashspeed = 500;
                current_counter_state.manualSpeed = current_counter_state.manualSpeed + 3;
                break;
            case 1:
                flashspeed = 200;
                current_counter_state.manualSpeed--;
                break;
            case 2:
                flashspeed = 100;
                current_counter_state.manualSpeed--;
                break;
            case 3:
                flashspeed = 50;
                current_counter_state.manualSpeed--;
                break;
        }
        
        }
                for(uint8_t i=0; i<=speedflip; i++){   
                HAL_Delay(flashspeed);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, led1_state);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, led2_state);
                HAL_Delay(flashspeed);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                
                }

                    HAL_Delay(250);
                

        }
        
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); 
    #else
        meter_iso newISO;
        switch(currentISO){
            case ISO_640:
                newISO = ISO_125;
                break;
            case ISO_125:
                newISO = ISO_640;
                break;
            default:
                newISO = ISO_640;
                savedISO = ISO_640;
                break;
        }
        save_iso(&newISO);
        ISOBlink(&savedISO);
    
        while(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET){
        }        
    #endif    
        
    } else if (HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET){
        isoBlinked = true;  
        modeSelection = true;
        while(HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET){
        
        if(modeflip == 0){
         multiple_exposure_flag = false;
            current_counter_state.selfTimer = true;
            current_counter_state.tMode = false;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
                        
            if (firstrun == false){
            tx = 0b00011110; // "t"
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }     

        }
        else if (modeflip == 1){
            multiple_exposure_flag = true;
             current_counter_state.selfTimer = false;
             current_counter_state.tMode = false;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);

             if (firstrun == false){
            tx = 0b01111010; // "d"
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }
              
            }
         else if (modeflip == 2){
            multiple_exposure_flag = false;
            current_counter_state.selfTimer = false;
            current_counter_state.tMode = true;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
            

            if (firstrun == false){
            tx = 0b00011100; // "L"
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }
                
        }

        if(uwTick - start_time >= delay_ms){
        modeflip = (modeflip + 1) % 3;
        start_time = uwTick;
        firstrun = false;

            }

        #if DONGLELESS_MANUAL_SPEEDS_ONSHUTTERBUTTON   
        if(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET){
        
        meter_iso newISO;
        switch(currentISO){
            case ISO_640:
                newISO = ISO_125;
                break;
            case ISO_125:
                newISO = ISO_640;
                break;
            default:
                newISO = ISO_640;
                savedISO = ISO_640;
                break;
        }
        save_iso(&newISO);
        ISOBlink(&savedISO);
        multiple_exposure_flag = false;
        current_counter_state.selfTimer = false;
        current_counter_state.tMode = false;  
        loopexit = true;
        while(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET);
        while(HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET);
        }
        #endif

         
   }

    }
   

    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    // isoBlinked = true;         
    modeSelection = false;
       
}


void dongleless_display(int delay_ms){
uint8_t tx = 0;

     if (current_counter_state.selfTimer){
         tx = 0b00011110; //"t"
            if (firstrun == false){
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }

            if(uwTick - global_start_time >= delay_ms){
            send_counter(0, 0, 0, 0);
            if(uwTick - global_start_time >= (2*delay_ms)){
            global_start_time = uwTick;
            firstrun = false;
            }
            }    
     } else if (multiple_exposure_flag && mexp_count < 1){ 
            tx = 0b01111010; // "d"
            if (firstrun == false){
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }

            if(uwTick - global_start_time >= delay_ms){
            send_counter(0, 0, 0, 0);
            if(uwTick - global_start_time >= (2*delay_ms)){
            global_start_time = uwTick;
            firstrun = false;
            }
            }    
    } else if (current_counter_state.tMode){
            tx = 0b00011100; // "L"
                    if (firstrun == false){
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }

            if(uwTick - global_start_time >= delay_ms){
            send_counter(0, 0, 0, 0);
            if(uwTick - global_start_time >= (2*delay_ms)){
            global_start_time = uwTick;
            firstrun = false;
            }
            }    
    } else if (current_counter_state.manualMode){
            tx = 0b00101010; // "n"
                    if (firstrun == false){
            send_counter(tx, 1, 0, 0);
            global_start_time = uwTick;
            firstrun = true;
            }

            if(uwTick - global_start_time >= delay_ms){
            send_counter(0, 0, 0, 0);
            if(uwTick - global_start_time >= (2*delay_ms)){
            global_start_time = uwTick;
            firstrun = false;
            }
            }    
    } else{
        if (firstrun == true){
            send_counter(0, 0, 0, 0);
            global_start_time = uwTick;
            firstrun = false; 
            }        
    }

}