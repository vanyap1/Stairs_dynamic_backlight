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


gpio rtc_int = {(uint8_t *)&PORTD , PORTD2};
gpio ld1 = {(uint8_t *)&PORTB , PORTB5};

uint8_t rtc_int_request = 0;
uint16_t BAT_VOLT = 0;
uint16_t LIGHT_LEVEL = 0;


rtc_date sys_rtc = {.date = 19,
	.month = 6,
	.year = 23,
	.dayofweek = 6,
	.hour = 19,
	.minute = 29,
	.second = 00
};


uint8_t panel_addr = 0x64;

uint8_t reset_cmd[] = {0xa5, 0x5a};
//uint8_t init_cmd[] = {0x80, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xAA, 0xAA, 0xAA, 0xAA};
//uint8_t led_data[] = {0xa2, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f};
//uint8_t led1[]= {0xa2, 0, 0, 0};
	
ISR(INT0_vect){
	rtc_int_request=1;
	toggle_pin_level(&ld1);
}	
int main(void)
{
	char char_array[128]="\0";
	uart_init(9600,1);
	twi_init(400000);
	
	EICRA |= (0b10 << ISC00);
	EIMSK = 0x01; //0b00000001
	
	set_pin_dir(&ld1 , PORT_DIR_OUT); set_pin_level(&ld1, true);
	set_pin_dir(&rtc_int, PORT_DIR_IN);
	
	sei();
	rtc_set(&sys_rtc);
	rtc_int_enable(&sys_rtc ,0);
	adc_init();
	
	
    //twi_write_batch(panel_addr, reset_cmd, sizeof(reset_cmd));
    //_delay_ms(10);
    //led_board_init(panel_addr);
	//_delay_ms(10);
	//twi_write_batch(panel_addr, init_cmd, sizeof(init_cmd));
    
	//_delay_ms(10);
	//twi_write_batch(panel_addr, led_data, sizeof(led_data));
	led_board_sw_reset(panel_addr);
	led_board_init(panel_addr);

	
	uart_send_string((uint8_t *)"RUN\n\r");
    while (1) 
    {
		
		
		
		if (rtc_int_request != 0){
			rtc_sync(&sys_rtc);
			BAT_VOLT = get_mVolt(ADC4_PIN);
			LIGHT_LEVEL = get_mVolt(ADC5_PIN);			
			sprintf(char_array, "%02d:%02d:%02d; bat:%umV; l=%u; init: %02X \r\n", sys_rtc.hour, sys_rtc.minute, sys_rtc.second, BAT_VOLT, LIGHT_LEVEL);
			uart_send_string((uint8_t *)char_array);
			rtc_int_request = 0;
			
			
			
			//twi_write_batch(panel_addr, init_cmd, sizeof(init_cmd));
		}
		
		
		
		
    }
}

