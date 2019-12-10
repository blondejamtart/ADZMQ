#ifndef NDPluginZMQ_H
#define NDPluginZMQ_H

#include "NDPluginDriver.h"
#include <string>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

#define zmqFirstParamString "ZMQ_FIRST"
#define zmqIsConnectedParamString "ZMQ_IS_CONNECTED"
#define zmqConnectedAddressParamString "ZMQ_CONNECTED_ADDRESS"
#define zmqLastParamString "ZMQ_LAST"

/** Base class for NDArray ZMQ streaming plugins. */
class NDPluginZMQ : public NDPluginDriver {
public:
    NDPluginZMQ(const char *portName, const char *address, const char *transport, const char *zmqType,
                int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr,
                 int maxBuffers, size_t maxMemory, int priority, int stackSize);

    ~NDPluginZMQ();

    /* These methods override those in the base class */
    virtual void processCallbacks(NDArray *pArray);

protected:
    std::string getAttributesAsJSON(NDAttributeList *pAttributeList);

private:
    void *context;
    void *socket;
    std::string serverHost;
    int socketType;

    int zmqFirstParam;
#define NDZMQ_FIRST_DRIVER_COMMAND zmqFirstParam
    int zmqIsConnectedParam;
    int zmqConnectedAddressParam;
    int zmqLastParam;
#define NDZMQ_LAST_DRIVER_COMMAND zmqLastParam

};
    
#endif
