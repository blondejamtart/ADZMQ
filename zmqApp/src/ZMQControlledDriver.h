//
// Created by myr45768 on 21/01/2020.
//

#ifndef ADZMQ_WINCAMZMQDRIVER_H
#define ADZMQ_WINCAMZMQDRIVER_H

#include <string>

#include "ZMQDriver.h"

class ZMQControlledDriver : public ZMQDriver
{
public:
    ZMQControlledDriver(const char *portName, const char *address, const char *transport, const char *zmqType,
                    bool busyAcquire, int maxBuffers, size_t maxMemory, int priority, int stackSize);

    ~ZMQControlledDriver();

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

private:
    void startReceive(const char *receiveFunction);
    void stopAcquisition();
    void *controlSocket;  /* main socket to ZMQ server */
    std::string controlAddr;
    bool busyAcquire;

};

#endif //ADZMQ_WINCAMZMQDRIVER_H
