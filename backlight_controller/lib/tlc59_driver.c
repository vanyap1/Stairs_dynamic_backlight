
/*
 * tlc59_driver.c
 *
 * Created: 05.10.2023 22:26:29
 *  Author: Vanya
 */ 

#include "adc_hal.h"
#include "twi_hal.h" 
#include "tlc59_driver.h"



void led_board_sw_reset(uint8_t board_address){
	uint8_t reset_cmd[] = {SWRST_byte_1, SWRST_byte_2};
	uint8_t init_led_pattern[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	twi_write_batch_by_reg(board_address, PWM0 | 0xA0, init_led_pattern, sizeof(init_led_pattern));
	twi_write_batch(board_address, reset_cmd, sizeof(reset_cmd));
	uint8_t reg = (1 << OSC);
	twi_write(board_address,MODE1, &reg,1);
	//twi_write(board_address,MODE2, &reg,1);
	//twi_write(SWRST,TIMER_COUNTER0_REG,SWRST,1);
}

uint8_t led_board_init(uint8_t board_address){
	uint8_t init_cmd[] = {0xAA, 0xAA, 0xAA, 0xAA};
	uint8_t init_led_pattern[16] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	
	uint8_t reg = 0;
	reg = 0;
	twi_write(board_address,MODE1, &reg,1);
	twi_write(board_address,MODE2, &reg,1);
	twi_write_batch_by_reg(board_address, PWM0 | 0xA0, init_led_pattern, sizeof(init_led_pattern));
	reg = 0x00;
	twi_write(board_address,GRPPWM,&reg,1);
	reg = 0xFF;
	twi_write(board_address,GRPFREQ,&reg,1);
	reg = (2 << LDR0) | (2 << LDR1) | (2 << LDR2) | (2 << LDR3);
	twi_write(board_address,LEDOUT0,&reg,1);
	reg = (2 << LDR4) | (2 << LDR5) | (2 << LDR6) | (2 << LDR7);
	twi_write(board_address,LEDOUT1,&reg,1);
	reg= (2 << LDR8) | (2 << LDR9) | (2 << LDR10) | (2 << LDR11);
	twi_write(board_address,LEDOUT2,&reg,1);
	reg = (2 << LDR12) | (2 << LDR13) | (2 << LDR14) | (2 << LDR15);
	twi_write(board_address,LEDOUT3,&reg,1);
	return 1;
	
}

void led_write_batch(uint8_t board_address, int *pwmChannels, uint8_t channels_num){
	twi_write_batch_by_reg(board_address, PWM0 | 0xA0, pwmChannels, channels_num);
}

void led_write_single(uint8_t board_address, uint8_t led_num, uint8_t brightness){
	
}

void run_wave(uint8_t *brightness, uint8_t *pwmChannels) {

				
//uint8_t temp[16];
uint8_t i;
uint8_t shifts;

for (shifts = 0; shifts < 16; shifts++) {
	for (i = 0; i < 16; i++) {
		pwmChannels[i] = brightness[i+shifts];
	}
	led_write_batch(TLC_ADDR, pwmChannels,16);
	_delay_ms(50);
}






}
