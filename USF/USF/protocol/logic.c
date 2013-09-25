/*
 * logic.c
 *
 * Created: 7/10/2013 5:30:34 PM
 *  Author: kweekly
 */ 


#include "devicedefs.h"
#include "avrincludes.h"
#include "protocol/report.h"
#include "protocol/sensor_packet.h"
#include "protocol/datalink.h"
#include "device_headers.h"

#include "drivers/uart.h"
#include "drivers/i2cmaster.h"
#include "drivers/rtctimer.h"
#include "drivers/pcint.h"
#include "drivers/SPI.h"
#include "drivers/power.h"

#include "utils/scheduler.h"

#include "logic.h"


uint16_t requested_report_interval;
uint32_t last_periodic_report_sent;
uint8_t using_monitor_list;

void logic_init() {		
	datalink_set_rx_callback(&datalink_rx_callback);
	packet_set_handlers(&cmd_timesync_cb, &cmd_configure_sensor_cb, &cmd_actuate_cb);
}

uint16_t report_interval_needed() {
	if ( using_monitor_list ) {
		return 1;
	} else {
		return requested_report_interval;
	}	
}

void datalink_rx_callback(uint8_t * data, uint16_t len) {
	packet_recieved(data,len);
}

void cmd_configure_sensor_cb(uint8_t mode, uint16_t fields_to_report, uint16_t sample_interval) {
	printf_P(PSTR("Configuring fields %02X => %02X, sample interval %ds\n"),report_fields_requested,fields_to_report,sample_interval);
	report_fields_requested = fields_to_report;
	requested_report_interval = sample_interval;
	last_periodic_report_sent = 0;
	using_monitor_list = 0;
	
	scheduler_reset(); //removes all tasks
	scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,MOTE_TASK_ID, 0, &datalink_wake);
	//scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,LED_BLIP_TASK_ID, 0, &task_led_blip_on);
	//scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,LED_BLIP_TASK_ID, 100, &task_led_blip_off);
	scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,TASK_REPORTING, 0, &task_begin_report);

	scheduler_add_task(SCHEDULER_MONITOR_LIST,TASK_REPORTING,0, &task_begin_report);
	
	#ifdef REPORT_TYPE_TEMP	
		if ( fields_to_report & (REPORT_TYPE_TEMP) )
			humid_setup_reporting_schedule(1);
		else 
			humid_sleep();
	#endif
	#ifdef REPORT_TYPE_OCCUPANCY
		if ( fields_to_report & (REPORT_TYPE_OCCUPANCY) ) {
			pir_wake(); // PIR must stay always-on to be effective
			pir_setup_reporting_schedule(1);
		}			
		else 
			if ( !(fields_to_report & (REPORT_TYPE_OCCUPANCY_CHANGED)) ) pir_sleep();
		
	#endif
	#ifdef REPORT_TYPE_LIGHT
		if ( fields_to_report & (REPORT_TYPE_LIGHT) )
			light_setup_reporting_schedule(1);
		else 
			if ( !(fields_to_report & (REPORT_TYPE_LIGHT_CHANGED)) )light_sleep();
	#endif
	#ifdef REPORT_TYPE_ACCEL
		if ( fields_to_report & (REPORT_TYPE_ACCEL) )
			accel_setup_reporting_schedule(5);
		else
			if ( !(fields_to_report & (REPORT_TYPE_ORIENTATION_CHANGED)) ) accel_sleep();
	#endif
	#ifdef REPORT_TYPE_GYRO
		if ( fields_to_report & (REPORT_TYPE_GYRO) )
			gyro_setup_reporting_schedule(1);
		else 
			gyro_sleep();
	#endif
	#ifdef REPORT_TYPE_OCCUPANCY_CHANGED
		if ( fields_to_report & (REPORT_TYPE_OCCUPANCY_CHANGED) ) {
			pir_setup_interrupt_schedule(0);
			using_monitor_list = 1;
		} else  {
			if ( !(fields_to_report & (REPORT_TYPE_OCCUPANCY)) ) pir_sleep();
		}		
	#endif
	#ifdef REPORT_TYPE_ORIENTATION_CHANGED
		if ( fields_to_report & (REPORT_TYPE_ORIENTATION_CHANGED) ) {
			accel_setup_interrupt_schedule(0);
			using_monitor_list = 1;
		} else {
			accel_configure_orientation_detection(ACCEL_ORIENTATION_ALL, NULL);
			if ( !(fields_to_report & (REPORT_TYPE_ACCEL ) )) accel_sleep();
		}		
	#endif
	#ifdef REPORT_TYPE_LIGHT_CHANGED
		if ( fields_to_report & (REPORT_TYPE_LIGHT_CHANGED) ) {
			light_setup_interrupt_schedule(0);
			using_monitor_list = 1;
		} else {
			if ( !(fields_to_report & (REPORT_TYPE_LIGHT) ) ) light_sleep();
		}
	#endif	
	#ifdef USE_DOOR_SENSORS
		if ( fields_to_report & (REPORT_TYPE_DOOR_SENSORS))
		door_sensors_setup_interrupt_schedule(1);
		else {
			scheduler_remove_tasks(SCHEDULER_MONITOR_LIST,DOOR_SENSOR_TASK_ID);
		}
	#endif
	
	#ifdef REPORT_TYPE_POWER_CH0 // should cover all the other channels
		if ( fields_to_report & (REPORT_TYPE_POWER_CH0) )
			powermon_setup_reporting_schedule(1);
		else {
			scheduler_remove_tasks(SCHEDULER_PERIODIC_SAMPLE_LIST, POWERMON_TASK_ID);
		}
	#endif
		
	#ifdef REPORT_TYPE_RSSI 
		if ( fields_to_report & (REPORT_TYPE_RSSI) )
			datalink_setup_rssi_reporting(1);
		else {
			scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST, MOTE_TASK_ID, 0, &datalink_wake);	
		}
	#endif
	
	#ifdef USE_PN532
		rfid_setup_interrupt_schedule(1, &rfid_detection_cb);
		using_monitor_list = 1;
	#endif
	
	#ifdef USE_MACHXO2_PMCO2
		machxo2_setup_reporting_schedule(1);
	#endif
	

	scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,TASK_REPORTING, SCHEDULER_LAST_EVENTS, &task_print_report);
	scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,TASK_REPORTING, SCHEDULER_LAST_EVENTS, &task_send_report);
	scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,TASK_REPORTING, SCHEDULER_LAST_EVENTS, &report_poplast);
	scheduler_add_task(SCHEDULER_PERIODIC_SAMPLE_LIST,MOTE_TASK_ID, SCHEDULER_LAST_EVENTS, &datalink_sleep);
	
	scheduler_add_task(SCHEDULER_MONITOR_LIST,TASK_REPORTING, SCHEDULER_LAST_EVENTS, &task_print_report);
	scheduler_add_task(SCHEDULER_MONITOR_LIST,TASK_REPORTING, SCHEDULER_LAST_EVENTS, &task_check_send_report);
	scheduler_add_task(SCHEDULER_MONITOR_LIST,TASK_REPORTING, SCHEDULER_LAST_EVENTS, &report_poplast);
}


void rtc_timer_cb(void) {
	uint32_t curtime = rtctimer_read();
	if ( curtime < last_periodic_report_sent || curtime - last_periodic_report_sent  >= requested_report_interval ) {
		scheduler_start(SCHEDULER_PERIODIC_SAMPLE_LIST);
		last_periodic_report_sent = curtime;
	}
	if ( using_monitor_list ) {
		scheduler_start(SCHEDULER_MONITOR_LIST);
	}
}

void task_begin_report(void) {
	report_new(rtctimer_read());
}


void task_print_report(void) {
	if ( report_current()->fields != 0)
		report_print_human(report_current());
	//report_print(report_current());
}

void task_check_send_report(void) {
	if ( report_current()->fields != 0) {
		uint8_t packetbuf[128];
		datalink_wake();		
		uint16_t len = _construct_report_packet(packetbuf);
		datalink_send_packet_to_host(packetbuf, len);
		datalink_wake();
	}
}


void task_send_report(void) {
	uint8_t packetbuf[128];
	uint16_t len = _construct_report_packet(packetbuf);
	datalink_send_packet_to_host(packetbuf, len);
}

uint16_t _construct_report_packet(uint8_t * buf) {
	uint16_t len = 0;
	report_t * report = report_current();
	len += packet_construct_sensor_data_header(report->time,report->fields,buf);
	len += report_populate_real_data(report,buf + len);
	return len;
}



void cmd_timesync_cb(uint32_t new_time) {
	printf_P(PSTR("Time is now: %ld\n"),new_time);
	rtctimer_write(new_time);
	datalink_latch_destination_address();
}


void cmd_actuate_cb(uint16_t fields_to_affect, uint8_t * actuation_data) {
	
}
#ifdef USE_PN532
void rfid_detection_cb(uint8_t * uid, uint8_t uidlen) {
	uint8_t buf[128];
	datalink_wake();
	printf_P(PSTR("t=%10ld UID: "),rtctimer_read());
	for(int c =0; c < uidlen; c++)
		printf_P(PSTR("%02X"),uid[c]);
	kputs("\n");
	
	// Beeper trigger
	spi_pause();
	EXP_CSN = 0;
	EXP_MOSI = 0;
	_delay_us(250);
	EXP_CSN = 1;
	EXP_MOSI = 1;
	spi_resume();
	
	uint16_t pos = packet_construct_RFID_detected_header(rtctimer_read(), buf);
	memcpy(buf+pos,uid,uidlen);
	datalink_send_packet_to_host(buf,pos+uidlen);
	datalink_sleep();
}
#endif

#ifdef USE_AMN41121
void pir_detection_cb(uint8_t occupied) {
	
}
#endif

#ifdef USE_TSL2560
void light_detection_cb(uint8_t level) {
	printf_P(PSTR("Light level is now %d\n"),level);
}
#endif


#ifdef USE_LIS3DH
void accel_click_cb(unsigned char detect_mask) {
	
}

void accel_orientation_cb(unsigned char orientation) {
	
	
}
#endif


void task_led_blip_on(void) {
	LED1 = 1;
}

void task_led_blip_off(void) {
	LED1 = 0;
}