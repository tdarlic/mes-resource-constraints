// ConsoleCommands.c
// This is where you add commands:
//		1. Add a protoype
//			static eCommandResult_T ConsoleCommandVer(const char buffer[]);
//		2. Add the command to mConsoleCommandTable
//		    {"ver", &ConsoleCommandVer, HELP("Get the version string")},
//		3. Implement the function, using ConsoleReceiveParam<Type> to get the parameters from the buffer.
#include <stdio.h>
#include <string.h>
#include "lcd_log.h"
#include "consoleCommands.h"
#include "console.h"
#include "consoleIo.h"
#include "version.h"
#include "../../Drivers/Components/i3g4250d/i3g4250d.h"
#include "../../Drivers/STM32F429I-Discovery/stm32f429i_discovery_gyroscope.h"


#define IGNORE_UNUSED_VARIABLE(x)  if ( &x == &x ) {}
#define ABS(x)  (x < 0) ? (-x) : x

#define GBAR_LEN 8
#define GDISP_LEN ((GBAR_LEN * 2) + 2)
//Set the full scale for the gyro
#define GYRO_SCALE I3G4250D_FULLSCALE_245

static eCommandResult_T ConsoleCommandVer(const char buffer[]);
static eCommandResult_T ConsoleCommandHelp(const char buffer[]);
static eCommandResult_T ConsoleCommandParamExampleInt16(const char buffer[]);
static eCommandResult_T ConsoleCommandParamExampleHexUint16(const char buffer[]);
static eCommandResult_T ConsoleCommandGyroPresent(const char buffer[]);
static eCommandResult_T ConsoleCommandGyroTest(const char buffer[]);
static eCommandResult_T ConsoleCommandComment(const char buffer[]);
static eCommandResult_T ConsoleTestScreenLog(const char buffer[]);

static const sConsoleCommandTable_T mConsoleCommandTable[] =
{
    {";", &ConsoleCommandComment, HELP("Comment! You do need a space after the semicolon. ")},
    {"help", &ConsoleCommandHelp, HELP("Lists the commands available")},
    {"ver", &ConsoleCommandVer, HELP("Get the version string")},
    {"int", &ConsoleCommandParamExampleInt16, HELP("How to get a signed int16 from params list: int -321")},
    {"u16h", &ConsoleCommandParamExampleHexUint16, HELP("How to get a hex u16 from the params list: u16h aB12")},
	{"gp", &ConsoleCommandGyroPresent, HELP("Check is gyro present and responding")},
	{"gt", &ConsoleCommandGyroTest, HELP("Test gyro: params 10 - number of seconds to test")},
	{"slt", &ConsoleTestScreenLog, HELP("Screen log test")},

	CONSOLE_COMMAND_TABLE_END // must be LAST
};

static eCommandResult_T ConsoleTestScreenLog(const char buffer[])
{
	ConsoleIoSendString("Testing screen log\n");
	/* Show Header and Footer texts */
    BSP_LCD_Init();
    //LCD_LOG_Init();
    //LCD_LOG_SetHeader((uint8_t*)"This is the header");
    //LCD_LOG_SetFooter((uint8_t*)"This is the footer");


    BSP_LCD_Clear(LCD_COLOR_WHITE);

    return COMMAND_SUCCESS;
}

/**
 * @brief Draws gyro bars in ASCII
 * @param buffer *char Pointer to string which will be output to console
 * @param val float Value to be displayed
 * @param blen uint16_t Length of buffer
 */
static void getAxisBar(char * buffer, float val, uint16_t blen){
    uint8_t lenang = 0;
    uint16_t gyro_scale_divider;
    // calculate the individual display strings
    memset(buffer, 0x00, blen);
    //Xval = -100.00f; // debug
    switch (GYRO_SCALE){
        case I3G4250D_FULLSCALE_245:
            gyro_scale_divider = 245;
            break;
        case I3G4250D_FULLSCALE_500:
            gyro_scale_divider = 500;
            break;
        case I3G4250D_FULLSCALE_2000:
            gyro_scale_divider = 2000;
            break;
        default:
            gyro_scale_divider = 245;
    }

    lenang = (ABS((val/gyro_scale_divider) * GBAR_LEN));
    if (lenang > GBAR_LEN) {
        lenang = GBAR_LEN;
    }
    if (val < 0){
        memset(buffer, ' ', GBAR_LEN - lenang);
        memset(buffer + GBAR_LEN - lenang, '*', lenang);
        memset(buffer + GBAR_LEN, '-', 1);
        memset(buffer + GBAR_LEN + 1, ' ', GBAR_LEN);
    } else {
        if (0 == val){
            memset(buffer, ' ', GBAR_LEN);
            memset(buffer + GBAR_LEN, '0', 1);
            memset(buffer + GBAR_LEN + 1, ' ', GBAR_LEN);
        } else {
            memset(buffer, ' ', GBAR_LEN);
            memset(buffer + GBAR_LEN, '+', 1);
            memset(buffer + GBAR_LEN + 1, '*', lenang);
            memset(buffer + GBAR_LEN + 1 + lenang, ' ', GBAR_LEN - lenang);
        }
    }
}

static eCommandResult_T ConsoleCommandGyroTest(const char buffer[]){
	float Buffer[3];
	float Xval, Yval, Zval = 0x00;
	int16_t tsec;
    char strbuf[100];
    eCommandResult_T result;
    uint32_t endTick = 0;
    char xbuf[GDISP_LEN], ybuf[GDISP_LEN], zbuf[GDISP_LEN];

    result = ConsoleReceiveParamInt16(buffer, 1, &tsec);

    if (COMMAND_SUCCESS == result ){
		if (BSP_GYRO_Init(GYRO_SCALE) == GYRO_OK){
			ConsoleIoSendString("Starting test:\n");
			if (tsec < 1){
				ConsoleIoSendString("Error in duration of test: < 1");
				return COMMAND_PARAMETER_ERROR;
			}
            //function will exit after tsec
			endTick = HAL_GetTick() + (tsec * 1000);

			while(HAL_GetTick() < endTick){

				/* Read Gyro Angular data */
				BSP_GYRO_GetXYZ(Buffer);

				// device is outputting mdps (millidegrees per second)
				// to get DPS we need to divide by 1000
				Xval = (Buffer[0]/1000);
				Yval = (Buffer[1]/1000);
				Zval = (Buffer[2]/1000);
				//Reset the string buffer
				memset(strbuf, 0x00, 100);


				//sprintf(strbuf, "%+06.2f | %+06.2f | %+06.2f\n", Xval, Yval, Zval);
				//ConsoleIoSendString(strbuf);

				getAxisBar(xbuf, Xval, GDISP_LEN);
				getAxisBar(ybuf, Yval, GDISP_LEN);
				getAxisBar(zbuf, Zval, GDISP_LEN);
				sprintf(strbuf, "%s|%s|%s\n", xbuf, ybuf, zbuf);

				ConsoleIoSendString(strbuf);

				// Delay for 10 ms
				HAL_Delay(10);
			}
			ConsoleIoSendString("Test completed\n");
		}
    } else {
    	return COMMAND_ERROR;
    }
	return COMMAND_SUCCESS;
}


/**
 * Testing is the gyro present or not
 * In case that the gyro is present the device will return Gyro OK else Gyro error
 */
static eCommandResult_T ConsoleCommandGyroPresent(const char buffer[]){
	if (BSP_GYRO_Init(GYRO_SCALE) == GYRO_OK){
		ConsoleIoSendString("Gyro OK\n");
	} else {
		ConsoleIoSendString("Gyro Error\n");
	}
	return COMMAND_SUCCESS;
}

static eCommandResult_T ConsoleCommandComment(const char buffer[])
{
	// do nothing
	IGNORE_UNUSED_VARIABLE(buffer);
	return COMMAND_SUCCESS;
}

static eCommandResult_T ConsoleCommandHelp(const char buffer[])
{
	uint32_t i;
	uint32_t tableLength;
	eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

	tableLength = sizeof(mConsoleCommandTable) / sizeof(mConsoleCommandTable[0]);
	for ( i = 0u ; i < tableLength - 1u ; i++ )
	{
		ConsoleIoSendString(mConsoleCommandTable[i].name);
#if CONSOLE_COMMAND_MAX_HELP_LENGTH > 0
		ConsoleIoSendString(" : ");
		ConsoleIoSendString(mConsoleCommandTable[i].help);
#endif // CONSOLE_COMMAND_MAX_HELP_LENGTH > 0
		ConsoleIoSendString(STR_ENDLINE);
	}
	return result;
}

static eCommandResult_T ConsoleCommandParamExampleInt16(const char buffer[])
{
	int16_t parameterInt;
	eCommandResult_T result;
	result = ConsoleReceiveParamInt16(buffer, 1, &parameterInt);
	if ( COMMAND_SUCCESS == result )
	{
		ConsoleIoSendString("Parameter is ");
		ConsoleSendParamInt16(parameterInt);
		ConsoleIoSendString(" (0x");
		ConsoleSendParamHexUint16((uint16_t)parameterInt);
		ConsoleIoSendString(")");
		ConsoleIoSendString(STR_ENDLINE);
	}
	return result;
}
static eCommandResult_T ConsoleCommandParamExampleHexUint16(const char buffer[])
{
	uint16_t parameterUint16;
	eCommandResult_T result;
	result = ConsoleReceiveParamHexUint16(buffer, 1, &parameterUint16);
	if ( COMMAND_SUCCESS == result )
	{
		ConsoleIoSendString("Parameter is 0x");
		ConsoleSendParamHexUint16(parameterUint16);
		ConsoleIoSendString(STR_ENDLINE);
	}
	return result;
}

static eCommandResult_T ConsoleCommandVer(const char buffer[])
{
	eCommandResult_T result = COMMAND_SUCCESS;

    IGNORE_UNUSED_VARIABLE(buffer);

	ConsoleIoSendString(VERSION_STRING);
	ConsoleIoSendString(STR_ENDLINE);
	return result;
}


const sConsoleCommandTable_T* ConsoleCommandsGetTable(void)
{
	return (mConsoleCommandTable);
}


