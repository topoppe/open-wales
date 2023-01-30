#include <arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "credentials.h"
#include "lorawan.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>

#define mosfet_pin 3
#define rs485_rx_pin 0
#define rs485_tx_pin 10

bool GOTO_DEEPSLEEP = false;

const unsigned TX_INTERVAL = 20; // Seconds between sending two datasets
RTC_DATA_ATTR int bootCount = 0;
uint8_t FW_VERSION = 1;

Adafruit_BME280 bme;

StaticJsonDocument<1000> output_data;
char json_message[1000];

unsigned long lastMillis = 0;

// Readout battery State-of-charge
int read_soc()
{
  uint8_t max17261_address = 0x36;
  Wire.beginTransmission(max17261_address);
  Wire.write(0x06);
  uint8_t last_status = Wire.endTransmission(false);
  Wire.requestFrom(max17261_address, 1);
  byte soc = Wire.read();
  Wire.endTransmission();
  return soc;
}

// Convert string into hex
int StrToHex(char str[])
{
  return (int)strtol(str, 0, 16);
}

// activate RS485-transceiver, receive one dataset from sensormodule and deactivate transceiver
void sensor_read(Stream &serial_port)
{
  String sensor_incoming;
  char single_input;
  int y;
  char sensor_32_bit_string[9];
  char sensor_16_bit_string[5];

  Wire.end();
  digitalWrite(8, LOW);
  delay(5000);
  sensor_incoming = "No Data!";
  lastMillis = millis();
  // first flush the input buffer:
  while (serial_port.available() > 0)
  {
    single_input = serial_port.read();
  }
  // then start reading in the string
  while ((sensor_incoming[0] != 'S') && (sensor_incoming[sensor_incoming.length() - 1] != 'X') && ((millis() - lastMillis) < 2500))
  {
    if (serial_port.available())
    {
      sensor_incoming = serial_port.readString();
    }
  }
  // Sensor Typ MS5837
  if (sensor_incoming[1] == '1')
  {
    y = 0;
    for (int i = 13; i < 21; i++)
    {
      sensor_32_bit_string[y] = sensor_incoming[i];
      y = y + 1;
    }
    sensor_32_bit_string[8] = '\0';
    output_data["MS5837"]["wassertemperatur"] = StrToHex(sensor_32_bit_string) / 100.0F;

    y = 0;
    for (int i = 23; i < 31; i++)
    {
      sensor_32_bit_string[y] = sensor_incoming[i];
      y = y + 1;
    }
    sensor_32_bit_string[8] = '\0';
    output_data["MS5837"]["wasserdruck"] = StrToHex(sensor_32_bit_string);
  }
  // Sensor Typ LPS28DFW
  else if (sensor_incoming[1] == '2')
  {
    y = 0;
    for (int i = 7; i < 11; i++)
    {
      sensor_16_bit_string[y] = sensor_incoming[i];
      y = y + 1;
    }
    sensor_16_bit_string[4] = '\0';
    output_data["LPS28DFW"]["wassertemperatur"] = StrToHex(sensor_16_bit_string) / 100.0F;

    y = 0;
    for (int i = 13; i < 21; i++)
    {
      sensor_32_bit_string[y] = sensor_incoming[i];
      y = y + 1;
    }
    sensor_32_bit_string[8] = '\0';
    output_data["LPS28DFW"]["wasserdruck"] = StrToHex(sensor_32_bit_string) / 2048.0F;
  }
  digitalWrite(8, HIGH);
  Wire.begin(8, 9);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");

  pinMode(mosfet_pin, OUTPUT);
  digitalWrite(mosfet_pin, HIGH);

  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  Serial1.begin(9600, SERIAL_8N1, rs485_rx_pin, rs485_tx_pin);
  Serial1.setTimeout(100);
  lorawan_init(TX_INTERVAL); // start LoRaWAN-Stack
  Wire.begin(8, 9);          // start I2C

  unsigned bme_status;
  bme_status = bme.begin(0x76, &Wire); //  start BME Sensor
}

unsigned long time_now = millis();
int counter = 0;

uint8_t buffer[100];
float float_temp;
uint8_t float_converted[4];
int send_array_position = 0;
int i;
bool mosfet_state = 0;
String str;

void loop()
{
  os_runloop_once();
  const bool timeCriticalJobs = os_queryTimeCriticalJobs(ms2osticksRound((TX_INTERVAL * 1000)));
  if (!timeCriticalJobs && GOTO_DEEPSLEEP == true && !(LMIC.opmode & OP_TXRXPEND))
  {
    SaveLMICToRTC(TX_INTERVAL);
    GoDeepSleep();
  }
  else if (millis() >= (time_now + TX_INTERVAL))
  {
    time_now = millis();
    digitalWrite(mosfet_pin, HIGH);
    set_nss_pin(LOW);
    sensor_read(Serial1);
    set_nss_pin(HIGH);
    output_data["intern"]["soc"] = read_soc();
    output_data["intern"]["lufttemperatur"] = bme.readTemperature();
    output_data["intern"]["luftdruck"] = bme.readPressure() / 100.0F;
    output_data["intern"]["luftfeuchtigkeit"] = bme.readHumidity();
    serializeJson(output_data, json_message);
    output_data.clear();
    Serial.println(json_message);

    int string_laenge = 0;
    for (int i = 0; i <= 1000; i++)
    {
      if (json_message[i] == '\0')
      {
        break;
      }
      string_laenge++;
    }

    uint8_t data[string_laenge];
    for (i = 0; i <= string_laenge; i++)
    {
      data[i] = json_message[i];
    }
    send_lorawan_data(data, sizeof(data));
    digitalWrite(mosfet_pin, LOW);
  }
}
