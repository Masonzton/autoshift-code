//swclk = 45 and 4 on ice
//swdio =46 on sam and 2 on ice
//reset = 40  on sam and 10 on ice
//vtg =  44 or 43(core)  on sam and 1 on ice
//gnd = 5, 18, 35, 42   on  sam and 3 5 9 on ice (maybe not 9)


/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include "samc21g18a.h"
#include "tcc.h"
#include "system.h"
#include "delay.h"  //must call delay_init in main method

//figure out delay


#define CONF_PWM_MODULE      TCC0
#define CONF_PWM_CHANNEL     3 //this number is equal to W0=3 pin
#define CONF_PWM_OUTPUT      3 //I think its the same as the one above but don't really understand it
#define CONF_PWM_OUT_PIN     PIN_PA11F_TCC0_WO3
#define CONF_PWM_OUT_MUX     MUX_PA11F_TCC0_WO3

struct tcc_module tcc_instance;
struct tcc_config config_tcc;

/*
*configures PA09 and PA11 for pwm with
*PA09 duty cylcle of 1/d9  %
*PA11 duty cycle of 1/d11 %
* expected to be 2^N but probably still works with without
*/ 
static void configure_tcc0(int d11, int d9)
{
	tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
	config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV1;
	config_tcc.counter.period = 0xFFFF;
	config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	config_tcc.compare.match[CONF_PWM_CHANNEL] = (0xFFFF / d11);
	config_tcc.pins.enable_wave_out_pin[CONF_PWM_OUTPUT] = true;
	config_tcc.pins.wave_out_pin[CONF_PWM_OUTPUT]        = CONF_PWM_OUT_PIN;
	config_tcc.pins.wave_out_pin_mux[CONF_PWM_OUTPUT]    = CONF_PWM_OUT_MUX;
	config_tcc.compare.match[1]= (0xFFFF/d9);
	config_tcc.pins.enable_wave_out_pin[1] = true;
	config_tcc.pins.wave_out_pin[1] = PIN_PA09E_TCC0_WO1;
	config_tcc.pins.wave_out_pin_mux[1] = MUX_PA09E_TCC0_WO1; 
	tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
	tcc_enable(&tcc_instance);
	tcc_disable_double_buffering(&tcc_instance);
}

/*changes duty cycles of PA11 and PA09 without double buffer
* duty cycle of PA11 1/d11 %  
* duty cycle of PA09 = 1/d9 %
* d11 and d9 expected to be 2^N but probably still work with other cause automatically cast to int ?
*/
//TODO create function to work with any pins? maybe
static void change_pwm(int d11, int d9){
	tcc_set_compare_value(&tcc_instance,3,0xFFFF/d11);
	tcc_set_compare_value(&tcc_instance,1,0xFFFF/d9);
}

int main (void)
{
	system_init();
	configure_tcc0(32,1);
	delay_init();
	while (true) {
		//infinite loop
		delay_ms(500);
		change_pwm(1,32);
		delay_ms(500);
		change_pwm(32,1);
	}
}
