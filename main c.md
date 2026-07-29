# Wiring the driver into `main.c`

Paste each block into the **matching** `USER CODE` section of the CubeMX-generated
`main.c`. Never paste outside the `BEGIN`/`END` markers, or the next code
generation will erase it.

## Common to both modes

```c
/* USER CODE BEGIN Includes */
#include "button.h"
/* USER CODE END Includes */
```

```c
/* USER CODE BEGIN 2 */
Button_Init();
/* USER CODE END 2 */
```

## Polling mode

```c
/* USER CODE BEGIN WHILE */
while (1)
{
    Button_Update();

    switch (Button_GetEvent())
    {
    case BUTTON_EVENT_SINGLE:
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        break;

    case BUTTON_EVENT_DOUBLE:
        for (int i = 0; i < 4; i++) {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            HAL_Delay(120);
        }
        break;

    case BUTTON_EVENT_LONG:
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_Delay(2000);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        break;

    default:
        break;
    }
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */
```

## Interrupt mode (EXTI)

CubeMX setup: PA0 = `GPIO_EXTI0`, both-edge trigger, pull-up; enable EXTI line0
interrupt in the NVIC tab.

Add the callback:

```c
/* USER CODE BEGIN 0 */
volatile uint8_t button_irq_flag = 0;

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        button_irq_flag = 1;   /* keep the ISR tiny: just set a flag */
    }
}
/* USER CODE END 0 */
```

Then change the top of the loop (the `switch` stays identical to polling mode):

```c
while (1)
{
    if (button_irq_flag || Button_IsActive())
    {
        button_irq_flag = 0;
        Button_Update();
    }

    switch (Button_GetEvent())
    {
        /* ... same cases as polling mode ... */
    }
}
```
