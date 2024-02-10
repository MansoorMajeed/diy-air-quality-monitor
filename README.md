# DIY Indoor Air Quality Monitoring

Note: I am not an expert in embedded systems and microcontrollers, so please do not take anything
I did at face value, but this works for me.

## The Goal

I wanted to monitor my indoor Air Quality (Co2 and PM values) and plot the data on Grafana dashboard

## Components

- Temperature and humidity : DHT11 - This is not very good, but good enough for my use case
- CO2 : MHZ19-B
- PM (Particulate Matter) : PMS5003
- Esp32


## ESP-32 Code

The source code is [HERE](./esp32-code/)

### Setup

1. Update the WiFi SSID and Password
2. Update the MQTT broker IP, username, password and the topic name
3. Update the serial ports to appropriate

### Libraries Required

- https://github.com/fu-hsi/PMS
- https://github.com/plapointe6/EspMQTTClient
- https://github.com/WifWaf/MH-Z19