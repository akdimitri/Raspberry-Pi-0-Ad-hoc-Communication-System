# Raspberry-Pi-0-Ad-hoc-Communication-System

author:       Dimitrios Antoniadis

e-mail:       akdimitri@auth.gr

University:   Aristotle University of Thessaloniki (AUTH)

Subject:      Real Time Embedded Systems

Semester:     8th

---


**Description:** In this repository, an Ad-hoc Communication system is implemented for Raspberry Pi zero W. In the context of this network, devices are exchanging messages with specific format, until these messages reach their final destination.

---

This program is consisted of four(4) threads.
Threads:

    * Main thread
    * Message generator thread
    * Server thread
    * Client thread


_Main thread_ ([main.c](https://github.com/akdimitri/Raspberry-Pi-0-Ad-hoc-Communication-System/blob/master/code/main.c)):  this thread is responsible for the correct execution of the program. It initializes all data structures that are required for program's execution. Subsequently, it creates three threads _Message generator thread_, _Server thread_ and _Client thread_. Afterwards, this threads is waiting the other threads to exit.

_Message generator thread_ ([message_generator.c](https://github.com/akdimitri/Raspberry-Pi-0-Ad-hoc-Communication-System/blob/master/code/message_generator/message_generator.c)): this thread is created by _Main thread_. It is responsible for generating a message every [1,5] minutes. The newly generated message is saved in the _circular buffer_ alongside the _timestamp_ of generation.

_Server thread_ ([server.c](https://github.com/akdimitri/Raspberry-Pi-0-Ad-hoc-Communication-System/blob/master/code/server/server.c)): this thread is also created by _Main thread_. It is implimenting a _server_ which receives messages from other devices. When a message is received, _server_ is responsible for checking whether this message has already been received or if the device is the final destination of this message. According to the first case, if the message has not already been received, it is saved in the _circular buffer_ alongside the _timestamp_ of receivement. Otherwise, the message is discarded. According to the second case, if the message is destined for the _server's_ device, _server_ does not save the message but announces the receivement of the _message_.

_Client thread_ ([client.c](https://github.com/akdimitri/Raspberry-Pi-0-Ad-hoc-Communication-System/blob/master/code/client/client.c)): this thread is created by _Main thread_. It is constantly scanning the local network for connected devices. Upon a discovery of a device, _client_ is checking whether this devices has been connected before. Subsequently, _client_ sends only the messages that have not been sent before. If the connected device, is a new device, then all the messages included in the _circular buffer_ are sent to the device.
