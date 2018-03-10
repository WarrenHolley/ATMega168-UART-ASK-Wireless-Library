typedef int bool; //Boolean implementation.
#define true 1
#define false 0

void InitUSART (uint16_t dataBaudRate, bool transmitBool, bool receiveBool); //Initializer.

void TransmitByte (uint8_t dataByte ); //Transmits the given byte over UART.
void SecTransmitPacket(uint8_t ID, uint8_t inputByte); //Transmit 3 4-byte packets.
//Encodes the data into a packet with timing, information and checksum bytes.

uint8_t ReceiveByte(); //Returns a received byte from UART.
uint8_t* SecReceiveDataPacket(); //Parses above, returns 3 Byte array of data. (ID,Packet#,Data)
uint8_t  ReceivePersonalPacket(uint8_t ID); //Parses above, processes, returns 'Data' byte.

void printDouble(double value); //Translates float to char*, then transmits raw data.
void printUInt (uint8_t value); //Translates uint to char*, then transmits raw data.

void debugTransmit(); //Debug/Testing transmitter code. Transmits packetized count from 0-99.
void debugReceive();  //Debug/Testing receiver code. Receives and retransmits (to terminal) the uint
			//string of received data.



