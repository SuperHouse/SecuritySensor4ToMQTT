/**
 Security Sensor (4 channel) to MQTT

 Copyright 2015 SuperHouse Automation Pty Ltd <info@superhouse.tv>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*--------------------------- Configuration ------------------------------*/
/* Network config */
#define ENABLE_DHCP                 true   // true/false
#define ENABLE_MAC_ADDRESS_ROM      true   // true/false
#define MAC_I2C_ADDRESS             0x50   // Microchip 24AA125E48 I2C ROM address
static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // Set if no MAC ROM
static uint8_t ip[] = { 192, 168, 1, 35 }; // Use if DHCP disabled

/* Display config */
#define ENABLE_OLED                 false  // true/false. Enable if you have a Freetronics OLED128 connected
const long oled_timeout =            20;   // Seconds before screen blanks after last activity

// Panel-specific configuration:
int panelId = 15;                          // A unique identifier for this panel

const long seconds_start_delay = 60;  // Pause during startup to let PIRs settle down

/*------------------------------------------------------------------------*/

#include <SPI.h>
#include "Ethernet.h"
#include <PubSubClient.h>

/* Required for OLED */
#include "Wire.h"
#include <SD.h>
#include <FTOLED.h>
#include <fonts/SystemFont5x7.h>

#define STATE_SHORT        0
#define STATE_NORMAL       1
#define STATE_TAMPER       2
#define STATE_ALARM        3
#define STATE_ALARM_TAMPER 4
#define STATE_CUT          5
#define STATE_UNKNOWN      6

/* Analog readings corresponding to different sensor states */
struct input_ranges {
  int sensor_state;
  const String label;
  int range_bottom;
  int range_optimum;
  int range_top;
};
const input_ranges ranges[] {
  { STATE_SHORT,        "Shorted",         0,    0,  162 },  // 0
  { STATE_NORMAL,       "Normal",        163,  326,  409 },  // 1
  { STATE_TAMPER,       "Tamper",        410,  495,  551 },  // 2
  { STATE_ALARM,        "Alarm",         552,  609,  641 },  // 3
  { STATE_ALARM_TAMPER, "Alarm+Tamper",  642,  675,  848 },  // 4
  { STATE_CUT,          "Cut",           849, 1023, 1023 },  // 5
  { STATE_UNKNOWN,      "Unknown",      1024, 1024, 1024 },  // 6. Range beyond analog in because this is never read
};

/* Connected sensors */
struct sensor {
  const String label;
  byte analog_input;
  byte status_output;
  byte last_state;
};
sensor sensors[] {
  { "A", 0, 4, STATE_UNKNOWN },
  { "B", 1, 5, STATE_UNKNOWN },
  { "C", 2, 6, STATE_UNKNOWN },
  { "D", 3, 7, STATE_UNKNOWN },
};

/* Use 128x128 pixel OLED module ("OLED128") to display status */
const byte pin_cs     = 48;
const byte pin_dc     = 49;
const byte pin_reset  = 53;
OLED oled(pin_cs, pin_dc, pin_reset);
OLED_TextBox box(oled);
long lastActivityTime = 0;

byte server[] = { 192, 168, 1, 111 };   // MQTT server

char messageBuffer[100];
char topicBuffer[100];
char clientBuffer[20];

/**
 */
void callback(
  char* topic,
  byte* payload,
  int   length) {

  Serial.print("Received: ");
  for (int index = 0;  index < length;  index ++) {
    Serial.print(payload[index]);
  }
  Serial.println();
}

// Instantiate MQTT client
PubSubClient client(server, 1883, callback);


/**
 * Initial configuration
 */
void setup()
{
  if ( ENABLE_OLED == true )
  {
    oled.begin();
    oled.selectFont(SystemFont5x7);
    box.setForegroundColour(DODGERBLUE);

    box.println(F("     SuperHouse.TV      "));
    box.println(F("  Security Sensor v1.0  "));
    box.println(F("Getting MAC address:    "));
    box.print  (F("  "));
  }
  
  if (seconds_start_delay > 0)
  {
    Serial.print("Pausing ");
    Serial.print( seconds_start_delay );
    Serial.println(" seconds to let sensors settle");

    if ( ENABLE_OLED == true )
    {
      box.print(F("Pausing "));
      box.print(seconds_start_delay, DEC);
      box.println(" seconds");
      box.println("to let sensors settle");
    }

    delay(seconds_start_delay * 1000);
  }

  Wire.begin(); // Wake up I2C bus
  Serial.begin(9600);  // Use the serial port to report back readings

  if ( ENABLE_MAC_ADDRESS_ROM == true )
  {
    Serial.print(F("Getting MAC address from ROM: "));
    mac[0] = readRegister(0xFA);
    mac[1] = readRegister(0xFB);
    mac[2] = readRegister(0xFC);
    mac[3] = readRegister(0xFD);
    mac[4] = readRegister(0xFE);
    mac[5] = readRegister(0xFF);
  } else {
    Serial.print(F("Using static MAC address: "));
  }
  // Print the IP address
  char tmpBuf[17];
  sprintf(tmpBuf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(tmpBuf);

  if ( ENABLE_OLED == true )
  {
    box.println(tmpBuf);
  }

  // setup the Ethernet library to talk to the Wiznet board
  if ( ENABLE_DHCP == true )
  {
    Ethernet.begin(mac);      // Use DHCP
  } else {
    Ethernet.begin(mac, ip);  // Use static address defined above
  }

  // Print IP address:
  Serial.print(F("My IP: http://"));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    if ( thisByte < 3 )
    {
      Serial.print(".");
    }
  }
  Serial.println();

  if ( ENABLE_OLED == true )
  {
    box.println(F("My IP:"));
    box.print("  ");
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
      // print the value of each byte of the IP address:
      box.print(Ethernet.localIP()[thisByte], DEC);
      if ( thisByte < 3 )
      {
        box.print(".");
      }
    }
    box.println();
  }

  String clientString = "Arduino-" + String(Ethernet.localIP());
  clientString.toCharArray(clientBuffer, clientString.length() + 1);
  if (client.connect(clientBuffer)) {
    Serial.println("MQTT connected");
    client.publish("events", "Starting up");
    //client.subscribe("test/2");
  }

  Serial.println("Ready.");
}


/**
 * Main program loop
 */
void loop()
{
  if ( ENABLE_OLED == true )
  {
    if ( millis() > (lastActivityTime + (1000 * oled_timeout)))
    {
      oled.setDisplayOn(false);
    }
  }

  client.loop();
  byte i;
  for ( i = 0; i < 4; i++) {
    processSensor( i );
  }
}


/**
 */
void processSensor( byte sensorId )
{
  int sensorReading = analogRead( sensors[sensorId].analog_input );
  //Serial.print(sensorId, DEC);
  //Serial.print(": ");
  //Serial.println(sensorReading, DEC);

  byte sensorState = STATE_UNKNOWN;

  // This should check all entries in the states struct:
  if ( (sensorReading >= ranges[STATE_SHORT].range_bottom) && (sensorReading <= ranges[STATE_SHORT].range_top) ) {
    sensorState = STATE_SHORT;
  } else if ( (sensorReading >= ranges[STATE_NORMAL].range_bottom) && (sensorReading <= ranges[STATE_NORMAL].range_top) ) {
    sensorState = STATE_NORMAL;
  } else if ( (sensorReading >= ranges[STATE_TAMPER].range_bottom) && (sensorReading <= ranges[STATE_TAMPER].range_top) ) {
    sensorState = STATE_TAMPER;
  } else if ( (sensorReading >= ranges[STATE_ALARM].range_bottom) && (sensorReading <= ranges[STATE_ALARM].range_top) ) {
    sensorState = STATE_ALARM;
  } else if ( (sensorReading >= ranges[STATE_ALARM_TAMPER].range_bottom) && (sensorReading <= ranges[STATE_ALARM_TAMPER].range_top) ) {
    sensorState = STATE_ALARM_TAMPER;
  } else if ( (sensorReading >= ranges[STATE_CUT].range_bottom) && (sensorReading <= ranges[STATE_CUT].range_top) ) {
    sensorState = STATE_CUT;
  } else {
    sensorState = STATE_UNKNOWN;
  }

  // Compare to previous value
  if ( sensorState != sensors[sensorId].last_state )
  {
    // It changed!
    byte lastSensorState = sensors[sensorId].last_state;
    Serial.print("Sensor ");
    Serial.print(sensorId);
    Serial.print(" changed from ");
    Serial.print(ranges[lastSensorState].label);
    Serial.print(" to ");
    Serial.println(ranges[sensorState].label);
    sensors[sensorId].last_state = sensorState;
    String messageString = String(panelId) + '-' + sensors[sensorId].label + '-' + String(sensorState);

    Serial.println(messageString);

    messageString.toCharArray(messageBuffer, messageString.length() + 1);
    String topicString = "sensors";
    topicString.toCharArray(topicBuffer, topicString.length() + 1);
    client.publish("sensors", messageBuffer);

    if ( ENABLE_OLED == true )
    {
      box.setForegroundColour(LIMEGREEN);
      box.print(F("Event: "));
      box.println(messageString);
    }
  }
}



/**
 * Required to read the MAC address ROM
 */
byte readRegister(byte r)
{
  unsigned char v;
  Wire.beginTransmission(MAC_I2C_ADDRESS);
  Wire.write(r);  // Register to read
  Wire.endTransmission();

  Wire.requestFrom(MAC_I2C_ADDRESS, 1); // Read a byte
  while (!Wire.available())
  {
    // Wait
  }
  v = Wire.read();
  return v;
}
