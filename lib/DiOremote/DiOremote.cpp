/*
  DiOremote.c - Library for controlling DiO devices.
  Created by Charles GRASSIN, July 31, 2017.
  Under MIT License (free to use, modify, redistribute, etc.).
  www.charleslabs.fr
*/
#include "Arduino.h"
#include "DiOremote.h"

DiOremote::DiOremote(int pin){
	this->_pin = pin;
	pinMode(pin, OUTPUT);
}

void DiOremote::send(unsigned long codeToSend){
	unsigned long code;
	for (int j = 0; j < DiOremote_DEFAULT_TRY_COUNT; j++) {
		//Start sequence
		digitalWrite(_pin, HIGH);
		delayMicroseconds(DiOremote_START_FRAME_1);
		digitalWrite(_pin, LOW); 
		delayMicroseconds(DiOremote_START_FRAME_0);

		//Send the code
		code = codeToSend;
		for (int i = 0; i < 32; i++) {
			if (code & 0x80000000L) {
				//1 => send 10
				this->_send1();
				this->_send0();
			} else {
				//1 => send 01
				this->_send0();
				this->_send1();
			}
			code <<= 1;
		}

		//End sequence
		digitalWrite(_pin, HIGH);
		delayMicroseconds(DiOremote_END_FRAME_1);
		digitalWrite(_pin, LOW);
		delayMicroseconds(DiOremote_END_FRAME_0);

		//digitalWrite(_pin, HIGH);
		//delayMicroseconds(DiOremote_THIGH);
	}
}

void DiOremote::_send1(){
	digitalWrite(_pin, HIGH);
	delayMicroseconds(DiOremote_THIGH);
	digitalWrite(_pin, LOW);
	delayMicroseconds(DiOremote_TLOW_1);
}

void DiOremote::_send0(){
	digitalWrite(_pin, HIGH);
	delayMicroseconds(DiOremote_THIGH);
	digitalWrite(_pin, LOW);
	delayMicroseconds(DiOremote_TLOW_0);
}
