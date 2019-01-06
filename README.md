## NRF24 debugging tool

Simple sketch used for developing nRF24 radio link applications. 

### Features
- serial command interface (USB virtual vom port)
- send arbitrary data to any pipe addresses
- listen mode
- supports auto-ACK with payload


### Usage
#### Send message to device (w):
```
    w <device_pipeline_addr> <data0> ... <dataN>
```


#### Listen mode (l)
```
    l 
```

