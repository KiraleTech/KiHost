=======================================
KiNOS KBI interfacing C helper (KiHost)
=======================================

This project provides a set of C modules to help handling a Kirale Binary 
Interface enabled `KiNOS <http://kinos.io/>`_ device. The modules are written 
for use in a UNIX host, neccessary adaptions should be made for 
other environments.

The main purpose of the project is to help a novel KiNOS devices user understand
their capabilities and serve as a reference implementation for more advanced or
optimized developments.

Please refer to the `Kirale Binary Interface - Reference Guide - v2.0
<https://www.kirale.com/products/ktwm102/#resources>`_ for a complete 
comprehension of the KiNOS UART interfacing.

This project is licensed under the terms of the MIT license.

The modules dependancy scheme from higher to lower level would be:

::

 Application → kbi.c → cmds.c → cobs.c → uart.c

Modules
=======

uart.c
------

Provides the functions to open/close the serial device and send/receive 
characters. It should be adapted for every host platform.

cobs.c
------

Consistent Overhead Byte Stuffing (COBS) encoder and decoder implementation.

cmds.c
------

Send commands and receive responses and notifications based on the KBI Frame 
Format. Most of the frame's meaningful values are defined here.

kbi.c
-----

Implements a higher level way to handle the serial device usage.

A function is used to sequentially send a command and wait for a response with
several retries, making use of the UART receiving timeout capability. For a 
final implementation making use of serial interrupts is encouraged to handle
asynchronous notifications.

Also a set of socket handling functions is provided for the user to implement
any UDP application protocol on top of them.

Examples
========

client-server.c
---------------

For this example two UART enabled devices are required. One (the server) will 
form a Thread network and start listening for UDP traffic on a defined port. 
The received traffic will be echoed back. The other (the client) will join the 
network as an end device and start sending UDP traffic to its parent 
periodically and loggind the responses.

The server part also includes a small test to force a destination unreachable 
notification.

Some of the example parameters can be edited in the source.

Server setup:

::

 gcc -I include/ src/*.c examples/client-server.c -o server -DSERVER -DUART_PORT=\"/dev/ttyUSB0\"
 ./server

Client setup:

::

 gcc -I include/ src/*.c examples/client-server.c -o client -DDEBUG_COBS -DDEBUG_CMDS -DUART_PORT=\"/dev/ttyUSB1\"
 ./client

The debug macros are activated here to have an idea on screen of what's being 
transmitted and received from the device. One can play with them in both roles.

It might be interesting to setup a `sniffer 
<https://www.kirale.com/support/kb/usb-dongle-usage-as-packet-sniffer/>`_ to 
see the traffic over the air.


fwupdate.c
----------

This example shows how the KBI protocol can be used to perform a KiNOS firmware
upgrade.

It will simply pick up an official Kirale's DFU file, chop it into 64 bytes
blocks and send it to the device with the Firmware Update command using a stop
and wait ARQ mechanism.

There original and final firmware versions are shown on the screen, same as the
upload progress.

The usage is as follows:

::

 gcc -I include/ src/*.c examples/fwupdate.c -o fwupdate
 ./fwupdate --port /dev/ttyS1 --file KiNOS-GEN-KTWM102-1.3.7402.73020.dfu
