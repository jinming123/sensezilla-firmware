/*
 * dust_environment_sensor.c
 *
 * Created: 3/12/2013 3:59:40 PM
 *  Author: kweekly
 */ 

#include "all.h"

static FILE mystdout = FDEV_SETUP_STREAM(uart_putc, NULL, _FDEV_SETUP_WRITE);


void board_power_down_devices(void);
void avr_doze(void);
void avr_sleep(void);

void task_led_blip_on(void);
void task_led_blip_off(void);

#define TASK_REPORTING 0x02
void task_begin_report(void);
void task_print_report(void);

void test_click(unsigned char src);
void test_orient(uint8_t orient);

int main(void)
{
	cli();	
	LED1 = 1;
	uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(115200,F_CPU));
	stdout = &mystdout;
	sei();
	
	kputs("Setting DDR registers\n");
	DDRA = DDRA_SETTING;
	DDRB = DDRB_SETTING;
	DDRC = DDRC_SETTING;
	DDRD = DDRD_SETTING;
	DIDR0 = 0;
	DIDR1 = 0;
	
	kputs("Initializing PCINT\n");
	pcint_init();
	
	kputs("Initializing i2c\n");
	i2c_init();
	
	kputs("Initializing Reports\n");
	report_init();
		

	kputs("Initializing Devices... Light,");
	light_init();
	kputs("Gyro, ");
	gyro_init();
	kputs("Accell, ");
	accel_init();
	kputs("Humid, ");
	humid_init();
	kputs("PIR\n");
	pir_init();
	
	/*
	PIR_VCC = 1;
	while(1) {
		if(PIR_OUT_PIN) {
			kputs("ON\n");
		} else {
			kputs("OFF\n");
		}
		_delay_ms(50);
	}*/
	
	kputs("Powering down all devices\n");
	board_power_down_devices();
	
	kputs("Turning on always-on devices\n");
	pir_wake();
	accel_wake();
	
	kputs("Initialize interrupts\n");
	accel_configure_click(&test_click);
	accel_configure_orientation_detection(ACCEL_ORIENTATION_ALL,&test_orient);
	
	kputs("Initializing Scheduler and tasks\n");
	scheduler_init();
	scheduler_add_task(LED_BLIP_TASK_ID, 0, &task_led_blip_on);
	scheduler_add_task(LED_BLIP_TASK_ID, 10, &task_led_blip_off);
	
	scheduler_add_task(TASK_REPORTING, 0, &task_begin_report);
	humid_setup_reporting_schedule(1);
	light_setup_reporting_schedule(1);
	pir_setup_reporting_schedule(1);
	accel_setup_reporting_schedule(5);
	gyro_setup_reporting_schedule(1);
	scheduler_add_task(TASK_REPORTING, SCHEDULER_LAST_EVENTS, &task_print_report);
	
	kputs("Starting RTC clock\n");
	rtctimer_init();
	rtctimer_set_periodic_alarm(1,&scheduler_start);
	
    while(1)
    {
		pcint_check();
		rtctimer_check_alarm();
		avr_sleep();
		i2c_init(); // it appears we need to reset the i2c whenever we wake up
    }
}

void task_led_blip_on(void) {
	LED1 = 1;
}

void task_led_blip_off(void) {
	LED1 = 0;
}

void task_begin_report(void) {
	report_new(rtctimer_read());	
}

void task_print_report(void) {
	report_print_human(report_current());
	report_poplast();
}

void board_power_down_devices(void) {
	LED1 = 0;
	LED2 = 0;
	
	light_sleep();
	gyro_sleep();
	accel_sleep();
	humid_sleep();
	pir_sleep();
}


void avr_doze(void) {
	// go into idle mode
	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();
	sleep_cpu();
	sleep_disable();
}	

void avr_sleep(void) {
	uart_flush();
	uart1_flush();
	// go into sleep (only external interrupt or timer can wake us up)
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	sleep_enable();
	sleep_cpu();
	sleep_disable();
}

void test_click(unsigned char src) {
	kputs("Click ");
	switch(src & 0x07) {
		case 1:
			kputs("X\n");
			break;
		case 2:
			kputs("Y\n");
			break;
		case 4:
			kputs("Z\n");
			break;
		default:
			kputs("UKNOWN\n");
			break;
	}
}

void test_orient(uint8_t orient) {
	kputs("Orientation is now ");
	switch(orient) {
		case ACCEL_ORIENTATION_ZUP:
			kputs("ZUP\n");
			break;
		case ACCEL_ORIENTATION_ZDOWN:
			kputs("ZDOWN\n");
			break;
		case ACCEL_ORIENTATION_XUP:
			kputs("XUP\n");
			break;
		case ACCEL_ORIENTATION_XDOWN:
			kputs("XDOWN\n");
			break;
		case ACCEL_ORIENTATION_YUP:
			kputs("YUP\n");
			break;
		case ACCEL_ORIENTATION_YDOWN:
			kputs("YDOWN\n");
			break;
		default:
			kputs("UNKNOWN\n");
	}
	
}