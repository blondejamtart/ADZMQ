//
// Created by myr45768 on 21/01/2020.
//

#ifndef ADZMQ_WINCAMZMQDRIVER_H
#define ADZMQ_WINCAMZMQDRIVER_H

#define SEND_STOP       1
#define BUSY_ACQUIRE    2

#include <string>

#include "ZMQDriver.h"

class ZMQControlledDriver : public ZMQDriver
{
public:
    ZMQControlledDriver(const char *portName, const char *address, const char *transport, const char *zmqType,
                    unsigned int controlMode, int maxBuffers, size_t maxMemory, int priority, int stackSize);

    ~ZMQControlledDriver();

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

private:

    void startReceive(const char *receiveFunction);
    void stopAcquisition();

    virtual ChunkInfo parseHeader(const char *msg, NDAttributeList &attributeList);

    void *controlSocket;  /* main socket to ZMQ server */
    std::string controlAddr;
    bool busyAcquire;
    bool sendStop;

};

#endif //ADZMQ_WINCAMZMQDRIVER_H
