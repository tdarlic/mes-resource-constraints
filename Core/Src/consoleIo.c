// Console IO is a wrapper between the actual in and output and the console code
// In an embedded system, this might interface to a UART driver.
#include "stm32f4xx_hal.h"
#include "consoleIo.h"
#include <stdio.h>
#include <string.h>


UART_HandleTypeDef *consoleUartHandle;

uint8_t consoleByteBuffer = 0;

// This buffer will hold command
char consoleCommandBuffer[20];

uint16_t consoleBufferCount = 0;

uint8_t consoleCommandComplete = 0;


eConsoleError ConsoleReset(void);
eConsoleError ConsoleByteReset(void);

eConsoleError ConsoleIoInit(UART_HandleTypeDef *huart)
{
	consoleUartHandle = huart;
	ConsoleReset();
	HAL_UART_Receive_IT(consoleUartHandle, &consoleByteBuffer, 1);
	return CONSOLE_SUCCESS;
}

/**
 * Resets the console
 */
eConsoleError ConsoleReset(void)
{
	consoleByteBuffer = 0;
	consoleCommandComplete = 0;
	consoleBufferCount = 0;
	memset(consoleCommandBuffer, 0, 20);
	return CONSOLE_SUCCESS;
}

// This is modified for the Wokwi RPi Pico simulator. It works fine 
// but that's partially because the serial terminal sends all of the 
// characters at a time without losing any of them. What if this function
// wasn't called fast enough?
eConsoleError ConsoleIoReceive(uint8_t *buffer, const uint32_t bufferLength, uint32_t *readLength)
{
    uint16_t i = 0;
    *readLength = 0;

    if (consoleCommandComplete){
		while (consoleCommandBuffer[i] != 0x00){
			buffer[i] = consoleCommandBuffer[i];
			i++;
		}
		*readLength = i;
		ConsoleReset();
    }
    
	return CONSOLE_SUCCESS;
}

eConsoleError ConsoleIoSendString(const char *buffer)
{
	printf("%s", buffer);
	return CONSOLE_SUCCESS;
}

/*
 * This function gets called after completion of RX cycle on UART
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	char lastChar = 0;
	// if console command complete it means that previous command was not parsed yet
	if (!consoleCommandComplete){
		lastChar = consoleByteBuffer;
		consoleCommandBuffer[consoleBufferCount++] = lastChar;
		if ('\n' == lastChar){
			consoleCommandComplete = 1;
		}
		HAL_UART_Receive_IT(consoleUartHandle, &consoleByteBuffer, 1);
	}
}

