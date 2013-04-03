/*
 * xbee_AT_settings.h
 *
 * Created: 4/2/2013 3:03:06 PM
 *  Author: kweekly
 */ 


#ifndef XBEE_AT_SETTINGS_H_
#define XBEE_AT_SETTINGS_H_

const unsigned char XBEE_AT_SETTING_STR[] PROGMEM =  
 "ID\x12\x34"
",D6\x1" // enable flow control RTS
",CH\xC"
",MY\xFF\xFF"
",MM\x0"
",RR\x2"
",RN\x3"
",CE\x0"
",EE\x0"
",SM\x1" // pin hibernate
",SO\x2" // supress IO samples
",AP\x2"
",XX"
;


#endif /* XBEE_AT_SETTINGS_H_ */