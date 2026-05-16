#include "uart.h"
using namespace std;

    // when object intilaize the port value to -1
    uartPrt::uartPrt()
    {
    fd = -1; 
    }
    // closing port when the object destructor
    uartPrt::~uartPrt()
    {
    if(fd != -1)
        {
        ::close(fd);
        }
    }

    bool uartPrt::open(char const *devname)
    {
        // check if port is open
        if(fd != -1)
        {
        ::close(fd);
        }
        // O_RDWR opens the uart port in 
        // read and write mode, look into O_ASYNC later
        fd = ::open(devname, O_RDWR);
        
        // port failed to open return error
        if(fd < 0)
        {
            perror(devname);
            return false;
        }
        
        // termios used to set the terminals settings 
        // ex: inputs, outputs, baudrate
        struct termios tio;
        
        // used to get the current terminal setting
        ::tcgetattr(fd, &tio);
        // sets the terminal settings to "raw" mode, ie no input and output processing
        cfmakeraw(&tio);
        
        // flow control on both input and output, 
        // ctrl-s and ctrl-q will no longer pause or resume the terminal
        tio.c_iflag &= ~(IXON | IXOFF);
        // setting the block time ie the time 
        // the code waits for an input to be indefinite
        tio.c_cc[VTIME] = 0;
        // setting the minimum number of bytes need to arrive to end blocking, 
        // 0 meaning if it recieives any input it will stop blocking
        tio.c_cc[VMIN] = 0;
        
        // actually setting the terminal with the new settings 
        // returns an int cause its a c function and booleans don't exist
        int err = ::tcsetattr(fd, TCSAFLUSH, &tio);
        
        // checks it setting the attributes failed
        if(err != 0) 
        {
            perror(devname);
            ::close(fd);
            fd = -1;
            return false;
        
        }
        return true;
    }

    unsigned char* uartPrt::package(unsigned char msg[], int& msgSize, const uint8_t& header)
    {
        // to return the new packet it needs to be allocated in memory
        unsigned char* packet = new unsigned char[msgSize+3];
        // shifting the array by one, to the right into the new array
        for(int i = msgSize-1; i >= 0; i--)
        {
            packet[i+1] = msg[i];
        }
        // assigning the header to the first index
        memcpy(&packet[0],&header,1);
        msgSize++;
        // adding the packet sequence and
        // doing incrementing it for next time
        memcpy(&packet[msgSize],&packetSequence,1);
        packetSequence++;
        msgSize++;

        // addition checksum, its the value of all bytes add together, 
        // it is designed to overflow as that the same as sum % 256
        uint8_t checksum = 0;

        cout << "packet: " << packetSequence <<" : ";
        for(int i = 0; i < msgSize; i++)
        {
            cout << packet[i];
            checksum += packet[i];
        }
        cout << checksum << endl;
        // assigning checksum to the final position of the msg
        memcpy(&packet[msgSize],&checksum,1);
        // increasing msgsize for the for loop later.
        msgSize++;

        return packet;
    }

    void uartPrt::send(chrono::time_point<chrono::high_resolution_clock>& t, unsigned char msg[], int msgSize, const uint8_t& header)
    {
        // msgSize is 21 max because one byte for header,
        // one byte for checksum and one byte for sequence 
        // are necessary
        if(msgSize>21)
        {
            cout << "message is too big" << endl;
            return;
        }

        // creates the checksum byte, and inserts both 
        // the header byte and sequence byte along with the new
        // byte checksum into the array
        unsigned char* packet = package(msg,msgSize,header);
        
        //matches the uart send buffer
        unsigned char txBuffer[msgSize];
        //where we write our message
        unsigned char *p_txBuffer;
        
        // intializing p_txBuffer to the start of txBuffer
        p_txBuffer = &txBuffer[0];
        
        
        for (int i = 0; i < msgSize; i++)
        {
            // writing our message into our buffer
            *p_txBuffer++ = packet[i];
        
        }
        
        if(fd != -1)
        {
            // writes it character by character into uart buffer and sends it  
            int count = write(fd, &txBuffer[0], (p_txBuffer - &txBuffer[0]));
            // for latency testing
            t = chrono::high_resolution_clock::now();
            
            if(count < 0) 
            {
                // error message
                // perror(&static_cast<char>(count))
                cout << "it shit itself" << endl;
            }
        }
        // removing the packet from memory even if everything fails
        delete[] packet;
    }

    void uartPrt::send(unsigned char msg[], int msgSize, const uint8_t& header)
    {
        // msgSize is 21 max because one byte for header,
        // one byte for checksum and one byte for sequence 
        // are necessary
        if(msgSize>21)
        {
            cout << "message is too big" << endl;
            return;
        }

        // creates the checksum byte, and inserts both 
        // the header byte and sequence byte along with the new
        // byte checksum into the array
        unsigned char* packet = package(msg,msgSize,header);
        
        //matches the uart send buffer
        unsigned char txBuffer[msgSize];
        //where we write our message
        unsigned char *p_txBuffer;
        
        // intializing p_txBuffer to the start of txBuffer
        p_txBuffer = &txBuffer[0];
        
        cout << "packet before send: ";
        for (int i = 0; i < msgSize; i++)
        {
            cout << packet[i];
            // writing our message into our buffer
            *p_txBuffer++ = packet[i];
        
        }
        cout << endl;
        
        if(fd != -1)
        {
            // writes it character by character into uart buffer and sends it  
            int count = write(fd, &txBuffer[0], (p_txBuffer - &txBuffer[0]));

            cout << "count: " << count << endl;
            
            if(count < 0) 
            {
                // error message
                // perror(&static_cast<char>(count))
                cout << "it shit itself" << endl;
            }
        }
        // removing the packet from memory even if everything fails
        delete[] packet;
    }

    void uartPrt::receive(chrono::time_point<chrono::high_resolution_clock>& t, unsigned char serial_message[])
    {
        // size of the uart receive buffer
        int size = 24;
        // place to write the recieved data
        unsigned char rxBuffer[size];
        //for checking if we have recieved anything
        bool pickup = false;
        // length of the recieved message
        int rxLength = 0;
        
        // empty out all queued and unread packages
        tcflush(fd, TCIOFLUSH);
        
        // intializing the actual output array to be empty spaces
        for(int ii =0; ii<size; ii++)
        {
        serial_message[ii] = ' ';
        }
        
        // while pickup is false and the port exists
        while(!pickup && fd != -1)
        {
            // read the buffer, the static_cast is to match 
            // reads declleration: ssize_t read(int, void*, size_t);
            // rxLength is the amount of bytes read
            rxLength = read(fd, static_cast<void*>(rxBuffer), size);
            if(rxLength >=0)
            {
                // easiest and most efficient method 
                // to get the data out of rxBuffer
                memcpy(serial_message, rxBuffer,size);
                // for latency testing
                t = chrono::high_resolution_clock::now();
                
                cout << chrono::duration_cast<chrono::milliseconds>(t.time_since_epoch()).count() << endl;
                // if we read size amount of bytes 
                // return exit the while loop
                if(rxLength == size)
                {
                    pickup = true;
                }
            }
        
        }
        return;
    }

    void uartPrt::receive(unsigned char serial_message[])
    {
        // size of the uart receive buffer
        int size = 24;
        // place to write the recieved data
        unsigned char rxBuffer[size];
        //for checking if we have recieved anything
        bool pickup = false;
        // length of the recieved message
        int rxLength = 0;
        
        // empty out all queued and unread packages
        tcflush(fd, TCIOFLUSH);
        
        // intializing the actual output array to be empty spaces
        for(int ii =0; ii<size; ii++)
        {
        serial_message[ii] = ' ';
        }
        
        // while pickup is false and the port exists
        while(!pickup && fd != -1)
        {
            // read the buffer, the static_cast is to match 
            // reads declleration: ssize_t read(int, void*, size_t);
            // rxLength is the amount of bytes read
            rxLength = read(fd, static_cast<void*>(rxBuffer), size);
            if(rxLength >=0)
            {
                // easiest and most efficient method 
                // to get the data out of rxBuffer
                memcpy(serial_message, rxBuffer,size);

                // if we read size amount of bytes 
                // return exit the while loop
                if(rxLength == size)
                {
                    pickup = true;
                }
            }
        
        }
        return;
    }
        

// testing functions
void testLoop(uartPrt& uart, unsigned char m[],int& i)
{
    m[5] = i;
    chrono::time_point<chrono::high_resolution_clock> send;
    uart.send(send, m,22,0xFF);
    i++;
    if( i >= 256)
    {
        i =0;
    }

    chrono::duration<long long, std::ratio<1,1000>> send_dur = chrono::duration_cast<chrono::milliseconds>(send.time_since_epoch());

    long long send_dur_count = send_dur.count();
    memcpy(m,&send_dur_count,4);

    uart.send(send, m,22,0xFF);

    uint32_t send_int;  
    memcpy(&send_int,&send_dur_count,4);



    cout << "send time:    " << send_int  << endl << "i:" << i << endl; 


}

void testSetup(uartPrt& uart,unsigned char input[], unsigned char m[])
{

    
    chrono::time_point<chrono::high_resolution_clock> send = chrono::high_resolution_clock::now();
    
    chrono::duration<long long, std::ratio<1,1000>> send_dur = chrono::duration_cast<chrono::milliseconds>(send.time_since_epoch());
    
    long long send_dur_count = send_dur.count();
    cout << "sending" << endl;
    memcpy(m,&send_dur_count,4);
    uart.send(send, m, 22,0xFF);
    cout << "sent" << endl;

    
    cout << "recieving" << endl;
    chrono::time_point<chrono::high_resolution_clock> recieve;
    uart.receive(recieve,input);
    cout << "recieved" << endl;

    chrono::duration<long long, std::ratio<1,1000>> receive_dur = chrono::duration_cast<chrono::milliseconds>(recieve.time_since_epoch());

    long long receive_dur_count = receive_dur.count();

    memcpy(m,&receive_dur_count,4);
    uart.send(send, m,22,0xFF);
    unsigned long recieve_int;
    unsigned long send_int;  
    memcpy(&recieve_int,&receive_dur_count,4);
    memcpy(&send_int,&send_dur_count,4);



    cout << "recieve time: "  << receive_dur_count << endl << "send time:    " << send_dur_count << endl; 

}