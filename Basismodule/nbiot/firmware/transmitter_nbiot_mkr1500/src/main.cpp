#include <MKRNB.h>
#include <ArduinoBearSSL.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include "ArduinoLowPower.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "wiring_private.h"

Uart Serial0(&sercom0, A4, A3, SERCOM_RX_PAD_1, UART_TX_PAD_0); // Create the new UART instance assigning it to pin A3 (rx) and A4 (tx)

const char pin[] = "";
const char apn[] = "iot.1nce.net";
const char login[] = "";
const char password[] = "";

#define FEEDBACK_PIN 1
NBClient net;
GPRS gprs;
NB nbAccess; //(true);
NBScanner scannerNetworks;

NBUDP Udp;
unsigned int localPort = 2390;
IPAddress timeServer(194, 163, 172, 98);
const int NTP_PACKET_SIZE = 48;     // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming and outgoing packets

Adafruit_BME280 bme;

StaticJsonDocument<1000> output_data;
char json_message[1000];
int counter;
unsigned long lastMillis = 0;
unsigned long epoch;

int port = 80;
char server[] = "www.topo-eng.de"; // hostname of web server:
String HTTP_METHOD = "POST";
String PATH_NAME = "/pegel/receive_post.php";

BearSSLClient sslClient(net);
bool time_is_set = false;

unsigned long sendNTPpacket(IPAddress &address)
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  Udp.beginPacket(address, 123);
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

unsigned long getTime()
{
  return epoch;
}

// Receive NTP-Time
unsigned long getNTP()
{
  Udp.begin(localPort);
  sendNTPpacket(timeServer);
  delay(1000);
  if (Udp.parsePacket())
  {
    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    epoch = secsSince1900 - seventyYears;
    Udp.stop();
    time_is_set = true;
  }
  else
  {
    Udp.stop();
    time_is_set = true;
  }
}

// Set Modem Radio Access Technology
bool setRAT(String choice)
{
  String response;
  MODEM.sendf("AT+COPS=2");
  MODEM.waitForResponse(2000);
  MODEM.sendf("AT+URAT=%s", choice.c_str());
  MODEM.waitForResponse(2000, &response);
  return true;
}

// Set Modem-Config and save it
bool apply()
{
  MODEM.send("AT+CFUN=15");
  MODEM.waitForResponse(5000);
  delay(5000);
  do
  {
    delay(1000);
    MODEM.noop();
  } while (MODEM.waitForResponse(1000) != 1);
  return true;
}

// Connect to cellular network
void connect()
{
  bool connected = false;
  while (!connected)
  {
    if ((nbAccess.begin(pin) == NB_READY) &&
        (gprs.attachGPRS() == GPRS_READY))
    { // gprs.attachGPRS(apn, login, password)
      connected = true;
    }
    else
    {
      Serial.print(".");
      delay(1000);
    }
  }
  getNTP();
}

// Convert string to hex
int StrToHex(char str[])
{
  return (int)strtol(str, 0, 16);
}

void setup()
{
  Serial1.begin(9600);
  Serial1.setTimeout(150);
  Serial.begin(115200);
  Serial0.begin(9600);
  Serial0.setTimeout(150);
  pinPeripheral(A4, PIO_SERCOM_ALT); // Assign RX function to pin A3
  pinPeripheral(A3, PIO_SERCOM_ALT); // Assign TX function to pin A4
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FEEDBACK_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(FEEDBACK_PIN, HIGH);
  bme.begin(0x76); // Start BME Sensor
  connect();       // Connect to cell-network
}

void sensor_read(Stream &serial_port)
{
  String sensor_incoming;
  char single_input;
  int y;
  char sensor_32_bit_string[9];
  char sensor_16_bit_string[5];

  sensor_incoming = "No Data!";
  lastMillis = millis();
  // first flush the input buffer:
  while (serial_port.available() > 0)
  {
    single_input = serial_port.read();
  }
  // then start reading in the string
  y = 0;
  while ((sensor_incoming[0] != 'S') && (sensor_incoming[y - 1] != 'X') && ((millis() - lastMillis) < 5000))
  { //
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

    output_data["MS5837"]["roh_string"] = sensor_incoming.substring(0, sensor_incoming.length() - 2);
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

    output_data["LPS28DFW"]["roh_string"] = sensor_incoming.substring(0, sensor_incoming.length() - 2);
  }
}

void loop()
{
  // Get NTP Time and set SSL-Lib
  int bat_sensorValue = analogRead(ADC_BATTERY);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  float bat_voltage = bat_sensorValue * (4.3 / 1023.0);
  output_data["intern"]["bat_spannung"] = bat_voltage;
  lastMillis = millis();
  String sensor_incoming;
  int sensor_string_length = 0;
  char sensor_32_bit_string[9];
  int y = 0;
  sensor_read(Serial0);
  sensor_read(Serial1);
  output_data["intern"]["lufttemperatur"] = bme.readTemperature();
  output_data["intern"]["luftdruck"] = bme.readPressure() / 100.0F;
  output_data["intern"]["luftfeuchtigkeit"] = bme.readHumidity();
  output_data["intern"]["carrier"] = scannerNetworks.getCurrentCarrier();
  output_data["intern"]["mobile_reception"] = map(scannerNetworks.getSignalStrength().toInt(), 0, 31, 0, 100);
  serializeJson(output_data, json_message);
  output_data.clear();

  Serial.println("Sensoren fertig, verbinde mit Server");
  Serial.println(nbAccess.getTime());
  bool daten_gesendet = false;
  int sende_versuch = 0;

  while (!daten_gesendet && sende_versuch <= 10)
  {

    if (int result = net.connect(server, 80))
    {
      int string_laenge = 0;
      for (int i = 0; i <= 1000; i++)
      {
        if (json_message[i] == '\0')
        {
          break;
        }
        string_laenge++;
      }
      Serial.print("Mit Server verbunden! Result:");
      Serial.println(result);
      Serial.println("POST " + PATH_NAME + "?UUID=SENSOR2 HTTP/1.1");
      net.println("POST " + PATH_NAME + "?UUID=SENSOR2 HTTP/1.1");
      Serial.println("Host: " + String(server));
      net.println("Host: " + String(server));
      Serial.println("Content-Type: text/plain");
      net.println("Content-Type: text/plain");
      net.println("Connection: close");
      Serial.print("Content-Length: ");
      net.print("Content-Length: ");
      Serial.println(string_laenge);
      net.println(string_laenge);
      Serial.println();
      net.println();
      Serial.print(json_message);
      net.print(json_message);
      Serial.println();
      net.println();
    }
    Serial.println("Sensoren gesendet!");
    daten_gesendet = true;
    int timeout_counter = 0;
    net.stop();
    if (!daten_gesendet)
    {
      sende_versuch++;
    }
  }
  Serial.println("Daten gesendet! Abschalten...");
  nbAccess.shutdown();
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(FEEDBACK_PIN, LOW);
  delay(50000000);
}

void SERCOM0_Handler()
{

  Serial0.IrqHandler();
}