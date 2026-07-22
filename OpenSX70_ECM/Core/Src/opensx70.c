#include "opensx70.h"

meter_iso savedISO;

volatile bool isoBlinked = false;
bool multiple_exposure_first_run = true;
bool selfy = false;
bool tmode = false;
bool manualmode = false;
bool manualmenu = false;
bool loopexit = false;
bool firstrun = false;
bool sendonce = true;
bool empty = false;


int manualspeed = 0; 
int speedselect = 0;
int mexp_count = 0;

uint8_t off = 0xFF;
uint8_t on = 0X01; 
uint8_t tx = 0x0;
uint32_t global_start_time = 0;
uint8_t b = 0;

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
    
    if (counter_response_received) {
    counter_response_received = false;
    b = counter_uart_buffer[0];
    HAL_UART_Receive_IT(&huart1, counter_uart_buffer, 1);
    if (b == 0xFE){
        empty = true;
        modeselection = true;
        } else if (b == 0xFF) {
        empty = false;
        modeselection = false;
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        } else if (b == 0XFA) {
        selfy = true;
        } else if (b == 0xFB) {
        multiple_exposure_flag = true;
        } else if (b == 0xFC) {
        tmode = true;
        } else if (b >=1 && b <= 12) {
            manualspeed = b - 1;
            manualmode = true;
            manualmenu = true;
            isoBlinked = true;
        } else if (b == 0xEE){
            manualmenu = false;
            isoBlinked = false;
        }

    }

     if (manualmenu) convert_speed_display(manualspeed);

    if (empty & !manualmenu){
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        HAL_Delay(50);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        HAL_Delay(50);
        
    }

   

}


camera_state do_state_init (void){
    global_start_time = uwTick;
    savedISO = read_iso();
    solenoid_init();
    initialize_peripheral_device(&current_dongle_state);
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
    initialize_peripheral_device(&current_dongle_state);
    HAL_Delay(50);
    send_counter(0, 1, 1, 0xCE);
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
            

        if(selfy){
            self_timer();
            selfy = false;
        }

        begin_exposure();
        sendonce = true;
        if (tmode){
            time_mode_noflash();
            tmode = false;
        } else {
        
            
            if (manualmode){
                manual_exposure_noflash(ShutterSpeedTiming[manualspeed]);
                
               #if !MANUAL_SPEED_LOCK 
                manualmode = false;
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
          
       
        if(selfy){
            self_timer();
            selfy = false;
        }

        begin_exposure();
        if (tmode){
            time_mode();
            tmode = false;
        } else {

            if (manualmode){
                manual_exposure(&ShutterSpeedTiming[manualspeed]);
                manualmode = false;
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
    
    #if DONGLELESS_MANUAL_SPEEDS
        int speedzoneflip = 0;
        int speedflip = 0;
        int flashspeed = 500;
        bool led1_state = false;
        bool led2_state = false;  
        modeselection = true;
        while(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET){
                if (!manualmode) manualmode = true;
        if(speedzoneflip == 0){
                manualspeed = 11;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); 
                led1_state = true;
                led2_state = false;
                

        }
        else if (speedzoneflip == 1){
                manualspeed = 7;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET); 
                led1_state = false;
                led2_state = true;
     
            }
         else if (speedzoneflip == 2){
                manualspeed = 3;
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
                manualspeed = manualspeed + 3;
                break;
            case 1:
                flashspeed = 200;
                manualspeed--;
                break;
            case 2:
                flashspeed = 100;
                manualspeed--;
                break;
            case 3:
                flashspeed = 50;
                manualspeed--;
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
        modeselection = true;
        while(HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET){
        
        if(modeflip == 0){
         multiple_exposure_flag = false;
            selfy = true;
            tmode = false;
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
             selfy = false;
             tmode = false;
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
            selfy = false;
            tmode = true;
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

        #if DONGLELESS_MANUAL_SPEEDS    
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
        selfy = false;
        tmode = false;  
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
    modeselection = false;
       
}


void dongleless_display(int delay_ms){
uint8_t tx = 0;

     if (selfy){
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
    } else if (tmode){
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
    } else if (manualmode & !manualmenu){
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

void convert_speed_display(int speed){
uint8_t tx = 0;
uint8_t tx2 = 0;
uint8_t tx3 = 9;
int delay_ms = 175;

    switch (speed){
        case 0:
        tx = 2;
        tx2 = 0b00101110;
            break;
        case 1:
        tx = 1;
        tx2 = 0b00101110;
            break;
        case 2:
        tx = 5;
        tx2 = 0;
        tx3 = 0;
            break;
        case 3:
        tx = 2;
        tx2 = 5;
        tx3 = 0;
            break;
        case 4:
        tx = 1;
        tx2 = 2;
        tx3 = 5;
            break;
        case 5:
        tx = 6;
        tx2 = 0;
            break;
        case 6:
        tx = 3;
        tx2 = 0;
            break;
        case 7:
        tx = 1;
        tx2 = 5;
            break;
        case 8:
        tx = 8;
            break;
        case 9:
        tx = 4;
            break;
        case 10:
        tx = 2;
            break;
        case 11:
        tx = 1;
            break;
 
    }
  
        send_counter(tx, 1, 0, 0);
        HAL_Delay(delay_ms);

        if (speed > 10){
        HAL_Delay(delay_ms*2);  
        send_counter(0xDD, 1, 0, 0);
        HAL_Delay(delay_ms*3);     
        }

        if (speed < 11 && speed > 7){  
        HAL_Delay(delay_ms); 
            send_counter(0xF, 1, 0 , 0);
        HAL_Delay(delay_ms);     
        }
        
        if (speed < 8 || speed < 2){
                if (tx2 == tx3) {
                send_counter(0xF, 1, 0 , 0);
                HAL_Delay(delay_ms/2);    
                }
        send_counter(tx2, 1, 0, 0);
        HAL_Delay(delay_ms);
        

                if (speed < 5 && speed > 1){  
                if (tx2 == tx3) {
                send_counter(0xF, 1, 0 , 0);
                HAL_Delay(delay_ms/2);    
                }
                send_counter(tx3, 1, 0, 0);
                HAL_Delay(delay_ms);
                }

    
        send_counter(0xF, 1, 0 , 0);
        HAL_Delay(delay_ms*3);
             }
        
        send_counter(0, 0, 0, 0);       


} 