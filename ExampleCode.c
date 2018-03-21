#include <stdint.h>
#include "RFTransceiver.h"

int main (void)
{	
	//Select one, as they are both blocking.
	//debugTransmit(1); //Continuously Transmit to device with ID 1
  	debugReceive(); //Continuously Receive and retransmit depacketized value direceted to all devices.

	return 1; //Exit
}
