#ifndef _INC_TIMER_H
#define _INC_TIMER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32f446xx.h"

    void TimerInit();
    void DelayUS(uint16_t delay);
    void DelayMS(uint16_t delay);
    void TIM2_Init();
    uint32_t GetTick();

#ifdef __cplusplus
}
#endif

#endif