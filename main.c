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
#include <ti/drivers/i2c/I2CCC26XX.h>

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/mpu9250.h"

/* Task */
#define STACKSIZE 2048
Char labTaskStack[STACKSIZE];
Char commTaskStack[STACKSIZE];

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
static PIN_Handle PbuttonHandle;
static PIN_State PbuttonState;
static PIN_Handle ledHandle;
static PIN_State ledState;

PIN_Config buttonConfig[] = {
	Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE, PIN_TERMINATE
};

PIN_Config PbuttonConfig[] = {
	Board_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE, PIN_TERMINATE
};

PIN_Config ledConfig[] = {
   Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX, PIN_TERMINATE
};


// Global variables
uint8_t state_data = 0;

uint8_t state_mpu = 0;
uint32_t t_mpu = 0;
uint32_t t_0_mpu = 0;
uint32_t t_error = 10000;
float ax, ay, az, gx, gy, gz;

char direction[10] = "", str_moves[4] = "0";
uint8_t num_moves = 0;

uint8_t state_bp = 0;
uint32_t t_bp = 0;
uint32_t t_0_bp = 0;
uint8_t value_bp = 1;

uint8_t state_b1 = 0;
uint32_t t_b1 = 0;
uint32_t t_0_b1 = 0;
uint8_t value_b1;

uint8_t state_menu = 0;

uint32_t t_debounce = 500;
uint32_t t_longhold = 150000;

char payload_send[16];
int8_t game_result = -1;

uint8_t next_case;

// Other global stuff
I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
I2C_Params i2cMPUParams;
void SM_get_data();
void SM_mpu();
void SM_b1();
void SM_bp();
void SM_menu(Display_Handle displayHandle);



/* Task Functions */
Void labTaskFxn(UArg arg0, UArg arg1) {
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
    //sprintf(direction, "START");
    

    /*
	if (displayHandle) {
	      Display_print0(displayHandle, 5, 3, "Shall we play");
	      Display_print0(displayHandle, 6, 5, "..a game?");

	      Task_sleep(3 * 1000000/Clock_tickPeriod);     // shows it for 3 seconds
	      Display_clear(displayHandle);                 // clears the display
	}
    */

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

        SM_b1();
        SM_bp();
        SM_menu(displayHandle);

        if (state_menu == 1) {
        	Display_print0(displayHandle, 1, 2, "Start <-");
        	Display_print0(displayHandle, 2, 2, "Exit"); 	// show menu
        }

        else if (state_menu == 2) {
        	Display_print0(displayHandle, 1, 2, "Start");
        	Display_print0(displayHandle, 2, 2, "Exit  <-"); 	// show menu
            
        }

        else if (state_menu == 3) {
            SM_get_data();
            SM_mpu();

            if (state_b1 == 4 && (strlen(direction) >= 2)) {
                    ++num_moves;
                    sprintf(payload_send, "event:%s", direction);
                    Send6LoWPAN(0x1234, payload_send, strlen(payload_send));        //Send message
                    //System_printf("Direction sent: %s\n", payload_send);
                    //System_flush();
                    StartReceive6LoWPAN();      //Put radio back to reception mode
            }

        	if (state_mpu == 7) {sprintf(direction, "UP");}
			if (state_mpu == 8) {sprintf(direction, "DOWN");}
			if (state_mpu == 9) {sprintf(direction, "LEFT");}
			if (state_mpu == 10) {sprintf(direction, "RIGHT");}

	    	sprintf(str_moves, "Moves : %d", num_moves);
	    	Display_print0(displayHandle, 1, 2, str_moves);
	    	Display_print0(displayHandle, 3, 2, direction);
        }

        else if (state_menu == 4) {
        	Display_print0(displayHandle, 4, 2, "VICTORY"); 	// show victory screen
        	Task_sleep(2 * 1000000/Clock_tickPeriod);    		// shows it for 2 seconds
        }

        else if (state_menu == 5) {
        	Display_print0(displayHandle, 4, 2, "DEFEAT"); 		// show defeat screen
        	Task_sleep(2 * 1000000/Clock_tickPeriod);    		// shows it for 2 seconds
        }

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
            state_mpu = 2;
        break;

        case 2:     // check direction
            if (ay < -0.65 && ax > -0.5 && ax < 0.5) {
                t_mpu = Clock_getTicks();
                state_mpu = 3;
            }
            else if (ay > 0.65 && ax > -0.5 && ax < 0.5) {
                t_mpu = Clock_getTicks();
                state_mpu = 4;
            }
            else if (ax < -0.65 && ay > -0.5 && ay < 0.5) {
                t_mpu = Clock_getTicks();
                state_mpu = 5;
            }
            else if (ax > 0.65 && ay > -0.5 && ay < 0.5) {
                t_mpu = Clock_getTicks();
                state_mpu = 6;
            }
            else {
                state_mpu = 0;
            }
        break;

        case 3:     // error prevention UP
            t_mpu = Clock_getTicks();
            if (ay > -0.5) {
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > t_error) {
                state_mpu = 7;
            }
        break;

        case 4:     // error prevention DOWN
            t_mpu = Clock_getTicks();
            if (ay < 0.5) {
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > t_error) {
                state_mpu = 8;
            }
        break;

        case 5:     // error prevention LEFT
            t_mpu = Clock_getTicks();
            if (ax > -0.5) {
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > t_error) {
                state_mpu = 9;
            }
        break;

        case 6:     // error prevention RIGHT
            t_mpu = Clock_getTicks();
            if (ax < 0.5) {
                state_mpu = 0;
            }
            else if (t_mpu - t_0_mpu > t_error) {
                state_mpu = 10;
            }
        break;

        case 7:         // active UP
            if (ay > -0.5) {state_mpu = 11;}
        break;

        case 8:         // active DOWN
            if (ay < 0.5) {state_mpu = 11;}
        break;

        case 9:         // active LEFT
            if (ax > -0.5) {state_mpu = 11;}
        break;

        case 10:         // active RIGHT
            if (ax < 0.5) {state_mpu = 11;}
        break;

        case 11:             // back to normal
            state_mpu = 0;
        break;
    }
}


void SM_b1() {
    switch (state_b1) {
        case 0:     // reset
            state_b1 = 1;
        break;

        case 1:     // button press
            value_b1 = PIN_getInputValue(Board_BUTTON0);
            if (value_b1 == 0) {
                state_b1 = 2;
            }
        break;

        case 2:     // debounce timer start
            t_0_b1 = Clock_getTicks();
            state_b1 = 3;
        break;

        case 3:     // debounce check
            value_b1 = PIN_getInputValue(Board_BUTTON0);
            t_b1 = Clock_getTicks();

            if (value_b1 != 0) {state_b1 = 0;}
            else if (t_b1 - t_0_b1 > t_debounce) {
                state_b1 = 4;
            }
        break;

        case 4:     // send info
            state_b1 = 5;
        break;

        case 5:     // button held down
            value_b1 = PIN_getInputValue(Board_BUTTON0);
            if (value_b1 != 0) {state_b1 = 6;}
        break;

        case 6:     // button released
            state_b1 = 0;
        break;
    }
}

void SM_bp() {
    switch (state_bp) {
        case 0:     // reset
            state_bp = 1;
        break;

        case 1:     // button press
            value_bp = PIN_getInputValue(Board_BUTTON1);
            if (value_bp == 0) {
                state_bp = 2;
            }
        break;

        case 2:     // long press timer start
            System_printf("Button power pressed.\n");
            System_flush();
            t_0_bp = Clock_getTicks();
            state_bp = 3;
        break;

        case 3:     // long press check
            value_bp = PIN_getInputValue(Board_BUTTON1);
            t_bp = Clock_getTicks();

            if (value_bp != 0) {state_bp = 0;}
            else if (t_bp - t_0_bp > t_longhold) {
                state_bp = 4;
            }
        break;

        case 4:     //  button pressed
            System_printf("Power button activated.\n");
            System_flush();
            // do something (shut down?)
            state_bp = 5;
        break;

        case 5:     // button held down
            value_bp = PIN_getInputValue(Board_BUTTON1);
            if (value_bp != 0) {state_bp = 6;}
        break;

        case 6:     // button released
            System_printf("Button power released.\n");
            System_flush();
            state_bp = 0;
        break;
    }
}

void SM_menu(Display_Handle displayHandle) {
	switch (state_menu) {
		case 0:		// empty screen / power turned off
			if (state_bp == 4)		// power button was pressed -> turn the screen on
			{
				state_menu = 1;
			}
        break;

		case 1:		// display menu
			if (state_b1 == 4)  {   // button was pressed, go to next option}
                next_case = 2;
                state_menu = 6;
            } 
			if (state_bp == 4) {    // choose current option (start the game)
                sprintf(direction, "");
                num_moves = 0;
                next_case = 3;
                state_menu = 6;
            } 
        break;

		case 2:
            if (state_b1 == 4) {    // button was pressed, go to previous option
                next_case = 1;
                state_menu = 6;
            }
            if (state_bp == 4) {    // choose current option (turn power off)
                next_case = 0;
                state_menu = 6;
            }
        break;

        case 3:        // display game window
            if (game_result == 1) {
                next_case = 4;
                state_menu = 6;        // won game
            }
            if (game_result == 0) {
                next_case = 5;
                state_menu = 6;        // lost game
            }
            if (state_bp == 4)  {
                next_case = 1;
                state_menu = 6;        // power button was pressed in the middle of the game
            }                        // -> go to main menu
        break;

		case 4:
			game_result = -1;			// reset game_result variable
			state_menu = 1;			// show victory screen and go to main menu
        break;

		case 5:
			game_result = -1;			// reset game_result variable
			state_menu = 1;			// show defeat screen and go to main menu
        break;

        case 6:
            Display_clear(displayHandle);
            state_menu = next_case;
        break;
	}
}


/* Communication Task */
Void commTaskFxn(UArg arg0, UArg arg1) {

    char payload_receive[16];     // Buffer
    uint16_t senderAddr;

	int32_t receive_rf = StartReceive6LoWPAN();     // Radio to receive mode
	if(receive_rf != true) {
		System_abort("Wireless receive mode failed");
	}

    while (1) {
    	if (GetRXFlag() == true) {          // If true, we have a message
            memset(payload_receive,0,16);                        // Empty the buffer
            Receive6LoWPAN(&senderAddr, payload_receive, 16);    // Read message into the buffer
            System_printf("payload: %s payload[4]: %c", payload_receive, payload_receive[4]);                        // Print received message to console screen
            System_flush();
        }
        uint32_t win = "266,WIN";
        uint32_t lost = "266,LOST GAME";
        if (payload_receive == "266,WIN")  {
            System_printf("if-lause lol\n");                        // Print received message to console screen
            System_flush();
            game_result = 1; 	// won game
        }
        if (payload_receive == "266,LOST GAME") {
            game_result = 0;		// lost game
        }
    }
}


Int main(void) {
    // Task variables
	Task_Handle labTask;
	Task_Params labTaskParams;
	Task_Handle commTask;
	Task_Params commTaskParams;

    Board_initGeneral();    // initialize board

    Board_initI2C();        // initialize i2c

    //Board_initUART();       // initialize UART

    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig); // open MPU power pin
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }

    buttonHandle = PIN_open(&buttonState, buttonConfig);    // open and configure the button pin
        if(!buttonHandle) {
        System_abort("Error initializing button pins\n");
        }
    
    PbuttonHandle = PIN_open(&PbuttonState, PbuttonConfig);    // open and configure the power button pin
        if(!PbuttonHandle) {
            System_abort("Error initializing Power button pins\n");
        }

    ledHandle = PIN_open(&ledState, ledConfig);           // open and configure the led pin
        if(!ledHandle) {
            System_abort("Error initializing LED pins\n");
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
    Init6LoWPAN(); // This function call before use!

    Task_Params_init(&commTaskParams);
    commTaskParams.stackSize = STACKSIZE;
    commTaskParams.stack = &commTaskStack;
    commTaskParams.priority=1;

    commTask = Task_create(commTaskFxn, &commTaskParams, NULL);
    if (commTask == NULL) {
    	System_abort("Task create failed!");
    }

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();
    
    /* Start BIOS */
    BIOS_start();

    return (0);
}
