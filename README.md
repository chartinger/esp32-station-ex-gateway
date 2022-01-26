# esp32-station-ex-gateway

Firmware for an ESP32 (D1 Mini Format) to

* connect CommandStation-EX to an MQTT broker
* act as a Websocket server

### Notes

This is a simple serial to mqtt/websocket bridge. The ESP32 has to be connected to your CommandStationEX via serial port. It **replaces** the Wifi shield method.

## Custom platformio.ini

For custom platforms simply create a `plaformio.local.ini` with your overrides

### Example: Switching to OTA for esp8266

Create `plaformio.local.ini` with:

```
[env:esp8266-ota-01]
extends = env:esp8266
upload_protocol = espota
upload_port = your-mdns-and-ota-hostname
upload_flags = --auth=your-ota-password
```
