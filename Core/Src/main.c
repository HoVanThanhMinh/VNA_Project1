/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"
//IC CD4094
#define STR_PIN GPIO_PIN_8
#define DATA_PIN GPIO_PIN_9
#define CLOCK_PIN GPIO_PIN_8
// DS1307
#define DS1307_I2C_ADDR     0x68
#define DS1307_REG_SECOND     0x00
#define DS1307_REG_MINUTE     0x01
#define DS1307_REG_HOUR      0x02
#define DS1307_REG_DOW        0x03
#define DS1307_REG_DATE       0x04
#define DS1307_REG_MONTH      0x05
#define DS1307_REG_YEAR       0x06
#define DS1307_REG_CONTROL     0x07
#define DS1307_REG_UTC_HR    0x08
#define DS1307_REG_UTC_MIN    0x09
#define DS1307_REG_CENT        0x10
#define DS1307_TIMEOUT        1000
//PT2272
#define D0_PIN GPIO_PIN_5
#define D1_PIN GPIO_PIN_4
#define D2_PIN GPIO_PIN_3
#define D3_PIN GPIO_PIN_0
// REMOTE
#define button1 0x0001
#define button2 0x0010
#define button3 0x0011
#define button4 0x0100
#define button5 0x0101
#define button6 0x0110
#define button7 0x0111
#define button8 0x1000
#define button9 0x1001
#define button10 0x1010
#define button11 0x1011
#define button12 0x1100
// LED7
#define NUM_DIGITS 4
//
uint8_t seven_segmenta[10]={0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82,0xf8, 0x80, 0x90};
typedef struct {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t dow;
    uint8_t date;
    uint8_t month;
    uint16_t year;
} DS1307_STRUCT;
DS1307_STRUCT ds1307;
uint8_t DS1307_DecodeBCD(uint8_t bin);
uint8_t DS1307_EncodeBCD(uint8_t dec);
void DS1307_SetClockHalt(uint8_t halt);
void DS1307_SetRegByte(uint8_t regAddr, uint8_t val);
void DS1307_SetTimeZone(int8_t hr, uint8_t min);
uint8_t DS1307_GetClockHalt(void);
uint8_t DS1307_GetRegByte(uint8_t regAddr);
void DS1307_config();
void DS1307_gettime();
void DS1307_settime(uint8_t sec,uint8_t min,uint8_t hour_24mode,uint8_t dayOfWeek,uint8_t date,uint8_t month, uint16_t year);
void SystemClock_Config(void);
void send_data_to_cd4094(uint8_t data);
void send_out_led(uint8_t data1, uint8_t data2, uint8_t data3, uint8_t data4);
void display_time_on_7seg(uint8_t hour, uint8_t min, uint8_t sec);
void display_date_on_7seg(uint8_t date, uint8_t month, uint16_t year);
void reset_led(void);
uint8_t read_data_from_remote(void);
void handle_control_button();

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  reset_led();
  HAL_TIM_Base_Start_IT(&htim2);
  DS1307_config();
  DS1307_settime(59, 8, 04, 2, 1, 5, 2024);
  while (1)
  {
       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET); 
       HAL_Delay(50);
       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET); 
       HAL_Delay(50);
       DS1307_gettime();
       display_time_on_7seg(ds1307.hour, ds1307.min, ds1307.sec);
       //display_date_on_7seg(ds1307.date, ds1307.month, ds1307.year);
       handle_control_button();
       HAL_Delay(1);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}
void DS1307_gettime(){
    uint16_t cen;
    ds1307.sec=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_SECOND) & 0x7f);
    ds1307.min=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_MINUTE));
    ds1307.hour=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_HOUR) & 0x3f);
    ds1307.dow=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_DOW));
    ds1307.date=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_DATE));
    ds1307.month=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_MONTH));
    cen = DS1307_GetRegByte(DS1307_REG_CENT) * 100;
    ds1307.year=DS1307_DecodeBCD(DS1307_GetRegByte(DS1307_REG_YEAR)) + cen;
}
void DS1307_SetRegByte(uint8_t regAddr, uint8_t val) {
    uint8_t bytes[2] = { regAddr, val };
    if(HAL_I2C_Master_Transmit(&hi2c1, DS1307_I2C_ADDR << 1, bytes, 2, DS1307_TIMEOUT) != HAL_OK)
    {
      Error_Handler();
    }
}
uint8_t DS1307_GetClockHalt(void) {
    return (DS1307_GetRegByte(DS1307_REG_SECOND) & 0x80) >> 7;
}
void DS1307_settime(uint8_t sec,uint8_t min,uint8_t hour_24mode,uint8_t dayOfWeek,uint8_t date,uint8_t month, uint16_t year){
    DS1307_SetRegByte(DS1307_REG_SECOND, DS1307_EncodeBCD(sec | DS1307_GetClockHalt()));
    DS1307_SetRegByte(DS1307_REG_MINUTE, DS1307_EncodeBCD(min));
    DS1307_SetRegByte(DS1307_REG_HOUR, DS1307_EncodeBCD(hour_24mode & 0x3f));//hour_24mode Hour in 24h format, 0 to 23.
    DS1307_SetRegByte(DS1307_REG_DOW, DS1307_EncodeBCD(dayOfWeek));//dayOfWeek Days since last Sunday, 0 to 6.
    DS1307_SetRegByte(DS1307_REG_DATE, DS1307_EncodeBCD(date));//date Day of month, 1 to 31.
    DS1307_SetRegByte(DS1307_REG_MONTH, DS1307_EncodeBCD(month));//month Month, 1 to 12.
    DS1307_SetRegByte(DS1307_REG_CENT, year / 100);
    DS1307_SetRegByte(DS1307_REG_YEAR, DS1307_EncodeBCD(year % 100));//2000 to 2099.
}
uint8_t DS1307_GetRegByte(uint8_t regAddr) {
    uint8_t val;
    HAL_I2C_Master_Transmit(&hi2c1, DS1307_I2C_ADDR << 1, &regAddr, 1, DS1307_TIMEOUT);
    HAL_I2C_Master_Receive(&hi2c1, DS1307_I2C_ADDR << 1, &val, 1, DS1307_TIMEOUT);
    return val;
}
void DS1307_SetClockHalt(uint8_t halt) {
    uint8_t ch = (halt ? 1 << 7 : 0);
    DS1307_SetRegByte(DS1307_REG_SECOND, ch | (DS1307_GetRegByte(DS1307_REG_SECOND) & 0x7f));
}
void DS1307_SetTimeZone(int8_t hr, uint8_t min) {
    DS1307_SetRegByte(DS1307_REG_UTC_HR, hr);
    DS1307_SetRegByte(DS1307_REG_UTC_MIN, min);
}
void DS1307_config(){
    DS1307_SetClockHalt(0);
    DS1307_SetTimeZone(+8, 00);
}
uint8_t DS1307_DecodeBCD(uint8_t bin) {
    return (((bin & 0xf0) >> 4) * 10) + (bin & 0x0f);
}
uint8_t DS1307_EncodeBCD(uint8_t dec) {
    return (dec % 10 + ((dec / 10) << 4));
}
void send_data_to_cd4094(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(GPIOC, DATA_PIN, (data & 0x80) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, CLOCK_PIN, GPIO_PIN_SET); HAL_Delay(1); 
        HAL_GPIO_WritePin(GPIOC, CLOCK_PIN, GPIO_PIN_RESET); HAL_Delay(1);
        data = data << 1;
    }
}
void send_out_led(uint8_t data1, uint8_t data2, uint8_t data3, uint8_t  data4) {
    send_data_to_cd4094(data1);
    send_data_to_cd4094(data2);
    send_data_to_cd4094(data3);
    send_data_to_cd4094(data4);
    HAL_GPIO_WritePin(GPIOA, STR_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOA, STR_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
}
void reset_led(void)
{
    for(int i=0;i<10;i++)
    {
      send_out_led(seven_segmenta[i], seven_segmenta[i], seven_segmenta[i], seven_segmenta[i]);
      HAL_Delay(1);
    }
}
void display_time_on_7seg(uint8_t hour, uint8_t min, uint8_t sec)
{   
    uint8_t led1, led2, led3, led4;
    led1= seven_segmenta[hour/10];
    HAL_Delay(1);
    led2 = seven_segmenta[hour%10];
    HAL_Delay(1);
    led3 = seven_segmenta[min/10];
    HAL_Delay(1);
    led4 = seven_segmenta[min%10];
    HAL_Delay(1);
    send_out_led(led1, led2, led3, led4);
}
void display_date_on_7seg(uint8_t date, uint8_t month, uint16_t year) {
    uint8_t led5, led6, led7, led8;
    led5 = seven_segmenta[(date / 10) % 10]; // Tens of date
    led6 = seven_segmenta[date % 10];       // Units of date
    led7 = seven_segmenta[(month / 10) % 10]; // Tens of month
    led8 = seven_segmenta[month % 10];       // Units of month
    
    // Send data to CD4094 to display
    send_out_led(led5, led6, led7, led8);
}
// 0001->0010->0011->0100->...1100.
uint8_t read_data_from_remote(void) {
    uint8_t data = 0;
    if (HAL_GPIO_ReadPin(GPIOB, D0_PIN) == GPIO_PIN_SET) {
        data |= (1 << 0);
    }
    if (HAL_GPIO_ReadPin(GPIOB, D1_PIN) == GPIO_PIN_SET) {
        data |= (1 << 1);
    }
    if (HAL_GPIO_ReadPin(GPIOB, D2_PIN) == GPIO_PIN_SET) {
        data |= (1 << 2);
    }
    if (HAL_GPIO_ReadPin(GPIOA, D3_PIN) == GPIO_PIN_SET) {
        data |= (1 << 3);
    }
    return data;
}
void handle_control_button(void)
{   
    unsigned int data[NUM_DIGITS] = {0};
    static int current_index = 2;
    static int digit_index = 0;
    uint8_t flag = 0; 
    uint8_t buttonPressed = read_data_from_remote();
    int hc = (ds1307.hour/10)%10;
    int hv = (ds1307.hour)%10;
    int mc = (ds1307.min/10)%10;
    int mv = (ds1307.min)%10;
//    int sc = (ds1307.sec/10)%10;
//    int sv = (ds1307.sec)%10;
if(buttonPressed !=0)
  {
 // Button 10 : Manual settings
  if(buttonPressed == 1010)
   {
     flag =1;
   } 
else if(flag)
   {
   if(buttonPressed == 0001)
   {
    digit_index++;
    if(current_index ==0){
     if(digit_index >2)
       digit_index = 0;
    }
    if(current_index !=0 && current_index %2 !=0){
      if(digit_index >9)
        digit_index =0;
    }
    if(current_index %2 ==0){
      if(digit_index >6)
        digit_index = 0;
    }
    if(current_index >=0 && current_index <= 3)
    {
      for(int i=0; i<4; i++)
      {
        if(i==current_index)
        {
               data[i]=seven_segmenta[1];
        }
        else {
               data[i]=seven_segmenta[0];
        }
      }
    }
    data[current_index] = seven_segmenta[digit_index];
    switch(current_index)
    {
    case 0:
      ds1307.hour = digit_index * 10 + hv;
      break;
    case 1:
      ds1307.hour = hc*10 + digit_index;
      break;
    case 2:
      ds1307.min = digit_index*10 + mv;
      break;
    case 3:
      ds1307.min = mc*10 + digit_index;
      break;
    }
    DS1307_settime(ds1307.sec, ds1307.min, ds1307.hour, ds1307.dow, ds1307.date, ds1307.month, ds1307.year);
    data[current_index] = seven_segmenta[NUM_DIGITS];
    }
   }
 // Button 2: reduce value 1
 if(buttonPressed ==0010)
 {
   digit_index--;
   if(digit_index <0)
     digit_index = 9;
     data[current_index] = seven_segmenta[digit_index];
     HAL_Delay(1);
 }
 // Button 3: move currsor right
 if(buttonPressed ==0011)
 {
//   digit_index=0;
   current_index++;
   if (current_index > 3)
   current_index = 0;
 }
 //Button 4: move curror left
 if(buttonPressed ==0100)
 {
//  digit_index =0;
  current_index--;
  if(current_index <0)
    current_index = 3;
 }
 //Button 5: increase value 1
 if(buttonPressed ==0101)
 {
  digit_index++;
  if(digit_index > 9)
    digit_index =0;
    data[current_index] = seven_segmenta[digit_index];
    HAL_Delay(1);
 }
 //Button 11:  Reset Time
 if(buttonPressed ==1011)
 {
   DS1307_settime(59,59,23,2,1,5,2024);
   HAL_Delay(1);
 }
 // Button 12: Exit Manual Setting
 if(buttonPressed ==1100)
 {
   flag = 0;
 }
 }     
}   
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef  USE_FULL_ASSERT

  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
