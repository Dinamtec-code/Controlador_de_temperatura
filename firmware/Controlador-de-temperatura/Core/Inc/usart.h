/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */
#define DMA_TX_BUFFER_SIZE 64
#define DMA_RX_BUFFER_SIZE 64
#define SDIN_BUFFER_SIZE 64

typedef enum State{
	BUFFER_idle = 0,
	BUFFER_busy
}BufferStateTypeDef;

typedef struct C_BUFFER_HANDLER{
	uint8_t * BufferStart_p;
	size_t BufferLength;
	size_t BufferStartOffset; 		/* El encargado de cambiar el offset es la interrupción sobre el final de la transmisión*/
	size_t BufferEndOffset;		    /* Es la posición donde se va a escribir el próximo dato */
	size_t ToSend;
	size_t BufferNextStartOffset;
	size_t BufferDataLength;				/* Usado para indicarle al modulo DMA cuantos datos se van a enviar */
	size_t DataSize;
	BufferStateTypeDef BufferState;
}cBufferHandler_t;

typedef enum BUFFER_TYPE{
	dataBufferType,
	sdoutBufferType,
	errorBufferType,
	sdinBufferType
}bufferType_t;

/* USER CODE END Private defines */

void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
void TransmitionStart(cBufferHandler_t*);
HAL_StatusTypeDef sendChar(char ch);

void ReceptionStart();
void uartSetReady();
void uartOnReceive();
void uartConsumeData();
HAL_UART_StateTypeDef uartReceive();
uint8_t* uartGetData();
uint8_t uartIsReady();
BufferStateTypeDef sdinIsBusy();

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
