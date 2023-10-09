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
gpio led_rst = {(uint8_t *)&PORTD, PORTD4};

gpio ld1 = {(uint8_t *)&PORTB , PORTB5};


uint8_t rtc_int_request = 0;
uint16_t BAT_VOLT = 0;
uint16_t LIGHT_LEVEL = 0;
uint8_t pir1_active = 0;

uint8_t active_timer = 0;
uint8_t action_request = 0;
uint8_t isActive = 0;




rtc_date sys_rtc = {.date = 19,
	.month = 6,
	.year = 23,
	.dayofweek = 6,
	.hour = 19,
	.minute = 29,
	.second = 00
};


	uint8_t panel_addr = 0x64;

	
	
	
	
	//uint8_t brightness_on_du[] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 250, 100, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};
		
		
		
	///uint8_t brightness[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 100, 250, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};	

	uint8_t pwmChannels[16];
	
		
ISR(INT0_vect){
	rtc_int_request=1;
	//toggle_pin_level(&ld1);
}

ISR(INT1_vect){
	pir1_active=1;
	//toggle_pin_level(&ld1);
}

	
int main(void)
{
	char char_array[128]="\0";
	uart_init(9600,1);
	twi_init(400000);
	
	EICRA |= (0b10 << ISC00) | (0b11 << ISC10);
	EIMSK = 0x03; //0b00000001
	
	set_pin_dir(&ld1 , PORT_DIR_OUT); set_pin_level(&ld1, true);
	set_pin_dir(&led_rst , PORT_DIR_OUT); set_pin_level(&led_rst, false);
	
	
	set_pin_dir(&rtc_int, PORT_DIR_IN);
	set_pin_dir(&pir1_int, PORT_DIR_IN);
	
	
	sei();
	rtc_set(&sys_rtc);
	rtc_int_enable(&sys_rtc ,0);
	adc_init();
	

	//led_board_sw_reset(panel_addr);
	

	
	uart_send_string((uint8_t *)"RUN\n\r");
		sleep_enable();
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    while (1) 
    {
		
		
		if (pir1_active == 1){
			//uart_send_string((uint8_t *)"INT1\n\r");
			if(isActive == 0){
				action_request = ON_UP_DOWN;
			}
			
			pir1_active=0;
		}
		
		
		if(action_request != 0 && active_timer == 0){
			uart_send_string((uint8_t *)"CASE\n\r");
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
					uart_send_string((uint8_t *)"CASE\n\r");
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
			//sprintf(char_array, "%02d:%02d:%02d; bat:%umV; l=%u; init: %02X \r\n", sys_rtc.hour, sys_rtc.minute, sys_rtc.second, BAT_VOLT, LIGHT_LEVEL);
			
			sprintf(char_array, "A: %02d; Req: %02d; Timer: %02d;\n\r", isActive, action_request, active_timer);
			uart_send_string((uint8_t *)char_array);
			rtc_int_request = 0;
			
			
			//run_wave(ON_UP_DOWN);
			//_delay_ms(1000);
			//run_wave(OFF_UP_DOWN);
			//_delay_ms(1000);
			//run_wave(ON_DOWN_UP);
			//_delay_ms(1000);
			//run_wave(OFF_DOWN_UP);
			//_delay_ms(1000);
			
			//run_wave(brightness, pwmChannels, 1);
			
			if(active_timer != 0){
				active_timer--;
				uart_send_string((uint8_t *)"countdown");
			}
			
		}
		
		sleep_cpu();
		
		
    }
}

