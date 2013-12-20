/*
 * devicedefs.h
 *
 * Created: 3/12/2013 4:27:35 PM
 *  Author: kweekly 
 */ 
#ifndef DEVICEDEFS_H_
#define DEVICEDEFS_H_
#define MOTE_NONE 0
#define MOTE_XBEE 1
#define MOTE_WIFLY 2


#include "configurations.h"

#if !(MOTE_TYPE == MOTE_XBEE || MOTE_TYPE == MOTE_WIFLY || MOTE_TYPE == MOTE_NONE)
#error "Unknown MOTE_TYPE"
#endif


// Power mode
#if defined(ENVIRONMENT_SENSOR) && !defined(USE_PN532) && !defined(USE_MACHXO2_PMCO2) && !defined(USE_K20)
#define LOW_POWER
#else
#define HIGH_POWER
#endif

// FCPU
#if defined ENVIRONMENT_SENSOR
#define F_CPU 8000000L
#elif defined POWER_STRIP_MONITOR
#define F_CPU 16384000L
#endif


// DDRs
#if defined ENVIRONMENT_SENSOR
	#if HW_VERSION==1
		#define DDRA_SETTING	0b10111100
		#define DDRB_SETTING	0b10110010
	#elif HW_VERSION==2
		#define DDRA_SETTING	0b10111100
		#if defined(USE_DOOR_SENSORS)
			#if defined(USE_TOUCH_SENSORS)
				#define DDRB_SETTING	0b00010000
			#else
				#define DDRB_SETTING	0b00110000
			#endif
		#else
			#define DDRB_SETTING	0b10110000
		#endif
	#elif HW_VERSION==3
		#define DDRA_SETTING 0b10110000
		#if defined(USE_DOOR_SENSORS)
			#if defined(USE_TOUCH_SENSORS)
				#define DDRB_SETTING	0b00010010
			#else
				#define DDRB_SETTING	0b00110010
			#endif
		#else
			#define DDRB_SETTING	0b10110010
		#endif
	#endif
	
	#define DDRC_SETTING	0b00000000 // hopefully automatically initialized by i2c port
	#define DDRD_SETTING	0b01111010
#elif defined POWER_STRIP_MONITOR 
	#define DDRA_SETTING	0b11111111
	#define DDRB_SETTING	0b10111111
	#define DDRC_SETTING	0b11000010
	#define DDRD_SETTING	0b11101010
#endif


typedef struct
{
	unsigned int bit0:1;
	unsigned int bit1:1;
	unsigned int bit2:1;
	unsigned int bit3:1;
	unsigned int bit4:1;
	unsigned int bit5:1;
	unsigned int bit6:1;
	unsigned int bit7:1;
} _io_reg;

#define REGISTER_BIT(rg,bt) ((volatile _io_reg*)&rg)->bit##bt


// Expansion
#define EXP_SCK		 REGISTER_BIT(PORTB,7)
#define EXP_MISO	 REGISTER_BIT(PORTB,6)
#define EXP_MISO_PIN REGISTER_BIT(PINB,6)
#define EXP_MISO_PCINT 14
#define EXP_MOSI	 REGISTER_BIT(PORTB,5)
#define EXP_CSN		 REGISTER_BIT(PORTB,4)

#define EXP_SCK_DDR		REGISTER_BIT(DDRB,7)
#define EXP_MISO_DDR	REGISTER_BIT(DDRB,6)
#define EXP_MOSI_DDR	REGISTER_BIT(DDRB,5)
#define EXP_CSN_DDR		REGISTER_BIT(DDRB,4)

// Mote
#if defined(ENVIRONMENT_SENSOR)
	#define MOTE_TASK_ID	0x05
	#define MOTE_SLEEP		REGISTER_BIT(PORTA,5)
	#if HW_VERSION==1
		#define MOTE_TIMEN		REGISTER_BIT(PORTA,3)
		#define MOTE_TX_RTSN	REGISTER_BIT(PORTB,2)
		#define MOTE_RX_CTSN	REGISTER_BIT(PORTB,0)
		#define MOTE_TX_CTSN	REGISTER_BIT(PORTB,1)
		#define MOTE_RESETN		EXP_CSN  // workaround
	#elif HW_VERSION==2
		#define MOTE_ASSOCIATED		REGISTER_BIT(PORTB,2)
		#define MOTE_ASSOCIATED_PIN REGISTER_BIT(PINB,2)
		#define MOTE_ASSOCIATED_PCINT 10
		#define MOTE_CONNECTED		REGISTER_BIT(PORTB,1)
		#define MOTE_CONNECTED_PCINT 9
		#define MOTE_CONNECTED_PIN	REGISTER_BIT(PINB,1)
		#define MOTE_RESETN		REGISTER_BIT(PORTA,4)	
	#elif HW_VERSION==3
		#define MOTE_ASSOCIATED		REGISTER_BIT(PORTB,3)
		#define MOTE_ASSOCIATED_PIN REGISTER_BIT(PINB,3)
		#define MOTE_ASSOCIATED_PCINT 11
		#define MOTE_CONNECTED		REGISTER_BIT(PORTB,2)
		#define MOTE_CONNECTED_PIN	REGISTER_BIT(PINB,2)
		#define MOTE_CONNECTED_PCINT 10
		#define MOTE_RESETN		REGISTER_BIT(PORTA,4)
		#define MOTE_SLEEP		REGISTER_BIT(PORTA,5)
		#define MOTE_CONNECT	REGISTER_BIT(PORTB,1)
	#endif
	
	#define MOTE_TX_CTSN		REGISTER_BIT(PORTB,0)
	#define MOTE_TX_CTSN_PIN	REGISTER_BIT(PINB, 0)
	#define MOTE_RX_RTSN	REGISTER_BIT(PORTD,6)
	#define MOTE_TX			REGISTER_BIT(PORTD,2)
	#define MOTE_RX			REGISTER_BIT(PORTD,3)
	#define XBEE_RTS_ENABLED
	#define XBEE_CTS_ENABLED
	#define XBEE_PINSLEEP_ENABLED
#elif defined(POWER_STRIP_MONITOR)
	#define MOTE_TASK_ID	0x05
	#define MOTE_RESETN		REGISTER_BIT(PORTC,1)
	#define MOTE_SLEEP		REGISTER_BIT(PORTA,6)
	#define MOTE_TIMEN		REGISTER_BIT(PORTA,5)
	#define MOTE_TX_RTSN	REGISTER_BIT(PORTD,4)
	#define MOTE_TX_CTSN	REGISTER_BIT(PORTD,5)
	#define MOTE_RX_CTSN	REGISTER_BIT(PORTC,0)
	#define MOTE_RX_RTSN	REGISTER_BIT(PORTD,6)
	#undef XBEE_PINSLEEP_ENABLED
#endif

#define MOTE_UART_FLUSH	uart1_flush
#define MOTE_UART_PUTC	uart1_putc
#define MOTE_UART_GETC	uart1_getc
#define MOTE_UART_WRITE uart1_write
#define MOTE_UART_INIT  uart1_init
#define MOTE_UART_DISABLE uart1_disable

// LEDs
#define LED_BLIP_TASK_ID 0x01
#if defined(ENVIRONMENT_SENSOR)
	#define LED1		REGISTER_BIT(PORTD,5)
	#define LED2		REGISTER_BIT(PORTD,4)
#elif defined(POWER_STRIP_MONITOR)
	#define LED1		REGISTER_BIT(PORTA,0)
	#define LED2		REGISTER_BIT(PORTA,1)
#endif

// PIR sensor
#ifdef ENVIRONMENT_SENSOR
	#define USE_AMN41121
	#define PIR_TASK_ID 0x11
	#define PIR_VCC		REGISTER_BIT(PORTA,7)
	#define PIR_OUT		REGISTER_BIT(PORTD,7)
	#define PIR_OUT_PCINT 31
	#define PIR_OUT_PIN	REGISTER_BIT(PIND,7)	



	#if HW_VERSION==1 || HW_VERSION==2
		#define USE_L3GD20
		// Gyro L3GD20
		#define GYRO_TASK_ID 0x20
		#define GYRO_ADDR	0b11010110
		#define GYRO_INT2_PCINT 11
		#define GYRO_INT2	REGISTER_BIT(PORTB,3)

		#define USE_SI7005
	#elif HW_VERSION==3
		#define USE_HTU21D
	#endif
	
	// Accelerometer LIS3DH
	#define USE_LIS3DH
	#define ACCEL_TASK_ID 0x10
	#define ACCEL_ADDR	0b00110010
	#define ACCEL_INT1_PCINT 0
	#define ACCEL_INT1_PIN REGISTER_BIT(PINA,0)
	#define ACCEL_INT1	REGISTER_BIT(PORTA,0)
	#define ACCEL_INT2_PCINT 1
	#define ACCEL_INT2_PIN REGISTER_BIT(PINA,1)
	#define ACCEL_INT2	REGISTER_BIT(PORTA,1)

	// Temp/Humid
	#define HUMID_TASK_ID 0x30
	#define TEMP_TASK_ID 0x40
	#define HUMID_ADDR	0b10000000
	#if HW_VERSION==1
		#define HUMID_VCC REGISTER_BIT(PORTA,2)
	#elif HW_VERSION==2
		#define HUMID_CSN  REGISTER_BIT(PORTA,2)
		#define HUMID_VCC	REGISTER_BIT(PORTA,3)
	#endif

	// Amb. Light
	#define USE_TSL2560
	#define LIGHT_TASK_ID 0x50
	#define LIGHT_ADDR	0b01010010
	//#define LIGHT_ADDR	0b01010110
	#define LIGHT_INT_PCINT 6
	#define LIGHT_INT	REGISTER_BIT(PORTA,6)
	
	#ifdef USE_PN532
		// RFID Reader PN532
		#define RFID_TASK_ID 0x60
		#define RFID_CSN EXP_CSN
	#endif
	
	#ifdef USE_DOOR_SENSORS
		#define DOOR_SENSOR_TASK_ID 0x70
		#define DOOR_SENSOR_VCC_PIN				EXP_CSN
		#define DOOR_SENSOR_SWITCH				REGISTER_BIT(PORTB, 7)
		#define DOOR_SENSOR_SWITCH_PIN			REGISTER_BIT(PINB, 7)
		#define DOOR_SENSOR_SWITCH_PCINT		15
		#define DOOR_SENSOR_INDOOR_TOUCH_PIN	REGISTER_BIT(PINB, 6)
		#define DOOR_SENSOR_INDOOR_TOUCH_PCINT	14
		#define DOOR_SENSOR_OUTDOOR_TOUCH_PIN	REGISTER_BIT(PINB, 5)
		#define DOOR_SENSOR_OUTDOOR_TOUCH_PCINT 13
	#endif
	
	#ifdef USE_K20
		#define K20_TASK_ID 0x80
		
		#if HW_VERSION==1 || HW_VERSION==2
			#define USE_SOFTSERIAL
			#define SOFTSERIAL_TX		EXP_MOSI
			#define SOFTSERIAL_RX_PIN	EXP_MISO_PIN
			#define SOFTSERIAL_RX_PCINT EXP_MISO_PCINT
		#else
			#define K20_ADDR 0xD0 // 0x68 shifted one to the left
		#endif
	#endif
#endif 

#ifdef POWER_STRIP_MONITOR
	// TRIAC gate controls
	#define GATE1		REGISTER_BIT(PORTB,0)
	#define GATE2		REGISTER_BIT(PORTB,1)
	#define GATE3		REGISTER_BIT(PORTB,2)
	#define GATE4		REGISTER_BIT(PORTB,3)
	#define GATE5		REGISTER_BIT(PORTB,4)
	#define GATE6		REGISTER_BIT(PORTC,7)

	// CS5467 Chips
	#define USE_CS5467
	#define POWERMON_TASK_ID 0x10
	#define POWERMON_NUM_CHIPS 3
	#define POWERMON_NUM_CHANNELS 6

	#define POWERMON_CS1 REGISTER_BIT(PORTA,4)
	#define POWERMON_CS2 REGISTER_BIT(PORTA,3)
	#define POWERMON_CS3 REGISTER_BIT(PORTA,2)
#endif


#ifndef sbi
#define sbi(X,Y) (X) |= _BV(Y)
#endif

#ifndef cbi
#define cbi(X,Y) (X) &= ~_BV(Y)
#endif

#ifndef tbs
#define tbs(X,Y) (((X) & _BV(Y)) != 0)
#endif

#ifndef tbc
#define tbc(X,Y) (((X) & _BV(Y)) == 0)
#endif


#endif /* DEVICEDEFS_H_ */