# Smart-lock-IoT
This repository contains the source code for a secure door lock system using an ESP8266 and an Arduino. The system incorporates RFID tag scanning, password input, motion detection, and remote control functionalities. Below is an overview of the key functionalities provided by this codebase:

Arduino Code (door_lock_arduino.ino):

Utilizes an MFRC522 RFID module to read RFID tags for authentication.
Interacts with a 4x3 keypad to input passwords for access.
Controls a servo motor to physically open and close the door.
Interfaces with a LiquidCrystal display to provide user feedback.
Emits sound using a buzzer to indicate successful accesses or alerts.
Utilizes a Passive Infrared (PIR) motion sensor to detect motion near the door.
Communicates with an ESP8266 via SoftwareSerial for remote control.
ESP8266 Code (door_lock_esp8266.ino):

Connects to a designated Wi-Fi network to establish internet connectivity.
Retrieves accurate time from NTP servers using the NTPClient library.
Subscribes to MQTT topics for remote commands, including door status, buzzer control, and motion sensor status.
Publishes MQTT messages to inform subscribers about door status changes, buzzer activation, and motion sensor detections.
Sends data to ThingSpeak cloud platform to monitor door status and motion sensor events.
Offers IFTTT integration to send alerts and notifications for specific events, such as failed access attempts and motion detections.
Usage:

The Arduino manages the door locking and authentication process using RFID tags and keypad inputs.
The ESP8266 handles internet connectivity, time synchronization, MQTT communication, and cloud data storage.
The system can be remotely controlled through MQTT commands, enabling users to open the door, activate/deactivate the buzzer, and monitor sensor status.
Failed access attempts are tracked and can trigger alerts through IFTTT.
Contributions:
Contributions to this repository are welcome. Feel free to add enhancements, improvements, or new features to make the door lock system even more secure and user-friendly.

Note:
Ensure that proper wiring and connections are made according to the circuit diagrams provided in the documentation. Additionally, customize network credentials and MQTT topics according to your environment.
