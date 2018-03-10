#include <stdlib.h> //For Memory Allocation

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <util/delay.h>

#include "RFTransceiver.h"

typedef int bool; //Boolean implementation.
#define true 1
#define false 0

//Setup Config:
#define WIRELESS_BAUD 1000 	//In bits/s
#define CHIP_ID 0 		//Receiver ID value [0-31]

// Initialize USART in UART format.
// Implemented and tested for ATMega168a.
// TODO: Test with remaining ATMega*8a line.
void InitUSART (uint16_t dataBaudRate, bool transmitBool, bool receiveBool) {
	//Calclate USART Baud Rate Register value.
	uint16_t UBRR_rate = F_CPU/(16*dataBaudRate) - 1;
	//Set Baud Rate
	UBRR0H = (unsigned char) (UBRR_rate >> 8); 
	UBRR0L = (unsigned char) UBRR_rate;
	//Enable Transmitter and/or Receiver
	UCSR0B = (transmitBool<<TXEN0)|(receiveBool<<RXEN0);
	// Frame Format: 8 bits of data, 1 stop bit
	UCSR0C = (1<<UCSZ00) | (1 << UCSZ01);

	sei(); //USART requies interrupts to function.

	//TODO. Upgrade for setable frame format range.
	//Defaulting to 8/1 as that is what's needed for the wireless component of the major
	// Climate Control system that this firmware was built for.
}

//Transmits the raw byte given as an argument.
//Is blocking until the output buffer is clear.
void TransmitByte (uint8_t dataByte ) {
	while (!( UCSR0A & (1<<UDRE0)))
		; //Delay until data buffer is empty
	//Push data to transmit buffer
	UDR0 = dataByte;
}

// Enpacket, transmit 3 4-byte packets.
// Packet Format: [Timing][ID(5),Packet(3)][Data][Checksum]
// [Checksum] = [ID,Packet] XOR [Data]
void SecTransmitPacket(uint8_t ID, uint8_t inputByte) {
	
	uint8_t ID_PacketByte = (ID<<3) | 0; //0 for packet# init.

	for (int i = 0; i < 3; i++){ //Transmit 3 packets
		TransmitByte(0b10101010);
		TransmitByte(ID_PacketByte+i); //=0,1,2
		TransmitByte(inputByte);
		TransmitByte(inputByte^(ID_PacketByte+i));
	}
}

// Returns the UART Received byte when available.
// Blocking until something is received.
uint8_t ReceiveByte() {
	while (! (UCSR0A & (1 << RXC0)))
		; //Wait for Receive Complete Flag to clear
	return UDR0;
}

// Receives, verifies, and truncates a received byte.
// Returns a 3-byte uint8 block: {ID,Packet#,Data}
// MALLOC! Be sure to free pointer after use.
uint8_t* SecReceiveDataPacket(){
	//Receipt Format: [Timing][ID(5),Packet(3)][Data][Checksum]
	uint8_t buffer[4];
	for (int i = 0; i < 4; i++)
		buffer[i] = ReceiveByte(); //Init: Fill buffer.
	
	//If timing buffer incorrect, or checksum value incorrect, shift, fill buffer.
	while (buffer[0] != 0b10101010  || (buffer[1]^buffer[2]) != buffer[3]){ 
	 	for (int i = 0; i < 3; i++)  		// Shift buffer. Could improve perf by using
			buffer[i] = buffer[i+1];  	//  modulo-circle array.
		buffer[3] = ReceiveByte();
	}
	//At this point timing and checksum values accurate.
	uint8_t* returnBlock = malloc(3);
	returnBlock[0] = buffer[1] >> 3;  //ID Value
	returnBlock[1] = buffer[1] & 0x7; //Packet#
	returnBlock[2] = buffer[2];	  //Data

	return returnBlock;
}

// Monitors receiver for datagrams intended for the device's ID.
// Attempts to receive the 3-block datagram, parse their values, and return the majority value.
// Uses basic Repetition Code for voting.
uint8_t ReceivePersonalPacket(uint8_t ID){
	
	bool didVote[3]; 	//Two arrays, boolean if there was a viable packet, and if
	uint8_t dataVotes[3];	// what the packets value was.
	for (int i = 0; i < 3; i++){
		didVote[i] = false; //Clear arrays.
		dataVotes[i] = 0;
	}

	//Init: Wait for data packet for this individual device.
	uint8_t* recData = SecReceiveDataPacket();
	while(recData[0] != ID){
		free(recData);
		recData = SecReceiveDataPacket();
	}
	
	//Vote Collation: if packet received, vote on it's value.
	// recData[1] is the packet#. As no packet repetition, are guarenteed to be in order.
	// Lost or dropped packets are ignored.
	if (recData[1] == 0){
		dataVotes[0] = recData[2];
		didVote[0] = true;
		//Fetch next block
		free(recData);
		recData = SecReceiveDataPacket();
	}
	if (recData[1] == 1 && recData[0] == ID){
		dataVotes[1] = recData[2];
		didVote[1] = true;
		free(recData);
		recData = SecReceiveDataPacket();
	}
	if (recData[1] == 2 && recData[0] == ID){
		dataVotes[2] = recData[2];
		didVote[2] = true;
	}// End Vote Collation.
	free(recData);
	
	//As transmissions are 3*4 bytes transmitted immediately, if ID changes, then 
	//following blocks were lost. Edge case where two transmissions A,B, both received.
	// If A2,A3,B1 dropped, would consider A1,B2 single data block.

	//Possible fix: more than 3-count packet counting, with offset check. 
	// Eg, Packet 3 not in same package as Packet 4. Disregard 3.
	
	
	//If three packets, return more common value.
	//If only one or two packets, assume the first is correct. 
	// (TODO: Maybe upgrade to 5+-block datagram, -require- at least 2 votes)
	// If no common value in 3, return first vote.

	//Edge Case: Corruption to values that make no sense.
	// Eg: 0x05 -> 0x85. Fix: Bound, or picking minimum of available values,
	//  as don't want to run mains power continuously.
	// Could also implement some FECC, but would require a hell of a lot long datagrams.
	// Maybe Manchester Encoding? Would fix a few issues, but only double size.
	//  Need to test if the decoding and FECC algs would fit in such a tiny device.

	
	if (didVote[0]){
		if (didVote[1] && didVote[2]){
			if (dataVotes[0] == dataVotes[1])
				return dataVotes[0];	   //If ABC, A=B,Ret A;
			if (dataVotes[1] == dataVotes[2])
				return dataVotes[1];	   //If ABC, B=C,Ret C;
		}
		return dataVotes[0];//AB||AC,ret A.
	}
	else if (didVote[1])//B||BC, ret B.
		return dataVotes[1];
	else // Only C, ret C.
		return dataVotes[2];	
}


// Steps through Decimal notation of the uint8 value, printing individual characters.
void printUInt (uint8_t value){
	uint16_t divisor=10;
	uint8_t outputValue;
	while (value/divisor > 0)
		divisor*=10;
	for (divisor/=10; divisor >= 1; divisor/=10) {
		outputValue='0'+(value/divisor)%10;
		TransmitByte(outputValue);
	}
	TransmitByte('\r');
	TransmitByte('\n');
}


//Debug example transmitter.
//Counts & Transmits 0->99. Repeats
void debugTransmit(){
	for (uint8_t i = 0; i < 100; i = (i+1)%100){
		SecTransmitPacket(CHIP_ID, i);
		TransmitByte('\r'); //For some terminals. Seperates lines so that it's not all a block.
		TransmitByte('\n'); //Is happily discarded as noise by the Receiving side.
		_delay_ms(1000); //Increment & print once per second (or about, with IO delays)
	}
}	
// Debug receiving script.
// Transmits character string values of received uint 
void debugReceive(){
	while(1)
		printUInt(ReceivePersonalPacket(CHIP_ID));
}
