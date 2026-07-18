void s1_iso_swap(void){
    meter_iso currentISO = savedISO;
    int modeflip = 0;
    int speedzoneflip = 0;
    int speedflip = 0;
    int flashspeed = 500;
    bool led1_state = false;
    bool led2_state = false;    
     uint32_t start_time = uwTick;
    const uint32_t delay_ms = 2000;
    
  
    
    if(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET){

    
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
        HAL_Delay(500);
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
        isoBlinked = true; 
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); 

        
        
    } else if (HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET){

        while(HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET){
        
        if(modeflip == 0){
             multiple_exposure_flag = true;
             selfy = false;
             tmode = false;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
        }
        else if (modeflip == 1){
            multiple_exposure_flag = false;
            selfy = true;
            tmode = false;
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
                HAL_Delay(100);
                
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
                
        }

        if(uwTick - start_time >= delay_ms){
        modeflip = (modeflip + 1) % 3;
        start_time = uwTick;

            }

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
        save_iso(newISO);
        ISOBlink(&savedISO);
        isoBlinked = true;
        multiple_exposure_flag = false;
        selfy = false;
        tmode = false;  
        loopexit = true;
        while(HAL_GPIO_ReadPin(S1T_GPIO_Port, S1T_Pin) == GPIO_PIN_SET);
        while(HAL_GPIO_ReadPin(S1F_GPIO_Port, S1F_Pin) == GPIO_PIN_SET);
        }
         
   }

    }
   

    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
    isoBlinked = true;         
  
       
}

