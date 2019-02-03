?*files from today
start of pwm file and how to do it*/

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

     

#define CONF_PWM_MODULE      TCC0
#define CONF_PWM_CHANNEL     3 //both numbers have to be equal and 
#define CONF_PWM_OUTPUT      1
#define CONF_PWM_OUT_PIN     PIN_PA11F_TCC0_WO3
#define CONF_PWM_OUT_MUX     MUX_PA11F_TCC0_WO3

struct tcc_module tcc_instance;

static void configure_tcc0(void)
{
	struct tcc_config config_tcc;
	tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
	config_tcc.counter.clock_prescaler = TCC_CLOCK_PRESCALER_DIV1;
	config_tcc.counter.period = 0xFFFF;
	config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	config_tcc.compare.match[CONF_PWM_CHANNEL] = (0xFFFF / 2);
	config_tcc.pins.enable_wave_out_pin[CONF_PWM_OUTPUT] = true;
	config_tcc.pins.wave_out_pin[CONF_PWM_OUTPUT]        = CONF_PWM_OUT_PIN;
	config_tcc.pins.wave_out_pin_mux[CONF_PWM_OUTPUT]    = CONF_PWM_OUT_MUX;
	tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
	tcc_enable(&tcc_instance);
}

int main (void)
{
	system_init();
	configure_tcc0();
	while (1) {
		//infinite loop
	}
}
