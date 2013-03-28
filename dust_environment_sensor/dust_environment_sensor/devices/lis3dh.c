/*
 * lis3dh.c
 *
 * Created: 3/12/2013 5:48:08 PM
 *  Author: kweekly
 */ 

#include "all.h"


#define STATUS_REG_AUX 0x07
#define OUT_ADC1_L		0x08
#define OUT_ADC1_H		0x09
#define OUT_ADC2_L		0x0A
#define OUT_ADC2_H		0x0B
#define OUT_ADC3_L		0x0C
#define OUT_ADC3_H		0x0D
#define INT_COUNTER_REG	0x0E
#define WHO_AM_I		0x0F
#define TEMP_CFG_REG	0x1F
#define CTRL_REG1		0x20
#define CTRL_REG2		0x21
#define CTRL_REG3		0x22
#define CTRL_REG4		0x23
#define CTRL_REG5		0x24
#define CTRL_REG6		0x25
#define REFERENCE		0x26
#define STATUS_REG		0x27
#define OUT_X_L			0x28
#define OUT_X_H			0x29
#define OUT_Y_L			0x2A
#define OUT_Y_H			0x2B
#define OUT_Z_L			0x2C
#define OUT_Z_H			0x2D
#define FIFO_CTRL_REG	0x2E
#define FIFO_SRC_REG	0x2F
#define INT1_CFG		0x30
#define INT1_SOURCE		0x31
#define INT1_THS		0x32
#define INT1_DURATION	0x33
#define CLICK_CFG		0x38
#define CLICK_SRC		0x39
#define CLICK_THS		0x3A
#define TIME_LIMIT		0x3B
#define TIME_LATENCY	0x3C
#define TIME_WINDOW		0x3D

#define READ_MULTIPLE 0x80

void (*click_cb)(void);
void (*orient_cb)(unsigned char orientation);


void accel_init(void) {
	unsigned char b;
	
	accel_wake();
	
	b = 0b00000100; // HPF freq is 0.2Hz, enable HPF for click, but not data or AOI function
	i2c_writereg(ACCEL_ADDR, CTRL_REG2, 1, &b);
	
	b = 0b00000000; // no interrupts ( for now ?)
	i2c_writereg(ACCEL_ADDR, CTRL_REG3, 1, &b);
	
	b = 0b10000000; // no self test, block data update, +/- 2G scale
	i2c_writereg(ACCEL_ADDR, CTRL_REG4, 1, &b);
	
	b = 0b00000000; // no 4D detection, no FIFO
	i2c_writereg(ACCEL_ADDR, CTRL_REG5, 1, &b);
	
	b = 0b00000000; 
	i2c_writereg(ACCEL_ADDR, CTRL_REG6, 1, &b);
	
	click_cb = orient_cb = NULL;
}

void accel_wake(void) {
	unsigned char b = 0x2F; // low power mode, 10Hz
	if ( i2c_writereg(ACCEL_ADDR, CTRL_REG1, 1, &b ) ) {
		kputs("Error turning on accel\n");
	}	
}

void accel_sleep(void) {
	
	unsigned char b = 0b00001000;
	if ( i2c_writereg(ACCEL_ADDR, CTRL_REG1, 1, &b ) ) {
		kputs("Error turning off accel\n");
	}
	
}

uint8_t accel_read_status(void) {
	unsigned char b;
	if ( i2c_readreg(ACCEL_ADDR, STATUS_REG, 1, &b) ) {
		kputs("Error reading status reg\n");
	}
	return b;
}

accel_reading_t accel_read(void) {
	accel_reading_t retval;
	unsigned char buf[6];
	
	while ( (accel_read_status() & _BV(3)) == 0) {
		kputs("Error: Accel not ready\n");
	}
	
	if ( i2c_readreg(ACCEL_ADDR, READ_MULTIPLE | OUT_X_L, 6, buf ) ) {
		kputs("Error reading from accel sensor\n");
	}
	retval.X = *(int16_t *)(buf);
	retval.Y = *(int16_t *)(buf+2);
	retval.Z = *(int16_t *)(buf+4);
	
	return retval;	
	
}

// Uses INT1 of accel -> PORTA.0 / PCINT0
void accel_configure_click( void (*c)(void) ) {
	unsigned char b = 0b1000000;
	if ( i2c_writereg(ACCEL_ADDR, CTRL_REG3, 1, &b ) ) {
		kputs("Error configuring accel sensor\n");
	}
	
	click_cb = c;
	
	// setup PCINT0
	pcint_register(0,&_accel_int1_cb);
}

void _accel_int1_cb() {
	if ( ACCEL_INT1_PIN && click_cb ) {
		click_cb();
	}
}


// uses INT2 -> PORTA.1 / PCINT1
void accel_configure_orientation_detection( unsigned char detection_mask, void (*o)(unsigned char orientation) ) {
	unsigned char b = 0b01000000; // active high, AOI interrupt enable
	if ( i2c_writereg(ACCEL_ADDR, CTRL_REG6, 1, &b ) ) {
		kputs("Error configuring accel sensor\n");
	}
	
	b = 0b11000000 | detection_mask; // enable 6D motion detection
	i2c_writereg(ACCEL_ADDR, INT1_CFG, 1, &b );
	
	b = 50; // Approx 0.8g
	i2c_writereg(ACCEL_ADDR, INT1_THS, 1, &b );
	
	orient_cb = o;
	
	// setup PCINT1
	pcint_register(1,&_accel_int2_cb);
}

void _accel_int2_cb() {
	unsigned char b;
	if ( ACCEL_INT2_PIN && orient_cb ) {
		if ( i2c_readreg(ACCEL_ADDR, INT1_SOURCE, 1, &b) ) {
			kputs("Error reading accel interrupt source\n");
			return;
		}
		orient_cb(b&0x3F);
	}
}

void accel_setup_reporting_schedule(uint16_t starttime) {
	scheduler_add_task(ACCEL_TASK_ID,starttime,&_accel_reporting_doread);
}

void _accel_reporting_doread() {
	report_current()->accel = accel_read();
	report_current()->fields |= REPORT_TYPE_ACCEL;
}

#define ACCEL_SENSITIVITY 2.0

void accel_fmt_reading(accel_reading_t * reading, uint8_t maxlen, char * str) {
	float ax = reading->X / (float)(1<<15) * ACCEL_SENSITIVITY;
	float ay = reading->Y / (float)(1<<15) * ACCEL_SENSITIVITY;
	float az = reading->Z / (float)(1<<15) * ACCEL_SENSITIVITY;
	snprintf_P(str,maxlen,PSTR("ax=%5.2fg ay=%5.2fg az=%5.2fg"),ax,ay,az);
}