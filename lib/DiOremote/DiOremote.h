/*
  DiOremote.h - Library for controlling DiO devices.
  Created by Charles GRASSIN, July 31, 2017.
  Under MIT License (free to use, modify, redistribute, etc.).
  www.charleslabs.fr
*/
#ifndef DiOremote_h
#define DiOremote_h

	#include "Arduino.h"

	//Default number of time a command is sent
	#define DiOremote_DEFAULT_TRY_COUNT 5

	//Protocol timing (in us)
	#define DiOremote_START_FRAME_1 220
	#define DiOremote_START_FRAME_0 2675
	#define DiOremote_THIGH 220
	#define DiOremote_TLOW_0 350
	#define DiOremote_TLOW_1 1400
	#define DiOremote_END_FRAME_1 220
	#define DiOremote_END_FRAME_0 10600

	class DiOremote
	{
		public:
			DiOremote(int pin);
			void send(unsigned long code);
		private:
			int _pin;
			void _send1();
			void _send0();
	};

#endif
