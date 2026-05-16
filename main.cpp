#include "uart.h"
 
 int main()
 {
 
 	unsigned char b[21] = {'k','i','l','l',' ','y','o','u','r',' ','s','e','l','f','k','i','l','l',' ','y','o'};

 	uartPrt uart;
 	

 	uart.open("/dev/ttyTHS1");
 	
	uart.send(b,21,49);

 	/*unsigned char m[256];
 	int i = 0; 
 	
 	testSetup(uart,b,m);
 	
 	for(;;){
		usleep(16667/2);
		testLoop(uart,m,i);
 	}*/
 	
 	return 0;
 	
 	
 }
 