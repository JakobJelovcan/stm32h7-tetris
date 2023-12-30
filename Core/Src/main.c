/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* Definitions for lcdTask */
osThreadId_t lcdTaskHandle;
const osThreadAttr_t lcdTask_attributes = { .name = "lcdTask", .stack_size = 128
        * 4, .priority = (osPriority_t)osPriorityNormal, };
/* Definitions for inputTask */
osThreadId_t inputTaskHandle;
const osThreadAttr_t inputTask_attributes = { .name = "inputTask", .stack_size =
        128 * 4, .priority = (osPriority_t)osPriorityLow, };
/* USER CODE BEGIN PV */
osMessageQueueId_t actionQueue;
TIM_HandleTypeDef tim2;
RNG_HandleTypeDef rng;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void StartLcdTask(void* argument);
void StartInputTask(void* argument);

/* USER CODE BEGIN PFP */
static void LCD_Config(void);
static void TS_Config(void);
static void TIM_Config(void);
static void BTN_Config(void);
static void RNG_Config(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    TS_Config();
    RNG_Config();
    LCD_Config();
    TIM_Config();
    BTN_Config();
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    /* USER CODE BEGIN 2 */

    /* USER CODE END 2 */

    /* Init scheduler */
    osKernelInitialize();

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */
    actionQueue = osMessageQueueNew(32, sizeof(action_t), NULL);
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* creation of lcdTask */
    lcdTaskHandle = osThreadNew(StartLcdTask, NULL, &lcdTask_attributes);

    /* creation of inputTask */
    inputTaskHandle = osThreadNew(StartInputTask, NULL, &inputTask_attributes);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    HAL_TIM_Base_Start_IT(&tim2);
    reset_game();
    /* USER CODE END RTOS_EVENTS */

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    HAL_StatusTypeDef ret = HAL_OK;

    /*!< Supply configuration update enable */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48 | RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 160;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
        | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
        | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }

    /*
     Note : The activation of the I/O Compensation Cell is recommended with communication  interfaces
     (GPIO, SPI, FMC, QSPI ...)  when  operating at  high frequencies(please refer to product datasheet)
     The I/O Compensation Cell activation  procedure requires :
     - The activation of the CSI clock
     - The activation of the SYSCFG clock
     - Enabling the I/O Compensation Cell : setting bit[0] of register SYSCFG_CCCSR
     */

     /*activate CSI clock mandatory for I/O Compensation Cell*/
    __HAL_RCC_CSI_ENABLE();

    /* Enable SYSCFG clock mandatory for I/O Compensation Cell */
    __HAL_RCC_SYSCFG_CLK_ENABLE()
        ;

    /* Enables the I/O Compensation Cell */
    HAL_EnableCompensationCell();
}

/* USER CODE BEGIN 4 */
static void LCD_Config(void) {
    BSP_LCD_Init(0, LCD_ORIENTATION_LANDSCAPE);
    BSP_LCD_SetLayerAddress(0, 0, LCD_LAYER_0_ADDRESS);
    UTIL_LCD_SetFuncDriver(&LCD_Driver);
    UTIL_LCD_SetFont(&Font12);
    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_BLACK);
    UTIL_LCD_SetTextColor(UTIL_LCD_COLOR_WHITE);
    UTIL_LCD_Clear(UTIL_LCD_COLOR_BLACK);
    HAL_NVIC_SetPriority(LTDC_IRQn, 10, 10);
    HAL_NVIC_EnableIRQ(LTDC_IRQn);
}

static void TS_Config(void) {
    TS_Init_t init = { TS_MAX_WIDTH, TS_MAX_HEIGHT, TS_SWAP_XY, 5 };
    BSP_TS_Init(0, &init);
}

static void TIM_Config(void) {
    //Timer base frequency is 200MHz
    //An interrupt is triggered every 10ms
    __HAL_RCC_TIM2_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM2_IRQn, 10, 10);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    tim2.Instance = TIM2;
    tim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    tim2.Init.Period = 10000 - 1;
    tim2.Init.Prescaler = 200 - 1;
    HAL_TIM_Base_Init(&tim2);
    __HAL_TIM_CLEAR_FLAG(&tim2, TIM_FLAG_UPDATE);
}

static void BTN_Config(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 10, 10);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    GPIO_InitTypeDef init = { 0 };
    init.Pin = GPIO_PIN_13;
    init.Mode = GPIO_MODE_IT_RISING;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    init.Pull = GPIO_PULLDOWN;

    HAL_GPIO_Init(GPIOC, &init);
}

static void RNG_Config(void) {
    __HAL_RCC_RNG_CLK_ENABLE();
    rng.Instance = RNG;
    rng.Init.ClockErrorDetection = RNG_CED_ENABLE;
    if (HAL_RNG_Init(&rng) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef* hltdc) {
    static uint16_t buffer_index = 0;

    buffer_index = 1 - buffer_index;
    uint32_t buffer_addr = (buffer_index) ? LCD_LAYER_1_ADDRESS : LCD_LAYER_0_ADDRESS;
    BSP_LCD_Reload(0, BSP_LCD_RELOAD_NONE); //Disable reloading
    BSP_LCD_SetLayerAddress(0, 0, buffer_addr); //Update the buffer
    UTIL_LCD_Clear(UTIL_LCD_COLOR_BLACK); //Clear the new buffer
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartLcdTask */
/**
 * @brief  Function implementing the lcdTask thread.
 * @param  argument: Not used
 * @retval None
 */
 /* USER CODE END Header_StartLcdTask */
void StartLcdTask(void* argument) {
    /* USER CODE BEGIN 5 */
    /* Infinite loop */
    action_t action;
    for (;;) {
        while (osMessageQueueGet(actionQueue, &action, 0U, 0U) == osOK) {
            perform_action(action);
        }
        clear_lines();
        render();
        BSP_LCD_Reload(0, BSP_LCD_RELOAD_VERTICAL_BLANKING);
        osDelay(20);
    }
    /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartInputTask */
/**
 * @brief Function implementing the inputTask thread.
 * @param argument: Not used
 * @retval None
 */
 /* USER CODE END Header_StartInputTask */
void StartInputTask(void* argument) {
    TS_State_t touch_state;
    /* USER CODE BEGIN StartInputTask */
    /* Infinite loop */
    for (;;) {
        BSP_TS_GetState(0, &touch_state);
        //Shift button state
        for (size_t i = 0; i < N_BTN; ++i) {
            buttons[i].state <<= 1;
        }
        //Get new button states
        if (touch_state.TouchDetected) {
            for (size_t i = 0; i < N_BTN; ++i) {
                buttons[i].state |= (buttons[i].x <= touch_state.TouchX &&
                    buttons[i].x + X_BTN >= touch_state.TouchX &&
                    buttons[i].y <= touch_state.TouchY &&
                    buttons[i].y + Y_BTN >= touch_state.TouchY);
            }
        }
        //Update actions
        for (size_t i = 0; i < N_BTN; ++i) {
            if ((buttons[i].state & 0xf) == 0x3) {
                buttons[i].polygon.selected = 1 - buttons[i].polygon.selected;
                osMessageQueuePut(actionQueue, &buttons[i].action, 2U, 0U);
            }
        }
        osDelay(20);
    }
    /* USER CODE END StartInputTask */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
       /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
