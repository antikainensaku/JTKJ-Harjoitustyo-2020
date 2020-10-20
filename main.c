#include <stdio.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/i2c/I2CCC26XX.h>

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"

/* Task */
#define STACKSIZE 2048
Char labTaskStack[STACKSIZE];
// Char commTaskStack[STACKSIZE];

/* Display */
Display_Handle displayHandle;

/*mpu global variables*/
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};
/*mpu uses its own I2C*/
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

// Pin configuration and variables
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;

PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE, PIN_TERMINATE
};

PIN_Config ledConfig[] = {
   Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX, PIN_TERMINATE
};

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
   PIN_setOutputValue( ledHandle, Board_LED1, !PIN_getOutputValue( Board_LED1 ) );  // Change led state with negation
}

/* Task Functions */
Void labTaskFxn(UArg arg0, UArg arg1) {

    UART_Handle 	uart;
    UART_Params 	uartParams;

    //char echo_msg[80];

    // Open the i2c bus for other sensor
    /*
    I2C_Handle      i2c;
    I2C_Params      i2cParams;
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);

    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }

    opt3001_setup(&i2c);    // sensor setup before use

    I2C_close(i2c); // close 12C for other sensor
    */

    float ax, ay, az, gx, gy, gz;

    I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;
	I2C_Params_init(&i2cMPUParams);
	i2cMPUParams.bitRate = I2C_400kHz;
	i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams); // open I2C for mpu
	if (i2cMPU == NULL) {
	    System_abort("Error Initializing I2CMPU\n");
	}

	PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON); // mpu power on

	Task_sleep(100000 / Clock_tickPeriod); // wait 100ms for the sensor to power up
	System_printf("MPU9250: Power ON\n");
	System_flush();


	System_printf("MPU9250: Setup and calibration...\n"); // mpu setup and calibration
	System_flush();
	mpu9250_setup(&i2cMPU);
	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

	I2C_close(i2cMPU); // close I2C for mpu



    // display setup
    Display_Params params;
	Display_Params_init(&params);
	params.lineClearMode = DISPLAY_CLEAR_BOTH;

	Display_Handle displayHandle = Display_open(Display_Type_LCD, &params);

	if (displayHandle) {
	      Display_print0(displayHandle, 5, 3, "Shall we play");
	      Display_print0(displayHandle, 6, 5, "..a game?");

	      Task_sleep(3 * 1000000/Clock_tickPeriod);     // shows it for 3 seconds
	      Display_clear(displayHandle);                 // clears the display
	}

    // setup UART (check for right COM port)

	UART_Params_init(&uartParams);
	uartParams.writeDataMode = UART_DATA_TEXT;
	uartParams.readDataMode = UART_DATA_TEXT;
	uartParams.readEcho = UART_ECHO_OFF;
	uartParams.readMode=UART_MODE_BLOCKING;
	uartParams.baudRate = 9600;             // rate 9600baud
	uartParams.dataLength = UART_LEN_8;     // 8
	uartParams.parityType = UART_PAR_NONE;  // n
	uartParams.stopBits = UART_STOP_ONE;    // 1

	uart = UART_open(Board_UART0, &uartParams);     // open UART

	if (uart == NULL) {
		System_abort("Error opening the UART");
	}


    while (1) {
    	/*
    	char strlux[40];

    	sprintf(strlux, "%lf lux", opt3001_get_data(&i2c));     // gets lux value from opt3001_get_data
    	System_printf("%s\n", strlux);                          // and prints it
    	System_flush();

    	Display_print0(displayHandle, 1, 1, strlux);        // prints lux value to display
		*/

    	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    	if (i2cMPU == NULL) {
    		System_abort("Error Initializing I2CMPU\n");
    	}
    	System_printf("Haloo2\n");
    	System_flush();

    	mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
    	System_printf("ax:%f, ay:%f, az:%f, gx:%f, gy:%f, gz:%f\n", ax, ay, az, gx, gy, gz);
    	System_flush();

    	I2C_close(i2cMPU);

    	/*
    	sprintf(echo_msg,"id:266,light:%lf\n\r", opt3001_get_data(&i2cMPU));   // CSV-format
    	UART_write(uart, echo_msg, strlen(echo_msg));       // writes to UART
		*/

    	Task_sleep(1000000 / Clock_tickPeriod);     // Once per second
    }
	//PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}

/* Communication Task */
/*
Void commTaskFxn(UArg arg0, UArg arg1) {

    // Radio to receive mode
	int32_t result = StartReceive6LoWPAN();
	if(result != true) {
		System_abort("Wireless receive mode failed");
	}

    while (1) {

        // If true, we have a message
    	if (GetRXFlag() == true) {

    		// Handle the received message..
        }

    	// Absolutely NO Task_sleep in this task!!
    }
}
*/

Int main(void) {
    // Task variables
	Task_Handle labTask;
	Task_Params labTaskParams;
	/*
	Task_Handle commTask;
	Task_Params commTaskParams;
	*/

    Board_initGeneral();    // initialize board

    Board_initI2C();        // initialize i2c

    Board_initUART();       // initialize UART

    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig); // open MPU power pin
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }

    buttonHandle = PIN_open(&buttonState, buttonConfig);    // open and configure the button pin
      if(!buttonHandle) {
         System_abort("Error initializing button pins\n");
      }

      ledHandle = PIN_open(&ledState, ledConfig);           // open and configure the led pin
      if(!ledHandle) {
         System_abort("Error initializing LED pins\n");
      }

      if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {       // interrupt handler for the button
            System_abort("Error registering button callback function");
      }


    /* Task */
    Task_Params_init(&labTaskParams);
    labTaskParams.stackSize = STACKSIZE;
    labTaskParams.stack = &labTaskStack;
    labTaskParams.priority=2;

    labTask = Task_create(labTaskFxn, &labTaskParams, NULL);
    if (labTask == NULL) {
    	System_abort("Task create failed!");
    }

    /* Communication Task */
	/*
    Init6LoWPAN(); // This function call before use!

    Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority=1;

    commTask = Task_create(commTaskFxn, &commTaskParams, NULL);
    if (commTask == NULL) {
    	System_abort("Task create failed!");
    }
	*/

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();
    
    /* Start BIOS */
    BIOS_start();

    return (0);
}
