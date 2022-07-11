#include "mbed.h"
#include <chrono>
#include <cstdio>
#include <functional>

//Inputs
InterruptIn button_1(D2);
InterruptIn button_2(D3);

// Digital Outputs
DigitalOut redLED(D6); 
DigitalOut greenLED(D7,1);
DigitalOut onBoardLED(LED1);     

// PWM
PwmOut buzz(D9);

// Create a UnbufferedSerial object with a default baud rate.
UnbufferedSerial serial_port(USBTX, USBRX);

//This will hold the single byte that we read from the serial port
volatile char p = 0;

//Serial Receive ISR
void on_rx_interrupt()
{
    // The variable p is shared, so we MUST use a critical section lock to avoid races
    CriticalSectionLock lock;
    serial_port.read((void*)&p, 1);
    // Also note that:
    // (i) serial_port is not referenced anywhere else
    // (ii) only ONE serial port is being used
    // So we work on the basis that read can be used.
    // UnbufferedSerial is not strictly documented as interrupt safe https://os.mbed.com/docs/mbed-os/latest/apis/thread-safety.html
}

// ISR for the switch
void button1Pressed() {

}
void button1Released() {
 
}

int main()
{
    // Set up the serial port to 9600,8,N,1
    serial_port.baud(9600);
    serial_port.format(8, SerialBase::None, 1);
    
    // Register a callback to process a Rx (receive) interrupt.
    serial_port.attach(&on_rx_interrupt, SerialBase::RxIrq);

    //PWM buzzer
    buzz.period_ms(1);
    buzz = 0.0f;

    //Set up button ISR
    button_1.rise(&button1Pressed);
    button_1.fall(&button1Released);

    //Write strings to the terminal
    char msg1[] = "Press 1 to turn ON the buzzer\n\r";
    char msg2[] = "Press 2 to turn OFF the buzzer\n\r";
    char msg3[] = "Press q to quit\n\r";
    serial_port.write(msg1, sizeof(msg1));
    serial_port.write(msg2, sizeof(msg2));
    serial_port.write(msg3, sizeof(msg3));
    wait_us(1000000);

    //Used for safe read of p (see below)
    char copyOfp;
    
    do
    {
        sleep();
        onBoardLED = !onBoardLED;       //Wake indicator

        //Read p safely
        CriticalSectionLock::enable();
        copyOfp = p;
        CriticalSectionLock::disable();

        switch (copyOfp) {
            case '1':
            buzz = 0.5f;
            break;
            case '2':
            buzz = 0.0f;
            break;
            default:
            serial_port.write("Received:", 10);
            serial_port.write((void*)&copyOfp, 1);
            serial_port.write("\n\r", 2);
            break;
        }
        
    } while (copyOfp != 'q');

    //Detatch ISR
    serial_port.attach(NULL);

    //Notify user
    serial_port.write("Done\n\r", 7);   //Blocking

    //Once finished - sleep
    while (true) {
        sleep();
        onBoardLED = !onBoardLED;
    }
 
}