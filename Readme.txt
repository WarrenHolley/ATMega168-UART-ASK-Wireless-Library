This library is intended for ATMega*8a devices to communicate using UART over simple
wireless RF 'ASK' (Amplitude Shift Keying) modules.

Communicates over a Simplex Model, with the receiver having a unique ID value.
The library can communicate to up to 32 different-IDd receivers.

The library also implements corruption resistance, with both bytewise checksumming,
 Repetition Code voting, and packet numbering to increase odds of successful delivery.



This code was published by Warren Holley, March 10, 2018
All data is published under the MIT License. Read the License doc for more details.


Notes on Testing of Reliability:
Current implementation appears to lose ~1-2% of total packets.
Testing was done over the range of 5cm-10m, for times up to 20 minutes.
Only one truly corrupt packet was seen during the entirety of testing.
All others were dropped or lost.
Plans for modifications are underway to increase reliablility. 
