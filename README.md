A Serial Port Logging and Debugging Tools Based on QT 5.1
============

The original idea comes from the uartassistant example from QextSerialPort project. 
(http://code.google.com/p/qextserialport/)

The uartassistant example uses QextSerialPort as the serial driver, and I changed it
to use the newly introduced QSerialPort in Qt 5.1. So you need at least Qt 5.1 to
compile the code.

Features
------------
- Can utilize two serial ports at the same time.
- Can act as a relay. ie what received at port A will be pass to port B, and vice versa.
- Log received data.
- Persistent settings.
- Auto open serial port.
