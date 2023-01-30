#include "LPS28DFW.h"
#include <Wire.h>

const uint8_t LPS28DFW_ADDR = 0x5C;
const uint8_t LPS23DFW_WHO_AM_I = 0x0F;
const uint8_t LPS23DFW_CTRL_REG1 = 0x10;
const uint8_t LPS23DFW_CTRL_REG2 = 0x11;
const uint8_t LPS23DFW_CTRL_REG3 = 0x12;
const uint8_t LPS23DFW_CTRL_REG4 = 0x13;
const uint8_t LPS23DFW_STATUS_REG = 0x27;
const uint8_t LPS23DFW_PRESS_OUT_XL_REG = 0x28;
const uint8_t LPS23DFW_TEMP_OUT_L_REG = 0x2B;

const uint8_t LPS23DFW_SWRESET = 0x04;


const uint8_t LPS28DFW_RESET = 0x1E;
const uint8_t LPS28DFW_ADC_READ = 0x00;
const uint8_t LPS28DFW_PROM_READ = 0xA0;
const uint8_t LPS28DFW_CONVERT_D1_8192 = 0x48; //A;
const uint8_t LPS28DFW_CONVERT_D2_8192 = 0x58; //A;


bool LPS28DFW::begin(TwoWire &wirePort) {
	return (init(wirePort));
}

bool LPS28DFW::init(TwoWire &wirePort) {

uint8_t result = 0;
uint8_t whoami = 0;
	_i2cPort = &wirePort; //Grab which port the user wants us to use
	// Reset the LPS28DFW, per datasheet
	_i2cPort->beginTransmission(LPS28DFW_ADDR);
	result = _i2cPort->write(LPS23DFW_CTRL_REG2);
	result = _i2cPort->write(LPS23DFW_SWRESET);
	_i2cPort->endTransmission();

	// // Wait for reset to complete
	delay(10);

	//Check if Sensor is right
	_i2cPort->beginTransmission(LPS28DFW_ADDR);
	result = _i2cPort->write(LPS23DFW_WHO_AM_I);
	_i2cPort->endTransmission(false);
	_i2cPort->requestFrom(LPS28DFW_ADDR,1);
	whoami = _i2cPort->read();

	//set mode to mode2, full scale up  to 2060hPa
	_i2cPort->beginTransmission(LPS28DFW_ADDR);
	result = _i2cPort->write(LPS23DFW_CTRL_REG2);
	result = _i2cPort->write(0b01000000);
	_i2cPort->endTransmission();

	//set ODR tot Oneshot, AVG to 512
	_i2cPort->beginTransmission(LPS28DFW_ADDR);
	result = _i2cPort->write(LPS23DFW_CTRL_REG1);
	result = _i2cPort->write(0b00000111);
	_i2cPort->endTransmission();



	if ( whoami != 0xB4 ) {
		return false; 
	}
	return true;
}

void LPS28DFW::read() {
	bool conversion_finished = false;
	uint8_t result = 0;
	uint8_t pressure_raw_xl, pressure_raw_l, pressure_raw_h;
	uint8_t temperature_raw_l, temperature_raw_h;
	pressure_raw = 0;
	temperature_raw = 0;
	//Check that _i2cPort is not NULL (i.e. has the user forgoten to call .init or .begin?)
	if (_i2cPort == NULL)
	{
		return;
	}

	// Request oneshot conversion
	_i2cPort->beginTransmission(LPS28DFW_ADDR);
	result = _i2cPort->write(LPS23DFW_CTRL_REG2);
	result = _i2cPort->write(0b01000001);
	_i2cPort->endTransmission();

	//wait for conversion to finish
	while (!conversion_finished){
		delay(1);
		_i2cPort->beginTransmission(LPS28DFW_ADDR);
		_i2cPort->write(LPS23DFW_STATUS_REG);
		_i2cPort->endTransmission(false);
		_i2cPort->requestFrom(LPS28DFW_ADDR,1);
		result = _i2cPort->read();
		if (result && 0b00000011){
			conversion_finished = true;
		}
	}

	//read out pressure	
	_i2cPort->beginTransmission(LPS28DFW_ADDR);
	_i2cPort->write(LPS23DFW_PRESS_OUT_XL_REG);
	_i2cPort->endTransmission(false);
	_i2cPort->requestFrom(LPS28DFW_ADDR,3);
	pressure_raw_xl = _i2cPort->read();
	pressure_raw_l = _i2cPort->read();
	pressure_raw_h = _i2cPort->read();
	pressure_raw = pressure_raw_h;
	pressure_raw = (pressure_raw << 8) | pressure_raw_l;
	pressure_raw = (pressure_raw << 8) | pressure_raw_xl;

	//read out temperature
	_i2cPort->beginTransmission(LPS28DFW_ADDR);
	_i2cPort->write(LPS23DFW_TEMP_OUT_L_REG);
	_i2cPort->endTransmission(false);
	_i2cPort->requestFrom(LPS28DFW_ADDR,2);
	temperature_raw_l = _i2cPort->read();
	temperature_raw_h = _i2cPort->read();
	temperature_raw = temperature_raw_h;
	temperature_raw = (temperature_raw << 8) | temperature_raw_l; 
}

uint32_t LPS28DFW::pressure() {
	return pressure_raw;
}

uint16_t LPS28DFW::temperature() {
	return temperature_raw;
}
