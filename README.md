## NRF24 debugging tool

Simple sketch used for developing nRF24 radio link applications. 

### Features
- serial command interface (USB virtual vom port)
- send arbitrary data to any pipe addresses
- listen mode (not yet supported)
- supports auto-ACK with payload


### Usage
#### Send message to device (w):
```
    w <device_pipeline_addr> <data0> ... <dataN>
```
- device_pipeline_addr: hex encoded string, 5 bytes (example: 314e4f4d57)
- dataN: hex encoded byte (example: 5C)


#### Listen mode (l)
```
    l 
```
(NOT YET IMPLEMENTED)

