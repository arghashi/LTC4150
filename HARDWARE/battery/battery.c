#include "battery.h"


//Change the following two lines to match your battery
//and its initial charge state

volatile double battery_mAh = 2000.00; //The total mA of the battery ------> need to be adjusted according to the actual situation
volatile double battery_percent = 100.00;  //Charge status (percentage) (unit: %) ------> need to be adjusted according to the actual situation

//global variable（“volatile” Indicates that interrupts can change them behind the scenes)：
volatile int isrflag;		//Can only be 1 or 0, used to determine whether the interrupt function is triggered
volatile long int time, lasttime;
volatile double mA;
double ah_quanta = 0.17067759; //milliamp hours per INT
double percent_quanta; //Calculated with the following, it means the percent decrease or increase per INT

/*******************************************************************************
*function prototype：void battery_init(void)
*function of function：The LTC4150 initializes the pins and calculates persistent_quanta by the way
*Arguments of the function: None
*Function return value: None
*Description of the function:
*Function Writer: Porcupine
*Function writing date: 2021/7/21
*The version number of the function: V1.0
********************************************************************************/

void battery_init(void)  //initialization pin
{
	GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOB, ENABLE );

    GPIO_InitStructure.GPIO_Pin = SHDN_Pin;//SHDN pin
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP ;   //Push-pull output
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = INT_Pin;//INT pin
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   //pull-up input
	GPIO_Init(GPIOB,&GPIO_InitStructure); 	
	
	GPIO_InitStructure.GPIO_Pin = POL_Pin;//POL pin
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU ;   //pull-up input
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	percent_quanta = 1.0 / (battery_mAh / 1000.0 * 5859.0 / 100.0);
	isrflag =0;
	
	SHDN = 1;  //Pull up SHDN pin level, LTC4150 defaults to working state
}


/*******************************************************************************
*函数的原型：int battery_stop(void)
*函数的功能：使LTC4150处与关断状态
*函数的参数：None
*函数返回值：None
*函数的说明：
*函数编写者: 豪猪
*函数编写日期：2021/7/21
*函数的版本号：V1.0
********************************************************************************/
void battery_stop(void)
{
	SHDN = 0;
}


/*******************************************************************************
*Prototype of the function: int battery_res(void)
* The function of the function: make the LTC4150 reset
* Arguments of the function: None
*Function return value: None
*Description of the function:
*Function Writer: Porcupine
*Function writing date: 2021/7/21
*The version number of the function: V1.0
********************************************************************************/
void battery_res(void)
{
	SHDN = 0;
	delay_ms(3000);
	SHDN = 1;
}

/*******************************************************************************
*Prototype of the function: void myISR(void)
*The function of the function: used to be triggered by an interrupt
* Arguments of the function: None
*Function return value: None
*Description of the function:
*Function Writer: Porcupine
*Function writing date: 2021/7/21
*The version number of the function: V1.0
********************************************************************************/
void myISR(void)// When INT is the falling edge, it is triggered by an interrupt, calculates the data
{
	static int polarity;//can only be 1 or 0

	// Determine the delay since the last interrupt (for mA calculation)
        // Note that the first measurement will be incorrect (no previous time!)
	lasttime = time;
	time = (millis() * 1000);

	// Get the polarity value and use it to judge the current direction
	polarity = POL;
	
	if (polarity) // POL output level high = charging (current direction IN—>OUT)
	{
		battery_mAh += ah_quanta;
		battery_percent += percent_quanta;
	}
	else // POL output level is low = no charge (current direction OUT—>IN)
	{
		battery_mAh -= ah_quanta;
		battery_percent -= percent_quanta;
	}
	//The following calculation current can be removed
	//Calculate mA from delay (optional)

	mA = 614.4 / ((time-lasttime) / 1000000.0);

	// If charging, we will make mA negative (optional)

	if (polarity) mA = mA * -1.0;

	// Set the isrflag so that the main loop knows that an interrupt has occurred

	isrflag = 1;
}



/*******************************************************************************
*The prototype of the function: void battery_start(void)
*Function of function: Start the function of the LTC4150
* Arguments of the function: None
*Function return value: None
*Description of the function:
*Function Writer: Porcupine
*Function writing date: 2021/7/21
*The version number of the function: V1.0
********************************************************************************/

void battery_start(void)
{
	// When we detect an INT signal, the myISR() function will run automatically. myISR() sets isrflag to 1
	// That way we know what's going on.
	char data1[8];
	char data2[6];
	char data3[6];
	char data4[7];	
	
	if (isrflag)  //Determine whether the INT pin triggers an interrupt
	{
	// reset the flag to 0 so that each INT is executed only once
	isrflag = 0;

	// The serial port prints the current state (variable calculated by myISR())
	printf("mAh: ");
	printf("%f", battery_mAh);
	printf(" soc: ");
	printf("%f", battery_percent);
	printf("%% time: ");
	printf("%f", (time-lasttime)/1000000.0);
	printf("s mA: ");
	printf("%f\n\n", mA);
		
	//OLED screen showing current data
	sprintf(data1, "%.2f" ,battery_mAh);     //Current battery level
	sprintf(data2, "%.2f" ,battery_percent);	//Battery percentage remaining
	sprintf(data3, "%.2f" ,((time-lasttime)/1000000.0));	//Time required for one INT
	sprintf(data4, "%.2f" ,mA);		//Current size
	Oled_Display_String(0, 48, data1);
	Oled_Display_String(2, 48, data2);
	Oled_Display_String(4, 56, data3);
	Oled_Display_String(6, 40, data4);
	}
	// You can run your own code in the main loop()
	// myISR() will automatically update the information
	// If needed, set isrflag to let you know that something has changed.

}

