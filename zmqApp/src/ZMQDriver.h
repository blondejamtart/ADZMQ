//
// Created by myr45768 on 21/01/2020.
//

#ifndef ADZMQ_ZMQDRIVER_H
#define ADZMQ_ZMQDRIVER_H

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#include "ADDriver.h"
#include <string>

class ZMQControlledDriver;

/** Driver for ZMQ **/
class ZMQDriver : public ADDriver
{
    friend ZMQControlledDriver;
public:
    /* Constructor and Destructor */
    ZMQDriver(const char *portName, const char *address, const char *transport, const char *zmqType,
              int maxBuffers, size_t maxMemory, int priority, int stackSize);

    ~ZMQDriver();

    /* These are the methods that we override from ADDriver */
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);

    void report(FILE *fp, int details);

    /* These are called from C and so must be public */
    void ZMQTask();

private:
    /* These are the methods that are new to this class */
    asynStatus readData();

    virtual void startReceive(const char *receiveFunction);
    virtual void stopAcquisition();

    /* These items are specific to the zmq driver */
    std::string serverHost;
    char stopHost[HOST_NAME_MAX];
    void *context; /* ZMQ context */
    void *socket;  /* main socket to ZMQ server */
    void *stopSocket;/* internal pub socket to stop */
    int socketType;
    epicsEventId startEventId;
};

/* array information parsed from data header */
struct ChunkInfo
{
    int ndims;
    size_t dims[ND_ARRAY_MAX_DIMS];
    NDDataType_t dataType;
    int frame;
    bool valid;
};


#endif //ADZMQ_ZMQDRIVER_H
