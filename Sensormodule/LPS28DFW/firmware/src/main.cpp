#include <Arduino.h>
#include <Wire.h>
#include "LPS28DFW.h"

uint8_t VERSION = 1;

LPS28DFW sensor;

int32_t TEMPERATURE;
int32_t PRESSURE;
int32_t TEMPERATURE_RAW;
int32_t PRESSURE_RAW;

char output[200];
int output_counter = 0;

void hex_switch(uint8_t input)
{
  switch (input)
  {
  case 0: output[output_counter] = '0'; break;
  case 1: output[output_counter] = '1'; break;
  case 2: output[output_counter] = '2'; break;
  case 3: output[output_counter] = '3'; break;
  case 4: output[output_counter] = '4'; break;
  case 5: output[output_counter] = '5'; break;
  case 6: output[output_counter] = '6'; break;
  case 7: output[output_counter] = '7'; break;
  case 8: output[output_counter] = '8'; break;
  case 9: output[output_counter] = '9'; break;
  case 10: output[output_counter] = 'A'; break;
  case 11: output[output_counter] = 'B'; break;
  case 12: output[output_counter] = 'C'; break;
  case 13: output[output_counter] = 'D'; break;
  case 14: output[output_counter] = 'E'; break;
  case 15: output[output_counter] = 'F'; break;
  }
  output_counter = output_counter + 1;
}

// Convert an unsigned-8-bit-input into two chars as hex
void int_to_hex(uint8_t input)
{
  uint8_t four_bit;
  four_bit = (input & 0xF0) >> 4;
  hex_switch(four_bit);
  four_bit = input & 0x0F;
  hex_switch(four_bit);
}

// Convert an signed-8-bit-input into two chars as hex
void hex_print_8(int8_t input)
{
  int_to_hex(input);
}

// Convert an signed-16-bit-input into two chars as hex
void hex_print_16(int16_t input)
{
  uint8_t single_byte;
  for (int i = 1; i >= 0; i--)
  {
    single_byte = byte(input >> (i * 8));
    int_to_hex(single_byte);
  }
}

// Convert an signed-32-bit-input into two chars as hex
void hex_print_32(int32_t input)
{
  uint8_t single_byte;
  for (int i = 3; i >= 0; i--)
  {
    single_byte = byte(input >> (i * 8));
    int_to_hex(single_byte);
  }
}

void setup()
{
  Serial.begin(9600); // Start Serial-interface
  Wire.begin();       // Start I2C-Interface
  sensor.init();      // Init the LPS28DFW-Sensor;
}

void loop()
{
  // Update pressure and temperature readings
  output_counter = 0;
  sensor.read();
  TEMPERATURE = sensor.temperature();
  PRESSURE = sensor.pressure();

  // Build the Output-Message
  output[output_counter] = 'S';
  output_counter = output_counter + 1;
  output[output_counter] = '2';
  output[output_counter + 1] = '0';
  output_counter = output_counter + 2;
  hex_print_8(VERSION);

  output[output_counter] = '2';
  output[output_counter + 1] = '2';
  output_counter = output_counter + 2;
  hex_print_16(TEMPERATURE);

  output[output_counter] = '2';
  output[output_counter + 1] = '3';
  output_counter = output_counter + 2;
  hex_print_32(PRESSURE); // 30 + 2

  output[output_counter] = 'X';

  for (int i = 0; i <= output_counter; i++)
  {
    Serial.print(output[i]); // Send the message
  }
  Serial.println("");
  delay(500); // Wait 500ms, than start-over
}