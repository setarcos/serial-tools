HC-05 Checker
============

The original idea comes from the uartassistant example from QextSerialPort project. 
(http://code.google.com/p/qextserialport/)

The uartassistant example uses QextSerialPort as the serial driver, and I changed it
to use the newly introduced QSerialPort in Qt 5.1. So you need at least Qt 5.1 to
compile the code.

How to use
------------
I use portA to connect the master, portB to connect the slave. Open portA first, if the hc05 is
in configure mode, then get the mac address and set the password. Then open portB, configure it
and test the handshake of the two hc05s.
