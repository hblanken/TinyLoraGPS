const int GPSBaud = 9600;

#include "SoftwareSerial256.h"
#include <SPI.h>
#include <LoRa.h>

// The Arduino pins used by the GPS module
const int GPS_ONOFFPin = A3;
const int GPS_SYSONPin = A2;
const int GPS_RXPin = A1;
const int GPS_TXPin = A0;
const int chipSelect = 10;


// The GPS connection is attached with a software serial port
SoftwareSerial Gps_serial(GPS_RXPin, GPS_TXPin);

// Set which sentences should be enabled on the GPS module
char nmea[] = {'1'/*GPGGA*/, '0'/*GNGLL*/, '0'/*GNGSA*/, '0'/*GPGSV/GLGSV*/, '0'/*GNRMC*/, '0'/*GNVTG*/, '0'/*not supported*/, '0'/*GNGNS*/};

int count = 0;

void setup() {
  Gps_serial.begin(GPSBaud);
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");
  
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  pinMode(GPS_SYSONPin, INPUT);
  digitalWrite(GPS_ONOFFPin, LOW);
  pinMode(GPS_ONOFFPin, OUTPUT);
  delay(100);
  Serial.print("Attempting to wake GPS module.. ");
  //digitalWrite( GPS_ONOFFPin, HIGH );
  //delay(300);
  while (digitalRead( GPS_SYSONPin ) == LOW )
  {
    // Need to wake the module
    digitalWrite( GPS_ONOFFPin, HIGH ); //LOW
    delay(100);

    digitalWrite( GPS_ONOFFPin, LOW ); //HIGH
    delay(100);
  }
  Serial.println("done.");
  delay(100);


  char command[] = "$PSRF103,00,00,00,01*xx\r\n";
  for (int i = 0; i < 8; i++) {
    command[10] = i + '0';
    command[16] = nmea[i];
    int c = 1;
    byte checksum = command[c++];
    while (command[c] != '*')
      checksum ^= command[c++];
    command[c + 1] = (checksum >> 4) + (((checksum >> 4) < 10) ? '0' : ('A' - 10));
    command[c + 2] = (checksum & 0xF) + (((checksum & 0xF) < 10) ? '0' : ('A' - 10));
    Gps_serial.print(command);
    delay(20);
  }
  
  while (Gps_serial.available())
    Gps_serial.read();
}

void loop() {
  while (Gps_serial.read() != '$') {
    //do other stuff here
  }
  while (Gps_serial.available() < 5);
  Gps_serial.read(); Gps_serial.read(); //skip two characters
  char c = Gps_serial.read();
  //determine sentence type
  if (c == 'R' || c == 'G') {
    c = Gps_serial.read();
    if (c == 'M') {
      logNMEA(1);
    } else if (c == 'G') {
      logNMEA(2);
    }
  }
}

void logNMEA(int type) {
  uint8_t buffer[100];
  buffer[0] = '$';
  buffer[1] = 'G';
  buffer[2] = 'P';
  if (type == 1) {
    buffer[3] = 'R';
    buffer[4] = 'M';
  } else if (type == 2) {
    buffer[3] = 'G';
    buffer[4] = 'G';
  }
  int counter = 5;
  char c = 0;
  while (!Gps_serial.available());
  c = Gps_serial.read();
  while (c != '*') {
    buffer[counter++] = c;
    while (!Gps_serial.available());
    c = Gps_serial.read();
  }
  buffer[counter++] = c;
  while (!Gps_serial.available());
  c = Gps_serial.read();
  buffer[counter++] = c;
  while (!Gps_serial.available());
  c = Gps_serial.read();
  buffer[counter++] = c;
  buffer[counter++] = 0x0D;
  buffer[counter++] = 0x0A;
  buffer[counter] = '\0';


  c = 1;
  byte checksum = buffer[c++];
  while (buffer[c] != '*')
    checksum ^= buffer[c++];
  buffer[c + 1] = (checksum >> 4) + (((checksum >> 4) < 10) ? '0' : ('A' - 10));
  buffer[c + 2] = (checksum & 0xF) + (((checksum & 0xF) < 10) ? '0' : ('A' - 10));

  Serial.println((char *)buffer);
  Serial.print("Sending packet: ");
  Serial.println(count);

  // send packet
  LoRa.beginPacket();
  LoRa.print("head");
  LoRa.print((char *)buffer);
  LoRa.print(count);
  LoRa.endPacket();

  count++;

  delay(500);
}
