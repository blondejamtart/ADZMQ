//
// Created by myr45768 on 21/01/2020.
//

#include <epicsExport.h>
#include <iocsh.h>

#include <sstream>
#include <zmq.h>

#include "ZMQControlledDriver.h"

static const char *driverName = "ZMQControlledDriver";

ZMQControlledDriver::ZMQControlledDriver(const char *portName, const char *address, const char *transport, const char *zmqType,
                                unsigned int controlMode, int maxBuffers, size_t maxMemory, int priority, int stackSize) :
        ZMQDriver(portName, address, transport, zmqType, maxBuffers,
                  maxMemory, priority, stackSize)
{
    // create a Pub/Sub socket for sending control messages back to the WinCam ZMQ sender process
    this->controlSocket = zmq_socket(this->context, ZMQ_PUSH);
    std::string addrString = std::string(address);
    size_t delim = addrString.find(":");
    std::string portStr = addrString.substr(delim + 1, std::string::npos);
    size_t port = atoi(portStr.c_str());
    std::stringstream addrStream;
    addrStream << transport << "://" << addrString.substr(0, delim) << ":" << (port + 1);
    this->controlAddr = addrStream.str();
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "binding to control socket %s\n", this->controlAddr.c_str());
    zmq_bind(this->controlSocket, this->controlAddr.c_str());
    this->sendStop = controlMode & SEND_STOP;
    this->busyAcquire = controlMode & BUSY_ACQUIRE;
}

ZMQControlledDriver::~ZMQControlledDriver()
{
    zmq_unbind(this->controlSocket, this->controlAddr.c_str());
    zmq_close(this->controlSocket);
}

void ZMQControlledDriver::stopAcquisition()
{
    if (this->sendStop)
    {
        zmq_send(this->controlSocket, "{\"acquire\": \"stop\"}", 19, 0);
        ZMQDriver::stopAcquisition();
    }
}


void ZMQControlledDriver::startReceive(const char *receiveFunction)
{
    ZMQDriver::startReceive(receiveFunction);
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "Sending start message to frame server\n");
    zmq_send(this->controlSocket, "{\"acquire\": \"start\"}", 20, 0);
}

/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire, ADBinX, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus ZMQControlledDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
    int function = pasynUser->reason;
    int status = asynSuccess;
    int adstatus;
    static const char *functionName = "writeInt32";

    /* Set the parameter and readback in the parameter library.  This may be overwritten when we read back the
     * status at the end, but that's OK */
    status |= setIntegerParam(function, value);

    if (function == ADAcquire)
    {
        getIntegerParam(ADStatus, &adstatus);
        if (value && (adstatus != ADStatusIdle) && this->busyAcquire)
        {
            /* RX thread already active, just send another control message */
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW, "Sending start message to frame server\n");
            zmq_send(this->controlSocket, "{\"acquire\": \"start\"}", 20, 0);
        }
        else
        {
            /* Call base class methods */
            status = ZMQDriver::writeInt32(pasynUser, value);
        }
    }
    else
    {
        /* If this parameter belongs to a base class call its method */
        status = ZMQDriver::writeInt32(pasynUser, value);
    }

    if (status)
        asynPrint(pasynUser, ASYN_TRACE_ERROR,
                  "%s:%s: error, status=%d function=%d, value=%d\n",
                  driverName, functionName, status, function, value);
    else
        asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
                  "%s:%s: function=%d, value=%d\n",
                  driverName, functionName, function, value);
    return ((asynStatus) status);
}

extern "C" int
ZMQControlledDriverConfig(const char *portName, const char *address, const char *transport, const char *zmqType,
                      int controlMode, int maxBuffers, size_t maxMemory, int priority, int stackSize)
{
    new ZMQControlledDriver(portName, address, transport, zmqType, controlMode,
            maxBuffers, maxMemory, priority, stackSize);
    return (asynSuccess);
}


/* Code for iocsh registration */
static const iocshArg ZMQControlledDriverConfigArg0 = {"Port name", iocshArgString};
static const iocshArg ZMQControlledDriverConfigArg1 = {"address", iocshArgString};
static const iocshArg ZMQControlledDriverConfigArg2 = {"transport protocol (tcp/udp)", iocshArgString};
static const iocshArg ZMQControlledDriverConfigArg3 = {"socket type", iocshArgString};
static const iocshArg ZMQControlledDriverConfigArg4 = {"controlMode", iocshArgInt};
static const iocshArg ZMQControlledDriverConfigArg5 = {"maxBuffers", iocshArgInt};
static const iocshArg ZMQControlledDriverConfigArg6 = {"maxMemory", iocshArgInt};
static const iocshArg ZMQControlledDriverConfigArg7 = {"priority", iocshArgInt};
static const iocshArg ZMQControlledDriverConfigArg8 = {"stackSize", iocshArgInt};
static const iocshArg *const ZMQControlledDriverConfigArgs[] = {&ZMQControlledDriverConfigArg0,
                                                            &ZMQControlledDriverConfigArg1,
                                                            &ZMQControlledDriverConfigArg2,
                                                            &ZMQControlledDriverConfigArg3,
                                                            &ZMQControlledDriverConfigArg4,
                                                            &ZMQControlledDriverConfigArg5,
                                                            &ZMQControlledDriverConfigArg6,
                                                            &ZMQControlledDriverConfigArg7,
                                                            &ZMQControlledDriverConfigArg8};
static const iocshFuncDef configZMQControlledDriver = {"ZMQControlledDriverConfig", 9, ZMQControlledDriverConfigArgs};

static void configZMQControlledDriverCallFunc(const iocshArgBuf *args)
{
    ZMQControlledDriverConfig(args[0].sval, args[1].sval, args[2].sval, args[3].sval, args[4].ival,
                              args[5].ival, args[6].ival, args[7].ival, args[8].ival);
}


static void ZMQControlledDriverRegister(void)
{
    iocshRegister(&configZMQControlledDriver, configZMQControlledDriverCallFunc);
}

extern "C"
{
epicsExportRegistrar(ZMQControlledDriverRegister);
}