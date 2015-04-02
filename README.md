# Security Sensor (4-channel) to MQTT

Developed by [SuperHouse Automation Pty Ltd](http://www.superhouse.tv/)  

## Description

Reads the state of up to 4 security sensors using End-of-Line resistors connected to analog inputs, and publishes state changes to an MQTT server.  

Can optionally display events locally using a Freetronics 128x128 colour OLED module.

See more at http://www.superhouse.tv/

## Required Libraries

1. [PubSubClient.h](https://github.com/knolleary/pubsubclient)

2. [FTOLED.h](https://github.com/freetronics/FTOLED) NOTE: Only required when using the OLED display option.

## Installation

1. Change directory to Arduino's main directory
2. mkdir SecuritySensor4ToMQTT
3. cd SecuritySensor4ToMQTT
4. git clone https://github.com/superhouse/SecuritySensor4ToMQTT.git .
5. Start Arduino IDE.
6. Go to File--->Sketchbook--->SecuritySensor4ToMQTT

## License
    Copyright (C) 2015 SuperHouse Automation Pty Ltd

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
