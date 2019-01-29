#include <stdint.h>
#include <stdio.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <SerialCommands.h>

RF24 radio(9,10);   /* CE pin 9, CSN Pin 10 */

uint8_t debug = 0;

/* code snipped for hex character to integer conversion functions (hexCharToBin(), hexStrToULL()) borrowed from:
http://forum.arduino.cc/index.php?topic=233813.0   ; posted by user westfw.
*/
char hexCharToBin(char c) {
 if (isdigit(c)) {  // 0 - 9
   return c - '0';
 } else if (isxdigit(c)) { // A-F, a-f
   return (c & 0xF) + 9;
 }
 return -1;
}

unsigned long long hexStrToULL(char * string)
{
  unsigned long long x =0;
  char c;
  do {
    c =hexCharToBin(*string++);
    if (c < 0)
      break;
    x = (x << 4) | c;
 } while (1);
 return x;
}





/*!
* \brief send data message via NRF24L01
* \param addr - pipeline address of target device
* \param data - pointer to buffer for payload data
* \param len - length of payload data buffer
* \param resp - pointer to buffer for ack payload data (min 32 bytes)
*
* \retval -1 - timeout while waiting for response
* \retval -2 - error writing data to NRF24L01 module
* \retval all values >=0 - length of ACK payload (response from device)
* 
*/
int8_t send_data(uint64_t addr, uint8_t *data, uint8_t len, uint8_t* resp){
    uint8_t dat[len];
    bool ok;
    uint8_t i;
    bool timeout = false;
    uint8_t response[32] = {0}; 
    uint8_t retval = 0;
    uint8_t response_len;
    for(i=0;i<len;i++){
        dat[i] = data[i];  // copy data buffer  
    }


    radio.stopListening();
    radio.openWritingPipe(addr);
        
    ok = radio.write( dat, len );

    if(!ok){
        return(-2);
    }
    unsigned long started_waiting_at = millis();
    
    while ( !radio.isAckPayloadAvailable() && ! timeout )
    if ((millis() - started_waiting_at) > 500 )
    {
       return(-1);
    }
      
    response_len = radio.getDynamicPayloadSize();
    radio.read(response, response_len);
    retval = response_len;
    for(i=0;i<response_len;i++){
        resp[i] = response[i];
    }    
    return(retval);
}

/* command parameters in HEX format
* example: send bytes [1, 10, 255] to address 0xDEADB
    w DEADB 1 a ff

    maximum of 32 bytes allowed
*/
void cmd_send_message(SerialCommands* sender){
    char *arg; 
    uint64_t pipeline;    
    uint8_t payload[32];
    uint8_t payload_count = 0;
    uint8_t response[32];
    int8_t response_len= 0;
    
    arg = sender->Next();
    
    if (arg != NULL)
    {
        pipeline = hexStrToULL(arg)&0xFFFFFFFFFF;    // pipeline limited to max 5 bytes
    }
    else
    {
        sender->GetSerial()->println(0x83, HEX); 
        return;
    }

    arg = sender->Next(); 

    while (arg != NULL) 
    {
        payload[payload_count] = (uint8_t)strtoul(arg, NULL, 16);
        payload_count++;
        arg = sender->Next(); 
    } 

    if((payload_count == 0) || (payload_count > 32))
    {
      sender->GetSerial()->println(0x84, HEX); // illegal payload size
      return;
    }
    // send data
    response_len = send_data(pipeline, payload, payload_count, response);

    // print error-code
    
    if(response_len == -1) sender->GetSerial()->println(0x81, HEX);   // timeout waiting for response
    if(response_len == -2) sender->GetSerial()->println(0x82, HEX);   // error writing to device
    if(response_len >= 0) sender->GetSerial()->print(0x0, HEX);     // OK
    
    
    // print payload
    if(response_len > 0)
    {
        sender->GetSerial()->print(" ");
        for(int i=0; i<response_len; i++)
        {
            sender->GetSerial()->print(response[i], HEX);            
            
            if(i<response_len) sender->GetSerial()->print(" ");
        }
        sender->GetSerial()->println("");
    }
}

/*!
* \brief enter RX mode
* \param rx_pipe_addr - address of RX pipeline
* 
* \details Enters RX mode, prints any received message over serial port
*/
void cmd_listen(SerialCommands* sender) {
    char *arg;
    arg = sender->Next();

    radio.startListening();

    /* not yet implemented */
}

/*!
* \brief Ser debug mode
* \param debugmode - 1: enable / 0 disable 
* 
* \details sets debug mode, which enables additional output
*/
void cmd_debug(SerialCommands* sender) {
    char *arg;
    arg = sender->Next();
    uint8_t debugmode = 0;

    if (arg != NULL)
    {
        debugmode = (uint8_t)hexStrToULL(arg);

        if(debugmode > 0)
        {
          debug = 1;
        }
        else
        {
          debug = 0;
        }
    }
}


//This is the default handler, iscalled when no other command matches. 
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
	sender->GetSerial()->print("what? [");
	sender->GetSerial()->print(cmd);
	sender->GetSerial()->println("]");
}

// test command for various purposes
void cmd_test(SerialCommands* sender)
{
	sender->GetSerial()->println("test successful");
}



char serial_command_buffer_[128];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

SerialCommand cmd_send_message_("w",  cmd_send_message);     
SerialCommand cmd_listen_("l",  cmd_listen);     
SerialCommand cmd_test_("t",  cmd_test);     
SerialCommand cmd_debug_("d",  cmd_debug);

void setup() {
  
  Serial.begin(115200);
  radio.begin();
  // Setup callbacks for SerialCommand commands
  serial_commands_.AddCommand(&cmd_send_message_);
  serial_commands_.AddCommand(&cmd_listen_);
  serial_commands_.AddCommand(&cmd_test_);
  serial_commands_.AddCommand(&cmd_debug_);
    
  serial_commands_.SetDefaultHandler(cmd_unrecognized);  // Handler for command that isn't matched  (says "What?") 
  Serial.println("ready");

  /* configure nRF24L01 module */
  radio.setRetries(15,15);
  radio.enableAckPayload();
  radio.setChannel(40);
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setCRCLength(RF24_CRC_16);
  
  radio.stopListening();
}

void loop() {
  serial_commands_.ReadSerial();     // We don't do much, just process serial commands
}
