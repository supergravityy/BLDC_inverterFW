#pragma once 

#include "stm32f767xx.h"
#include <stdbool.h>

#define UTILS_BIT_SHIFT(pos,val)        (val << pos)
#define UTILS_BIT_MASK(pos,len)         (((1UL << (len)) - 1) << (pos)) // 시작비트 포지션에서 원하는 크기만큼 마스크를 만듬
// 비트를 길이만큼 왼쪽으로 밀고, 1을 빼서 하위비트들을 전부 1로만듬 -> 이 비트들을 다시 시프팅함

#define PI 3.14159f

#define UTILS_DISABLE_ISR()             __disable_irq()
#define UTILS_ENABLE_ISR()              __enable_irq()
#define UTILS_ASSERT_FUNC()             do{while(1);}while(0)

uint16_t utils_ipol_u16u16(const uint16_t* mapX, const uint16_t* mapY, uint16_t mapSize, uint16_t input);
uint32_t utils_ipol_u16u32(const uint16_t* mapX, const uint32_t* mapY, uint16_t mapSize, uint16_t input);
int16_t utils_ipol_s16s16(const int16_t* mapX, const int16_t* mapY, uint16_t mapSize, int16_t input);
int16_t utils_ipol_s16u16(const int16_t* mapX, const uint16_t* mapY, uint16_t mapSize, int16_t input);
int16_t utils_ipol_u32s16(const uint32_t* mapX, const int16_t* mapY, uint16_t mapSize, uint32_t input);