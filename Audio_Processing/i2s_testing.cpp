#include <Arduino.h>
#include "stm32f3xx_hal.h"

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_rx;

volatile int16_t i2sBuffer[512];
volatile bool bufferReady = false;

void I2S3_GPIO_Init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // PC10 (CK) and PC11 (ext_SD)
    GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // PA4 (WS)
    GPIO_InitStruct.Pin       = GPIO_PIN_4;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void I2S3_DMA_Init() {
    __HAL_RCC_DMA2_CLK_ENABLE();

    hdma_spi3_rx.Instance                 = DMA2_Channel1;
    hdma_spi3_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_spi3_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_spi3_rx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_spi3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_spi3_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_spi3_rx.Init.Mode                = DMA_CIRCULAR;
    hdma_spi3_rx.Init.Priority            = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdma_spi3_rx);

    __HAL_LINKDMA(&hi2s3, hdmarx, hdma_spi3_rx);

    HAL_NVIC_SetPriority(DMA2_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Channel1_IRQn);
}

void I2S3_Init() {
    __HAL_RCC_SPI3_CLK_ENABLE();

    I2S3_GPIO_Init();
    I2S3_DMA_Init();

    hi2s3.Instance            = SPI3;
    hi2s3.Init.Mode           = I2S_MODE_SLAVE_RX;
    hi2s3.Init.Standard       = I2S_STANDARD_PHILIPS;
    hi2s3.Init.DataFormat     = I2S_DATAFORMAT_16B;
    hi2s3.Init.MCLKOutput     = I2S_MCLKOUTPUT_DISABLE;
    hi2s3.Init.AudioFreq      = I2S_AUDIOFREQ_44K;
    hi2s3.Init.CPOL           = I2S_CPOL_LOW;
    hi2s3.Init.ClockSource    = I2S_CLOCK_EXTERNAL;
    hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_ENABLE;

    if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
        Serial.println("I2S init failed!");
        while(1);
    }
}

// fires when DMA buffer is full
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s) {
    bufferReady = true;
}

// DMA interrupt handler
extern "C" void DMA2_Channel1_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_spi3_rx);
}

void setup() {
    Serial.begin(115200);
    HAL_Init();
    I2S3_Init();

    // start continuous DMA receive
    HAL_I2S_Receive_DMA(&hi2s3, (uint16_t*)i2sBuffer, 512);

    // // button toggle
    // pinMode(PB0, INPUT_PULLUP);
    // attachInterrupt(digitalPinToInterrupt(PB0), buttonISR, FALLING);
}

void loop() {
    // nothing needed here, audio handled in ISR
}