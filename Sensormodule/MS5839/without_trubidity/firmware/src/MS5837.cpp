#include "MS5837.h"
#include <Wire.h>

const uint8_t MS5837_ADDR = 0x76;
const uint8_t MS5837_RESET = 0x1E;
const uint8_t MS5837_ADC_READ = 0x00;
const uint8_t MS5837_PROM_READ = 0xA0;
const uint8_t MS5837_CONVERT_D1_8192 = 0x48; //A;
const uint8_t MS5837_CONVERT_D2_8192 = 0x58; //A;


bool MS5837::begin(TwoWire &wirePort) {
	return (init(wirePort));
}

bool MS5837::init(TwoWire &wirePort) {
	_i2cPort = &wirePort; //Grab which port the user wants us to use
	// Reset the MS5837, per datasheet
	_i2cPort->beginTransmission(MS5837_ADDR);
	_i2cPort->write(MS5837_RESET);
	_i2cPort->endTransmission();

	// Wait for reset to complete
	delay(10);

	// Read calibration values and CRC
	for ( uint8_t i = 0 ; i < 7 ; i++ ) {
		_i2cPort->beginTransmission(MS5837_ADDR);
		_i2cPort->write(MS5837_PROM_READ+(i*2));
		_i2cPort->endTransmission();

		_i2cPort->requestFrom(MS5837_ADDR,2);
		C[i] = (_i2cPort->read() << 8) | _i2cPort->read();
		prom_raw[i] = C[i];
	}

	// Verify that data is correct with CRC
	uint8_t crcRead = C[0] >> 12;
	uint8_t crcCalculated = crc4(C);

	if ( crcCalculated != crcRead ) {
		return false; // CRC fail
	}
	return true;
}

void MS5837::read() {
	//Check that _i2cPort is not NULL (i.e. has the user forgoten to call .init or .begin?)
	if (_i2cPort == NULL)
	{
		return;
	}

	// Request D1 conversion
	_i2cPort->beginTransmission(MS5837_ADDR);
	_i2cPort->write(MS5837_CONVERT_D1_8192);
	_i2cPort->endTransmission();

	delay(10); // Max conversion time per datasheet

	_i2cPort->beginTransmission(MS5837_ADDR);
	_i2cPort->write(MS5837_ADC_READ);
	_i2cPort->endTransmission();

	_i2cPort->requestFrom(MS5837_ADDR,3);
	D1_pres = 0;
	D1_pres = _i2cPort->read();
	D1_pres = (D1_pres << 8) | _i2cPort->read();
	D1_pres = (D1_pres << 8) | _i2cPort->read();

	D1_pres_raw = D1_pres;
	// Request D2 conversion
	_i2cPort->beginTransmission(MS5837_ADDR);
	_i2cPort->write(MS5837_CONVERT_D2_8192);
	_i2cPort->endTransmission();

	delay(10); // Max conversion time per datasheet

	_i2cPort->beginTransmission(MS5837_ADDR);
	_i2cPort->write(MS5837_ADC_READ);
	_i2cPort->endTransmission();

	_i2cPort->requestFrom(MS5837_ADDR,3);
	D2_temp = 0;
	D2_temp = _i2cPort->read();
	D2_temp = (D2_temp << 8) | _i2cPort->read();
	D2_temp = (D2_temp << 8) | _i2cPort->read();

	D2_temp_raw = D2_temp;

	calculate();
}

void MS5837::calculate() {
	// Given C1-C6 and D1, D2, calculated TEMP and P
	// Do conversion first and then second order temp compensation

	int32_t dT = 0;
	int64_t SENS = 0;
	int64_t OFF = 0;
	int32_t SENSi = 0;
	int32_t OFFi = 0;
	int32_t Ti = 0;
	int64_t OFF2 = 0;
	int64_t SENS2 = 0;

	// Terms called
	dT = D2_temp-uint32_t(C[5])*256l;


		SENS = (int64_t)(C[1])*32768l+((int64_t)(C[3])*dT)/256l;
		OFF = (int64_t)(C[2])*65536l+((int64_t)(C[4])*dT)/128l;
		P = (D1_pres*SENS/(2097152l)-OFF)/(8192l);
	

	// Temp conversion
	TEMP = 2000l+int64_t(dT)*C[6]/8388608LL;

	//Second order compensation
		if((TEMP/100)<20){         //Low temp
			Ti = (3*int64_t(dT)*int64_t(dT))/(8589934592LL);
			OFFi = (3*(TEMP-2000)*(TEMP-2000))/2;
			SENSi = (5*(TEMP-2000)*(TEMP-2000))/8;
			if((TEMP/100)<-15){    //Very low temp
				OFFi = OFFi+7*(TEMP+1500l)*(TEMP+1500l);
				SENSi = SENSi+4*(TEMP+1500l)*(TEMP+1500l);
			}
		}
		else if((TEMP/100)>=20){    //High temp
			Ti = 2*(dT*dT)/(137438953472LL);
			OFFi = (1*(TEMP-2000)*(TEMP-2000))/16;
			SENSi = 0;
		}
	

	OFF2 = OFF-OFFi;           //Calculate pressure and temp second order
	SENS2 = SENS-SENSi;

	TEMP = (TEMP-Ti);

		P = (((D1_pres*SENS2)/2097152l-OFF2)/8192l);
	
}

int16_t MS5837::prom(uint8_t address){
	return prom_raw[address];

}

int32_t MS5837::pressure() {
	return P;
}

int32_t MS5837::temperature() {
	return TEMP;
}

int32_t MS5837::raw_temp() {
	return D2_temp_raw;
}

int32_t MS5837::raw_press() {
	return D1_pres_raw;
}

uint8_t MS5837::crc4(uint16_t n_prom[]) {
	uint16_t n_rem = 0;

	n_prom[0] = ((n_prom[0]) & 0x0FFF);
	n_prom[7] = 0;

	for ( uint8_t i = 0 ; i < 16; i++ ) {
		if ( i%2 == 1 ) {
			n_rem ^= (uint16_t)((n_prom[i>>1]) & 0x00FF);
		} else {
			n_rem ^= (uint16_t)(n_prom[i>>1] >> 8);
		}
		for ( uint8_t n_bit = 8 ; n_bit > 0 ; n_bit-- ) {
			if ( n_rem & 0x8000 ) {
				n_rem = (n_rem << 1) ^ 0x3000;
			} else {
				n_rem = (n_rem << 1);
			}
		}
	}

	n_rem = ((n_rem >> 12) & 0x000F);

	uint8_t tp_temp = n_rem ^0x00;

	return n_rem ^ 0x00;
}