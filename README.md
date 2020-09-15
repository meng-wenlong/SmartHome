# SmartHome

A smart home system based on Arduino and OneNET. 

It can monitor the temperature, humidity and light intensity of the room locally and in the cloud. And it use a fan, a humidifier, and a led to adjust the room environment with negative feedback. The thresholds of temperature and humidity can be changed in the cloud. 

### Requirement

- Arduino UNO
- DHT11
- ESP8266
- Light sensor
- OLED screen
- led
- 2N7000 × 2
- Motor
- Atomized film

### Hardware Module

The wring between each module is shown in the figure below:

![Hardware](./pic/Hardware.png)

(Since our atomizer is powered by MicroUSB, here we use MicroUSB to represent the atomizer.)

### Get Start

1. You should first build an EDP-based product on OneNET. And add five data streams: Temperature, Humidity, Illumination, Set_Temperature, Set_Humidity.
2. Put the dht11 folder in your `/Arduino/libraries/` directory.
3. Open `runSmartHome/runSmartHome.ino` then modify API-Key and DeviceID. When connecting to WiFi for the first time, you need to modify the WiFi connection to your own WiFi name and password.
4. After making sure the hardware is connected correctly, you can upload the program!

### Phenomenon

The temperature, humidity and light intensity will be displayed on the OLED screen. When the temperature is higher than the threshold, the motor will drive the fan to rotate. When the humidity is lower than the threshold, the atomizer will work.