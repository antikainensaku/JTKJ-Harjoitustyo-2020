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

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"

/* Task */
#define STACKSIZE 2048
Char labTaskStack[STACKSIZE];
// Char commTaskStack[STACKSIZE];

/* Display */
Display_Handle displayHandle;

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
    I2C_Handle      i2c;
    I2C_Params      i2cParams;

    UART_Handle 	uart;
    UART_Params 	uartParams;

    char echo_msg[80];

    // Open the i2c bus
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);

    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }

    opt3001_setup(&i2c);    // sensor setup before use

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
    	char strlux[40];

    	sprintf(strlux, "%lf lux", opt3001_get_data(&i2c));     // gets lux value from opt3001_get_data
    	System_printf("%s\n", strlux);                          // and prints it
    	System_flush();

    	Display_print0(displayHandle, 1, 1, strlux);        // prints lux value to display

    	sprintf(echo_msg,"id:266,light:%lf\n\r", opt3001_get_data(&i2c));   // CSV-format
    	UART_write(uart, echo_msg, strlen(echo_msg));       // writes to UART

    	Task_sleep(1000000 / Clock_tickPeriod);     // Once per second
    }
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
