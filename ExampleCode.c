#include <stdint.h>
#include "RFTransceiver.h"

int main (void)
{
	InitUSART(1000, true, true); //Initizalize UART
	
	//Select one, as they are both blocking.
	//debugTransmit(); 
  	debugReceive();

	return 1; //Exit
}
