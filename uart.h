#include <termios.h> // terminal settings 
#include <unistd.h> // open(), close(), read() and write()
#include <sys/fcntl.h> // flags 
#include <iostream> // and debugging/error messages
#include <cstring> // memcpy
#include <chrono> // latency testing
using namespace std;

class uartPrt
{
    // port
    int fd;
    // minimum input
    const int VMINX = 1;
    // sequence of the packet
    uint8_t packetSequence = 1;
 	
    public:
    uartPrt();

    ~uartPrt();

    bool open(char const *devname);

    unsigned char* package(unsigned char msg[], int& msgSize, const uint8_t& header);

    void send(chrono::time_point<chrono::high_resolution_clock>& t, unsigned char msg[], int msgSize, const uint8_t& header);

    void send(unsigned char msg[], int msgSize, const uint8_t& header);

    void receive(chrono::time_point<chrono::high_resolution_clock>& t, unsigned char serial_message[]);

    void receive(unsigned char serial_message[]);
        
};

void testLoop(uartPrt& uart, unsigned char m[], int& i);

void testSetup(uartPrt& uart, unsigned char input[], unsigned char m[]);
 