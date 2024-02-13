/*
* backlight_controller.c
*
* Created: 05.10.2023 21:54:49
* Author : Vanya
*/
#include "config.h"
#include <avr/io.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "string.h"
#include "stdbool.h"
#include "lib/uart_hal.h"
#include "lib/gpio_driver.h"
#include "lib/twi_hal.h"
#include "lib/adc_hal.h"
#include "lib/rtc.h"
#include "lib/tlc59_driver.h"


gpio rtc_int = {(uint8_t *)&PORTD, PORTD2};
gpio pir1_int = {(uint8_t *)&PORTD, PORTD3};
gpio pir2_int = {(uint8_t *)&PORTD, PORTD5};

gpio led_rst = {(uint8_t *)&PORTD, PORTD4};
gpio ld1 = {(uint8_t *)&PORTB , PORTB5};
gpio bat_ritical_led = {(uint8_t *)&PORTB , PORTB1};

#define		bat_critical_level		370			//when battery voltage less than this parameter "bat_ritical_led" blinking every rtc interrupt
#define		bat_critical_led_time	3			//"bat_critical_led" light time in milliseconds
#define		ALOVED_LIGHT_LEVEL		5
#define		HORKING_HOUR_START		18
#define		HORKING_HOUR_STOP		6

uint8_t rtc_int_request = 0;
uint16_t BAT_VOLT = 0;
uint16_t LIGHT_LEVEL = 0;
uint8_t pir1_active = 0;
uint8_t pir2_active = 0;

uint8_t active_timer = 0;
uint8_t action_request = 0;
uint8_t isActive = 0;

rtc_date sys_rtc = {
	.date = 11,
	.month = 5,
	.year = 24,
	.dayofweek = 5,
	.hour = 23,
	.minute = 33,
	.second = 0
};

uint8_t panel_addr = TLC_ADDR;
uint8_t pwmChannels[16];


ISR(INT0_vect){
	rtc_int_request=1;
}

ISR(INT1_vect){
	pir1_active=1;
}

ISR(PCINT2_vect){
	if(get_port_pin_level(&pir2_int)==true){
		pir2_active=1;
	}
}

int main(void)
{
	char char_array[128]="\0";
	uart_init(9600,1);
	twi_init(200000);
	
	EICRA |= (0b10 << ISC00) | (0b11 << ISC10); //INT0 falling edge interrupt request (RTC module); First PIR sensor rising edge of INT1 generates an interrupt request
	EIMSK = 0x03;								//0b00000001 Enable booth INT0 and INT1 interrupt request 
	
	//PCINT interrupt config
	PCICR |= (1 << 2);							//PCIE2 Pin Change Interrupt Enable 2
	PCMSK2 |= (1 << PCINT21);					//PCINT[23:16] Pin Change Enable Mask
	
	
	set_pin_dir(&ld1 , PORT_DIR_OUT); set_pin_level(&ld1, true);
	set_pin_dir(&led_rst , PORT_DIR_OUT); set_pin_level(&led_rst, false);
	set_pin_dir(&bat_ritical_led , PORT_DIR_OUT); set_pin_level(&bat_ritical_led, false);
	
	
	set_pin_dir(&rtc_int, PORT_DIR_IN);
	set_pin_dir(&pir1_int, PORT_DIR_IN);
	set_pin_dir(&pir2_int, PORT_DIR_IN);
	
	sei();
	//rtc_set(&sys_rtc);
	rtc_int_enable(&sys_rtc ,0);
	adc_init();
	
	//led_board_sw_reset(panel_addr);
	//Used HW reset port for maximum power save mode
	//Led module in the reset state when LEDs is off
	
	uart_send_string((uint8_t *)"RUN\n\r");
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	while (1)
	{
		if (pir1_active == 1){
			uart_send_string((uint8_t *)"INT1\n\r");
			if(isActive == 0 && LIGHT_LEVEL < ALOVED_LIGHT_LEVEL && (sys_rtc.hour >= HORKING_HOUR_START || sys_rtc.hour <= HORKING_HOUR_STOP)){
				action_request = ON_UP_DOWN;
			}
			pir1_active=0;
		}
		
		if (pir2_active == 1){
			//uart_send_string((uint8_t *)"INT2\n\r");
			if(isActive == 0 && LIGHT_LEVEL < ALOVED_LIGHT_LEVEL && (sys_rtc.hour >= HORKING_HOUR_START || sys_rtc.hour <= HORKING_HOUR_STOP)){
				action_request = ON_DOWN_UP;
			}
			pir2_active=0;
		}
		
		if(action_request != 0 && active_timer == 0){
			//uart_send_string((uint8_t *)"CASE\n\r");
			switch (action_request) {
				case ON_UP_DOWN:
				if(isActive == 0){
					isActive = 1;
					active_timer = 5;
					set_pin_level(&led_rst, true);
					led_board_init(panel_addr);
					run_wave(action_request);
					action_request = OFF_UP_DOWN;
				}
				break;
				case ON_DOWN_UP:
				if(isActive == 0){
					isActive = 1;
					active_timer = 5;
					set_pin_level(&led_rst, true);
					led_board_init(panel_addr);
					run_wave(action_request);
					action_request = OFF_DOWN_UP;
				}
				break;
				default:
				//uart_send_string((uint8_t *)"CASE\n\r");
				if(isActive != 0){
					isActive = 0;
					run_wave(action_request);
					set_pin_level(&led_rst, false);
					action_request = 0;
				}
				break;
			}
		}
		
		if (rtc_int_request != 0){
			rtc_sync(&sys_rtc);
			BAT_VOLT = get_mVolt(ADC4_PIN);
			LIGHT_LEVEL = get_mVolt(ADC5_PIN);
			sprintf(char_array, "%02d:%02d:%02d; bat:%umV; l=%u\r\n", sys_rtc.hour, sys_rtc.minute, sys_rtc.second, BAT_VOLT, LIGHT_LEVEL);
			//sprintf(char_array, "A: %02d; Req: %02d; Timer: %02d; Pir2: %02d;\n\r", isActive, action_request, active_timer, get_port_pin_level(&pir2_int));
			uart_send_string((uint8_t *)char_array);
			rtc_int_request = 0;
			//if (sys_rtc.hour >= HORKING_HOUR_START || sys_rtc.hour <= HORKING_HOUR_STOP)
			//{
				//uart_send_string((uint8_t *)"Working period\n\r");
			//}
			if(active_timer != 0){
				if(get_port_pin_level(&pir2_int) == true || get_port_pin_level(&pir1_int) == true){
					//uart_send_string((uint8_t *)"Pir sensors wait\n\r");
					}else{
					active_timer--;
					//uart_send_string((uint8_t *)"countdown\n\r");
				}
			}
		}
		
		if(BAT_VOLT < bat_critical_level){
			set_pin_level(&bat_ritical_led, true);
			_delay_ms(bat_critical_led_time);
			set_pin_level(&bat_ritical_led, false);
		}
		sleep_cpu();
	}
}

