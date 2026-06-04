/**
 ******************************************************************************
 * @file    usart.c
 * @brief   This file provides code for the configuration
 *          of the USART instances.
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

/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <stdio.h>
#include <string.h>

bufferType_t CurrentBufferType = sdoutBufferType;

uint8_t dma_uart_tx_buffer[DMA_TX_BUFFER_SIZE] = "Buffer empty \n\r";

uint8_t dma_uart_rx_buffer[DMA_RX_BUFFER_SIZE];
uint8_t sdin_buffer[SDIN_BUFFER_SIZE];

/* Para transmisi�n se escribiran los datos en un buffer circular.
 * */
cBufferHandler_t TxbufferHandler = {.BufferStart_p = dma_uart_tx_buffer,
                                    .BufferLength = DMA_TX_BUFFER_SIZE,
                                    .BufferState = BUFFER_idle,
                                    .BufferStartOffset = 0,
                                    .BufferEndOffset = 0,
                                    .DataSize = sizeof(uint8_t),
                                    .ToSend = 0,
                                    .BufferNextStartOffset = 0,
                                    .BufferDataLength = 0};
volatile size_t rxIdxBuffer = 0;

/* Para la transmisi�n se usa DMA en modo normal para enviar los datos
 * Si los datos dan la vuelta al buffer, el modulo usart solo envia datos hasta el final del buffer.
 **/

// volatile size_t dataTxNextStartOffset = 15;	/* Sera la posici�n de inicio de transmisi�n una vez que se finalice la transmis�n actual */
volatile BufferStateTypeDef TxBufferState = BUFFER_idle;
volatile BufferStateTypeDef RxBufferState = BUFFER_idle;
volatile BufferStateTypeDef SdInBufferState = BUFFER_idle;

typedef enum
{
  STATE_IDLE,
  STATE_PROCESSING,
  STATE_ERROR
} TaskState_t;

static TaskState_t tx_state = STATE_IDLE;
static cBufferHandler_t *current_buffer = &TxbufferHandler;

void comm_task(void)
{
  switch (tx_state)
  {
  case STATE_IDLE:
    if (current_buffer->BufferState == BUFFER_idle && current_buffer->BufferDataLength > 0)
    {
      tx_state = STATE_PROCESSING;
    }
    break;
  case STATE_PROCESSING:
    if (huart2.gState == HAL_UART_STATE_READY)
    {
      TransmitionStart(current_buffer);
      tx_state = STATE_IDLE;
    }
    break;
  case STATE_ERROR:
    break;
  }
}

/* USER CODE END 0 */

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (uartHandle->Instance == USART2)
  {
    /* USER CODE BEGIN USART2_MspInit 0 */

    /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = USART_TX_Pin | USART_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Channel6;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmarx, hdma_usart2_rx);

    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Channel7;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle, hdmatx, hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    /* USER CODE BEGIN USART2_MspInit 1 */

    /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{

  if (uartHandle->Instance == USART2)
  {
    /* USER CODE BEGIN USART2_MspDeInit 0 */

    /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, USART_TX_Pin | USART_RX_Pin);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
    /* USER CODE BEGIN USART2_MspDeInit 1 */

    /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/*************************
 * Write chars in the circular buffer of dma TX
 *
 * */

HAL_StatusTypeDef sendChar(char ch)
{
  if (TxbufferHandler.BufferDataLength >= TxbufferHandler.BufferLength) {
    return HAL_ERROR;
  }
  __disable_irq();
  TxbufferHandler.BufferStart_p[TxbufferHandler.BufferEndOffset] =
      (uint8_t)ch;
  TxbufferHandler.BufferEndOffset++;
  TxbufferHandler.BufferDataLength++;
  if (TxbufferHandler.BufferEndOffset >= TxbufferHandler.BufferLength)
  {
    TxbufferHandler.BufferEndOffset = 0;
  }
  __enable_irq();

  return HAL_OK;
}

int __io_putchar(int ch)
{
  sendChar(ch);

  if (TxbufferHandler.BufferState == BUFFER_idle && huart2.gState == HAL_UART_STATE_READY)
  {
    TransmitionStart(&TxbufferHandler);
  }

  return 1;
}

int __io_getchar(void)
{
  // Code to read a character from the UART
  return 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    __disable_irq();
    if (CurrentBufferType == sdoutBufferType)
    {
      TxbufferHandler.BufferStartOffset =
          TxbufferHandler.BufferNextStartOffset;
      TxbufferHandler.BufferState = BUFFER_idle;
    }
    __enable_irq();
  }
}

void TransmitionStart(cBufferHandler_t *bufferHandler)
{
  __disable_irq();
  if (huart2.gState == HAL_UART_STATE_READY)
  {
    bufferHandler->BufferState = BUFFER_busy;
    bufferHandler->BufferNextStartOffset = bufferHandler->BufferStartOffset + bufferHandler->BufferDataLength;
    if (bufferHandler->BufferNextStartOffset <= bufferHandler->BufferLength)
    {
      bufferHandler->ToSend = bufferHandler->BufferDataLength;
      if (bufferHandler->BufferStartOffset + bufferHandler->BufferDataLength == bufferHandler->BufferLength)
      {
        bufferHandler->BufferNextStartOffset = 0;
      }
      bufferHandler->BufferDataLength = 0;
    }
    else
    {
      bufferHandler->ToSend = bufferHandler->BufferLength - bufferHandler->BufferStartOffset;
      bufferHandler->BufferDataLength -= bufferHandler->ToSend;
      bufferHandler->BufferNextStartOffset = 0;
    }
  }
  __enable_irq();
  if (bufferHandler->BufferState == BUFFER_busy) {
    HAL_UART_Transmit_DMA(&huart2,
                          bufferHandler->BufferStart_p + bufferHandler->BufferStartOffset * bufferHandler->DataSize,
                          bufferHandler->ToSend * bufferHandler->DataSize);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    uartOnReceive();
  }
}

void uartOnReceive()
{
  uint32_t bytes_received = DMA_RX_BUFFER_SIZE - hdma_usart2_rx.Instance->CNDTR;
  if (bytes_received > 0 && bytes_received <= SDIN_BUFFER_SIZE)
  {
    memset(sdin_buffer, 0, SDIN_BUFFER_SIZE);
    memcpy(sdin_buffer, dma_uart_rx_buffer, bytes_received);
    sdin_buffer[bytes_received] = '\0';
    SdInBufferState = BUFFER_busy;
  }
}

void uartSetReady()
{
  HAL_UART_DMAStop(&huart2);
  uartOnReceive();
  receptionStart();
}

void receptionStart()
{
  memset(dma_uart_rx_buffer, 0, DMA_RX_BUFFER_SIZE);
  RxBufferState = BUFFER_busy;
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
  HAL_UART_Receive_DMA(&huart2, dma_uart_rx_buffer, DMA_RX_BUFFER_SIZE);
  __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
}

HAL_UART_StateTypeDef uartReceive()
{
  return huart2.RxState;
}

uint8_t *uartGetData()
{
  return sdin_buffer;
}

uint8_t uartIsReady()
{
  if (RxBufferState == BUFFER_idle)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

BufferStateTypeDef sdinIsBusy()
{
  return SdInBufferState;
}

void uartConsumeData()
{
  SdInBufferState = BUFFER_idle;
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
