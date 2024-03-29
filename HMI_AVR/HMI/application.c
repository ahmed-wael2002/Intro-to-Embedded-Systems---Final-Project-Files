/*
 * application.c
 *
 *  Created on: Nov 27, 2023
 *      Author: Wael
 */

/*******************************************************************************
 *                      Libraries                                    *
 *******************************************************************************/
#include "buzzer.h"
#include "uart.h"
#include "lcd.h"
#include "led.h"
#include "std_types.h"
#include "protocol.h"
#include "util/delay.h"
#include "int.h"

/*******************************************************************************
 *                      Types Definition                                    *
 *******************************************************************************/
typedef enum{
	OFF,
	ON
}State;

volatile State mute_flag = ON;
volatile State system = OFF;

/*******************************************************************************
 *                      Function Prototypes                                   *
 *******************************************************************************/
void notifyUser(void);
void Toggle_System_Fn(void);
void Mute_Button_Fn(void);

/*******************************************************************************
 *                      Application                                    *
 *******************************************************************************/
int main(void){

	/* ****** Module Initializations ****** */
	LED_init();
	LCD_init();
	Buzzer_init();

	UART_ConfigType uart_configs = {9600,DISABLED,ONE_BIT,BIT_8};
	UART_init(&uart_configs);

	INT0_init(FALLING);
	INT0_setCallback(Mute_Button_Fn);

	INT1_init(FALLING);
	INT1_setCallback(Toggle_System_Fn);


	/* ****** System Variables ****** */
	uint8 cmd;
	uint8 before;

	/* ****** Application ****** */
	while(1){
		while(system == OFF);	// Sleep mode

		cmd = UART_recieveByte();
		if(cmd == before)
			continue;

		switch(cmd){

		/**********************************************/
		/*                 Fire Command               */
		/**********************************************/
		case FIRE_ALERT:
			LCD_displayString("Fire is reported!");
			notifyUser();
			break;

		/**********************************************/
		/*             Door Opened Command            */
		/**********************************************/
		case DOOR_OPENED_ALERT:
			LCD_displayString("Door is Opened!");
			notifyUser();
			break;

		/**********************************************/
		/*             Door Closed Command            */
		/**********************************************/
		case DOOR_CLOSED_ALERT:
			LCD_displayString("Door is Closed!");
			for(uint8 i=0; i<3; i++){
				Buzzer_on();
				_delay_ms(100);
				Buzzer_off();
				_delay_ms(100);
			}
			LCD_clearScreen();
			break;

		/**********************************************/
		/*         Dangerous Distance Command         */
		/**********************************************/
		case DISTANCE_ALERT:
			LCD_displayString("Intruder Alert!");
			notifyUser();
			break;
		}
		before = cmd;
		_delay_ms(150);
	}
	return 0;
}

/*******************************************************************************
 *                      Function Definitions                                   *
 *******************************************************************************/
void notifyUser(void){
	LED_turnOn(RED);
	Buzzer_on();
	mute_flag = OFF;
	while(mute_flag == OFF);

	/* Reset System */
	LED_turnOff(RED);
	Buzzer_off();
	LCD_clearScreen();
}

void Mute_Button_Fn(void){
	mute_flag = ON;
}

void Toggle_System_Fn(void){
	if(system == ON){
		LCD_clearScreen();
		LCD_displayString("Turning Off");
		_delay_ms(2000);
		LCD_clearScreen();
		system = OFF;
	}
	else{
		LCD_clearScreen();
		LCD_displayString("Turning On");
		_delay_ms(2000);
		LCD_clearScreen();
		system = ON;
	}
}
