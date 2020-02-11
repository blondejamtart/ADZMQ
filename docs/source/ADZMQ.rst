=====
ADZMQ
=====

.. contents:: Contents

Introduction
============

About
-----

ADZMQ provides plugins to allow NDArray data (including NDAttributes) to be trasmitted 
from or received into an AreaDetector plugin chain.
It is *NOT* required that the sender/receiver of this data is also an AreaDetector IOC; 
for example it could be a python script or some other data acquisition pipeline!
As such this module can allow for easy prototyping or testing of AreaDetector with other libraries.
 

ZeroMQ Basics
-------------

In ZeroMQ patterns, the message flow can be either uni- or
bi-directional. This project currently considers only uni-directional
message flow. And the patterns supported are PUB/SUB and PUSH/PULL.


ADZMQ Plugins
=============

ZMQDriver
---------

.. code:: bash

    # portName 		The name of the asyn port driver to be created.
    # address		The address & port of the ZMQ server, and pattern to be used (control port is port+1). [address:port].
    # transport 	The protocol to be used for the connection. [tcp/udp]
    # zmqType 		The type of the ZeroMQ connection. [PULL/SUB]
    # maxBuffers 	The maximum number of NDArray buffers that the NDArrayPool for this driver is
    #            	allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
    # maxMemory 	The maximum amount of memory that the NDArrayPool for this driver is
    #            	allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
    # priority 		The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
    # stackSize 	The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
    
     ZMQControlledDriverConfig(const char *portName, const char *address,
                               const char *transport, const char *zmqType,
                               int maxBuffers, size_t maxMemory,
                               int priority, int stackSize)

ZMQDriver listens for incoming data. By ZeroMQ patterns, this can be
either a puller or a subscriber.

ZMQControlledDriver
-------------------

.. code:: bash

    # portName 		The name of the asyn port driver to be created.
    # address		The address & port of the ZMQ server, and pattern to be used (control port is port+1). [address:port].
    # transport 	The protocol to be used for the connection. [tcp/udp]
    # zmqType 		The type of the ZeroMQ connection. [PULL/SUB]
    # controlMode 	Bitwise flag to set when & how control messages are sent.
    # maxBuffers 	The maximum number of NDArray buffers that the NDArrayPool for this driver is
    #            	allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
    # maxMemory 	The maximum amount of memory that the NDArrayPool for this driver is
    #            	allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
    # priority 		The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
    # stackSize 	The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
    
     ZMQControlledDriverConfig(const char *portName, const char *address,
                               const char *transport, const char *zmqType,
                               int controlMode, int maxBuffers, size_t maxMemory,
                               int priority, int stackSize)

ControlledZMQDriver is much the same as ZMQDriver, 
except it has an additional ZeroMQ socket (on port + 1) for sending 
control messages to the server (such as start/stop).
Currently, only start and stop messages are implemented.
Additionally, the server can send status messages in the header 
which will be used to set the ADStatusMessage param. 

The bitwise flags for control mode are as follows:

====================== ===============
Control Type           Flag Value
====================== ===============
*SEND_START*           0 (always true)
*SEND_STOP*            1
*SEND_START_WHEN_BUSY* 2
====================== ===============

NDPluginZMQ
-----------

.. code:: bash

     # portName         	The name of the asyn port driver to be created.
     # address			The address & port of the ZMQ server, and pattern to be used (control port is port+1). [address:port].
     # transport 		The protocol to be used for the connection. [tcp/udp]
     # zmqType 			The type of the ZeroMQ connection. [PUSH/PUB]
     # queueSize,       	The number of NDArrays that the input queue for this plugin can hold when 
     #                    	NDPluginDriverBlockingCallbacks=0. 
     # blockingCallbacks  	0=callbacks are queued and executed by the callback thread; 
     #                     	1 callbacks execute in the thread
     #                    	of the driver doing the callbacks.
     # NDArrayPort        	Port name of NDArray source
     # NDArrayAddr        	Address of NDArray source
     # maxBuffers         	Maximum number of NDArray buffers driver can allocate. -1=unlimited
     # maxMemory          	Maximum memory bytes driver can allocate. -1=unlimited

      NDZMQConfigure(const char *portName, const char *address, const char *transport,
                     const char *zmqType, int queueSize, int blockingCallbacks,
                     const char *NDArrayPort, int NDArrayAddr, int maxBuffers,
                     size_t maxMemory, int priority, int stackSize)

NDPluginZMQ pushes data out. By ZeroMQ patterns, this can be either a
pusher or a publisher.


