/* Blue Robotics Arduino LPS28DFW-30BA Pressure/Temperature Sensor Library
------------------------------------------------------------
Title: Blue Robotics Arduino LPS28DFW-30BA Pressure/Temperature Sensor Library
Description: This library provides utilities to communicate with and to
read data from the Measurement Specialties LPS28DFW-30BA pressure/temperature
sensor.
Authors: Rustom Jehangir, Blue Robotics Inc.
         Adam Šimko, Blue Robotics Inc.
-------------------------------
The MIT License (MIT)
Copyright (c) 2015 Blue Robotics Inc.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-------------------------------*/

#ifndef LPS28DFW_LIB
#define LPS28DFW_LIB

#include "Arduino.h"
#include <Wire.h>

class LPS28DFW {
public:

	bool init(TwoWire &wirePort = Wire);
	bool begin(TwoWire &wirePort = Wire); // Calls init()

	/** The read from I2C takes up to 40 ms, so use sparingly is possible.
	 */
	void read();

	/** Pressure returned in mbar or mbar*conversion rate.
	 */
	uint32_t pressure();

	/** Temperature returned in deg C.
	 */
	uint16_t temperature();



private:

	//This stores the requested i2c port
	TwoWire * _i2cPort;

	uint16_t temperature_raw;
	uint32_t pressure_raw;
};

#endif