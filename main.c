#include <stdio.h>
#include <time.h>

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



// Global variables
int state_data = 0;

int state_mpu = 0;
unsigned long t_mpu = 0;
unsigned long t_0_mpu = 0;
unsigned long debounce_time = 3000;
float ax, ay, az, gx, gy, gz;

char direction[10] = "START", str_moves[10] = "0";
int num_moves = 0;

// Other global stuff
I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
I2C_Params i2cMPUParams;
void SM_get_data();
void SM_mpu();


/* Task Functions */
Void labTaskFxn(UArg arg0, UArg arg1) {

    char direction[10], str_moves[10];

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

    /*
    I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams; */
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
	/*
	UART_Handle 	uart;
    UART_Params 	uartParams;

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
	*/


    while (1) {
    	/*								 **Light sensor value printed to display-code** 
    	char strlux[40];

    	sprintf(strlux, "%lf lux", opt3001_get_data(&i2c));     // gets lux value from opt3001_get_data
    	System_printf("%s\n", strlux);                          // and prints it
    	System_flush();

    	Display_print0(displayHandle, 1, 1, strlux);        // prints lux value to display
    	Display_clear(displayHandle);                 // clears the display
		*/

		SM_get_data();
		SM_mpu();

		if (state_mpu == 4) {sprintf(direction, "UP");}
		if (state_mpu == 5) {sprintf(direction, "DOWN");}
		if (state_mpu == 6) {sprintf(direction, "LEFT");}
		if (state_mpu == 7) {sprintf(direction, "RIGHT");}

    	sprintf(str_moves, "Moves : %d", num_moves);
    	Display_print0(displayHandle, 1, 2, str_moves);
    	Display_print0(displayHandle, 3, 2, direction);

    	/*
    	sprintf(echo_msg,"id:266,light:%lf\n\r", opt3001_get_data(&i2cMPU));   // CSV-format
    	UART_write(uart, echo_msg, strlen(echo_msg));       // writes to UART
		*/
    }
	//PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}

void SM_get_data() {
    switch (state_data) {
        case 0:     // reset
            state_data = 1;
        break;

        case 1:     // start
            i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            if (i2cMPU == NULL) {
                System_abort("Error Initializing I2CMPU\n");
            }
            else {state_data = 2;}
        break;

        case 2:     // get data
            mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
            I2C_close(i2cMPU);
            //System_printf("Took data\n");
            //System_flush();
            state_data = 1;
        break;
    }
}


void SM_mpu() {
    switch (state_mpu) {
        case 0:     // reset
            state_mpu = 1;
        break;

        case 1:     // start 
            t_0_mpu = Clock_getTicks();
            state_mpu = 3;
        break;

        case 2:     // get data
            
        break;

        case 3:     // check direction
            if (ay < -0.5 && ax > -0.5 && ax < 0.5) {
                t_mpu = Clock_getTicks();
                //System_printf("Ticks: %lu\n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 9;
            }
            else if (ay > 0.5 && ax > -0.5 && ax < 0.5) {
                t_mpu = Clock_getTicks();
                //System_printf("Ticks: %lu\n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 10;
            }
            else if (ax < -0.5 && ay > -0.5 && ay < 0.5) {
                t_mpu = Clock_getTicks();
                //System_printf("Ticks: %lu\n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 11;
            }
            else if (ax > 0.5 && ay > -0.5 && ay < 0.5) {
                t_mpu = Clock_getTicks()/10;
                //System_printf("Ticks: %lu\n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 12;
            }
            else {
                state_mpu = 0;
            }
        break;

        case 4:         // UP
            if (ay > -0.5) {state_mpu = 8;}
        break;

        case 5:         // DOWN
            if (ay < 0.5) {state_mpu = 8;}
        break;

        case 6:         // LEFT
            if (ax > -0.5) {state_mpu = 8;}
        break;

        case 7:         // RIGHT
            if (ax < 0.5) {state_mpu = 8;}
        break;

        case 8:             // back to normal
            ++num_moves;
            state_mpu = 0;
        break;

        case 9:
            t_mpu = Clock_getTicks();
            if (ay > -0.5) {
                System_printf("Error prevention\n");
                System_flush();
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > debounce_time) {
                //System_printf("Other Ticks: %lu \n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 4;
            }
        break;

        case 10:
            t_mpu = Clock_getTicks();
            if (ay < 0.5) {
                System_printf("Error prevention\n");
                System_flush();
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > debounce_time) {
                //System_printf("Other Ticks: %lu \n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 5;
            }
        break;

        case 11:
            t_mpu = Clock_getTicks();
            if (ax > -0.5) {
                System_printf("Error prevention\n");
                System_flush();
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > debounce_time) {
                //System_printf("Other Ticks: %lu \n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 6;
            }
        break;

        case 12:
            t_mpu = Clock_getTicks();
            if (ax < 0.5) {
                System_printf("Error prevention\n");
                System_flush();
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > debounce_time) {
                //System_printf("Other Ticks: %lu \n", (t_mpu - t_0_mpu));
                //System_flush();
                state_mpu = 7;
            }
        break;
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
