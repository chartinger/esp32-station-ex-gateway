# esp32-station-ex-gateway

Firmware for an ESP32 (D1 Mini Format) to

* connect CommandStation-EX to a MQTT broker
* act as a Websocket server

### Notes

This is a simple serial to mqtt/websocket bridge. The ESP32 has to be connected to your CommandStationEX via serial port. It **replaces** the Wifi shield method.
