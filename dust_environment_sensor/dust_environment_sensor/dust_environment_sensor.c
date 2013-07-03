/*
 * dust_environment_sensor.c
 *
 * Created: 3/12/2013 3:59:40 PM
 *  Author: kweekly
 
 FUSE SETTINGS:
 BODLEVEL = 2V7
 OCDEN = [ ]
 JTAGEN = [X]
 SPIEN = [X]
 WDTON = [ ]
 EESAVE = [ ]
 BOOTSZ = 4096W_F000
 BOOTRST = [ ]
 CKDIV8 = [ ]
 CKOUT = [ ]
 SUT_CKSEL = INTRCOSC_6CK_0MS

 EXTENDED = 0xFD (valid)
 HIGH = 0x99 (valid)
 LOW = 0xC2 (valid)

 
 */ 

#undef PN532_ATTACHED

#include "all.h"
#ifdef PN532_ATTACHED
#include "devices/PN532.h"
#endif

static FILE mystdout = FDEV_SETUP_STREAM(uart_putc, NULL, _FDEV_SETUP_WRITE);


void board_power_down_devices(void);
void avr_doze(void);
void avr_sleep(void);

void task_led_blip_on(void);
void task_led_blip_off(void);

#define TASK_REPORTING 0x02
void task_begin_report(void);
void task_print_report(void);
void task_send_report(void);

void test_click(unsigned char src);
void test_orient(uint8_t orient);

uint8_t rx_packet_buf[128];
void status_changed_cb(uint8_t status);
void rx_cb(xbee_16b_address addr_16b, xbee_64b_address addr_64b, uint8_t rssi, uint16_t nBytes);
void tx_cb(uint8_t frame_id, uint8_t status);

xbee_64b_address dest64;
xbee_16b_address dest16;
xbee_ipv4_address destipv4;
enum{
	NO_SEND,
	BROADCAST,
	SEND64,
	SEND16,
	WIFI_SEND
} dest_mode;


int main(void)
{
	uint32_t versiondata;
	char wdrst = 0;
	if ( MCUSR & _BV(WDRF) ) {
		wdrst = 1;
		MCUSR = 0;
	}	
	wdt_disable();
	
	cli();	
	LED1 = 1;
	uart_init(UART_BAUD_SELECT_DOUBLE_SPEED(115200,F_CPU));
	stdout = &mystdout;
	sei();
	
	if ( wdrst ) {
		kputs("\n***WATCHDOG RESET***\n");
	}
	
	wdt_enable(WDTO_8S);
	wdt_reset();
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
	
	#ifdef PN532_ATTACHED
	kputs("Initializing RFID reader... \n");
	rfid_init();
	versiondata = rfid_get_firmware_version();
	if ( !versiondata ) {
		versiondata = rfid_get_firmware_version(); // 1 retry	
	}
	if ( !versiondata ) {
		kputs("\tDidn't find board\n");
	} else {
		printf_P(PSTR("\tFound chip PN5%02X\n"),(uint8_t)(versiondata>>24));
		printf_P(PSTR("\tFirmware Ver. %d.%d\n"),(uint8_t)(versiondata>>16),(uint8_t)(versiondata>>8));
	}
	if (!rfid_SAMConfig()) {
		kputs("\tError putting in SAM mode.\n");
	}
	#endif
	
	kputs("Initializing wireless mote\n");
	dest_mode = BROADCAST;
	xbee_init();
	xbee_set_modem_status_callback(&status_changed_cb);
	xbee_set_tx_status_callback(&tx_cb);
	xbee_set_rx_callback(&rx_cb,rx_packet_buf);
	if (xbee_get_type() == XBEE_TYPE_WIFI) {
		dest_mode = WIFI_SEND;
	}
	
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
	xbee_setup_reporting_schedule(1); // also tells xbee to wake-up
	humid_setup_reporting_schedule(1);
	light_setup_reporting_schedule(1);
	pir_setup_reporting_schedule(1);
	accel_setup_reporting_schedule(5);
	gyro_setup_reporting_schedule(1);
	scheduler_add_task(TASK_REPORTING, SCHEDULER_LAST_EVENTS, &task_print_report);
	scheduler_add_task(TASK_REPORTING, SCHEDULER_LAST_EVENTS, &task_send_report);
	scheduler_add_task(TASK_REPORTING, SCHEDULER_LAST_EVENTS, &report_poplast);
	scheduler_add_task(MOTE_TASK_ID, SCHEDULER_LAST_EVENTS, &xbee_sleep);
	
	kputs("Starting RTC clock\n");
	rtctimer_init();
	rtctimer_set_periodic_alarm(2,&scheduler_start);
	
    while(1)
    {
		wdt_enable(WDTO_250MS);
		wdt_reset();
		pcint_check();
		rtctimer_check_alarm();
		wdt_disable();
		wdt_enable(WDTO_1S);
		wdt_reset();
		#ifdef PN532_ATTACHED
		if(rfid_passive_scan()) {
			
		}
		xbee_tick();
		_delay_ms(100);
		#else
		avr_sleep();
		#endif
		wdt_disable();
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
}

void task_send_report(void) {
	uint8_t packetbuf[128];
	int8_t resp;
	xbee_tick();
	
	redo_send:
	if (xbee_get_status() == XBEE_STATUS_ASSOC) {
		uint16_t len = report_populate_real(report_current(),packetbuf);
		if (dest_mode == BROADCAST) {
			xbee_send_packet_64(XBEE_BROADCAST_64b_ADDR,len,packetbuf,XBEE_TX_OPTION_BROADCAST_PAN);
		} else if ( dest_mode == SEND16 ){
			xbee_send_packet_16(dest16,len,packetbuf,0);
		} else if (dest_mode == SEND64) {
			xbee_send_packet_64(dest64,len,packetbuf,0);
		} else if (dest_mode == WIFI_SEND) {
			uint8_t ipbuf[4];
			resp = xbee_AT_get("MY",ipbuf);
			if ( resp < 0 ) {
				kputs("Radio not responding");
				return;
			}
			printf_P(PSTR("My IP address=%d.%d.%d.%d\n"),ipbuf[0],ipbuf[1],ipbuf[2],ipbuf[3]);
			destipv4 = (ipbuf[0]<<24UL) | (ipbuf[1]<<16UL) | (ipbuf[2]<<8UL) | 1;
			xbee_send_ipv4_packet(destipv4, len, packetbuf);
		}
	} else {
		uint8_t buf[1];
		resp = xbee_AT_get("AI",buf);
		if ( resp < 0 ) {
			kputs("Radio not responding.");
			return;
		}
		if (buf[0] == 0) {
			printf_P(PSTR("Radio ready, but did not send status update, correcting.\n"));
			xbee_set_status(XBEE_STATUS_ASSOC);
			goto redo_send;
		}
		else printf_P(PSTR("Radio not ready to send packet AI=%02X\n"),buf[0]);
	}
	xbee_tick();
}

void status_changed_cb(uint8_t status) {
	printf_P(PSTR("Modem status is now %d\n"),status);
}
void rx_cb(xbee_16b_address addr_16b, xbee_64b_address addr_64b, uint8_t rssi, uint16_t nBytes) {
	/*
	printf_P(PSTR("Packet received src16:%04X src64:"),addr_16b);
	for ( int c = 0; c < 8; c++) {
		printf("%02X",((char*)&addr_64b)[8-c-1]);
	}
	printf_P(PSTR(" rssi:-%ddBm data:"),rssi);
	for ( int c = 0; c < nBytes; c++)
		printf("%02X",rx_packet_buf[c]);
	printf("\n");
	*/
	if ( nBytes == 0 ) return;
	
	uint32_t * timeptr;
	switch(rx_packet_buf[0]){
		case 0x01:
			timeptr = (uint32_t *)(&(rx_packet_buf[1]));
			printf_P(PSTR("Time is now: %ld\n"),*timeptr);
			rtctimer_write(*timeptr);
			if ( addr_16b != XBEE_UNKNOWN_16b_ADDR ) {
				dest_mode = SEND16;
				dest16 = addr_16b;
			} else if ( addr_64b != XBEE_UNKNOWN_64b_ADDR ) {
				dest_mode = SEND64;
				dest64 = addr_64b;
			}
			break;
		default:
			printf_P(PSTR("Unknown remote command %02X\n"),rx_packet_buf[0]);
	}
}
void tx_cb(uint8_t frame_id, uint8_t status){
	//printf_P(PSTR("Last TX message had error code %d\n"),status);
}

void board_power_down_devices(void) {
	LED1 = 0;
	LED2 = 0;
	
	light_sleep();
	gyro_sleep();
	accel_sleep();
	humid_sleep();
	pir_sleep();
	xbee_sleep();
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
	xbee_tick();
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