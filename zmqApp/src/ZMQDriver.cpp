/* ZMQDriver.cpp
 *
 * This is a driver to get data from a ZeroMQ server.
 *
 * Author: Xiaoqiang Wang
 *         Paul Scherrer Institute
 *
 * Created:  June 5, 2014
 *
 */
#include <cstring>

#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsStdio.h>
#include <epicsMutex.h>
#include <cantProceed.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsExit.h>

#include <zmq.h>

// TODO: replace JSON library with rapidJSON
#include <JSON.h>

#include "ZMQDriver.h"

static const char *driverName = "ZMQDriver";

void ZMQDriver::getNDAttrFromJSON(JSONValue *value, ChunkInfo &info, NDAttributeList &attributeList)
{
    if (!value->IsObject())
    {
        fprintf(stderr, "Invalid JSON Object\n");
        return;
    }
    JSONObject root = value->AsObject();

    /* check htype, only "chunk-1.0" supported */
    if (root.find(L"htype") == root.end() ||
        !root[L"htype"]->IsArray())
    {

        fprintf(stderr, "Invalid \"htype\" field\n");
        return;
    }
    JSONArray htype = root[L"htype"]->AsArray();
    if (htype[0]->AsString() != L"chunk-1.0")
    {
        fprintf(stderr, "\"htype\" != \"chunk-1.0\" \n");
        return;
    }

    /* get shape info */
    if (root.find(L"shape") == root.end() ||
        !root[L"shape"]->IsArray())
    {
        fprintf(stderr, "Invalid \"shape\" field\n");
        return;
    }
    JSONArray shape = root[L"shape"]->AsArray();
    if (shape.size() > ND_ARRAY_MAX_DIMS)
        return;
    info.ndims = shape.size();
    for (int i = 0; i < (int) shape.size(); i++)
    {
        info.dims[i] = shape[i]->AsNumber();
    }

    /* get frame number */
    if (root.find(L"frame") == root.end() ||
        !root[L"frame"]->IsNumber())
    {
        fprintf(stderr, "Invalid \"frame\" field\n");
        return;
    }
    info.frame = root[L"frame"]->AsNumber();

    /* get data type */
    if (root.find(L"type") == root.end() ||
        !root[L"type"]->IsString())
    {
        fprintf(stderr, "Invalid \"type\" field\n");
        return;
    }
    info.valid = true;
    std::wstring type = root[L"type"]->AsString();
    if (type == L"uint8")
        info.dataType = NDUInt8;
    else if (type == L"int8")
        info.dataType = NDInt8;
    else if (type == L"int16")
        info.dataType = NDInt16;
    else if (type == L"uint16")
        info.dataType = NDUInt16;
    else if (type == L"int32")
        info.dataType = NDInt32;
    else if (type == L"uint32")
        info.dataType = NDUInt32;
    else
    {
        info.valid = false;
        fprintf(stderr, "Unsupported data type\n");
    }
    /* parse ndattr */
    if (root.find(L"ndattr") == root.end() ||
        !root[L"ndattr"]->IsObject())
    {
        return;
    }
    JSONObject ndattr = root[L"ndattr"]->AsObject();
    for (JSONObject::iterator attr = ndattr.begin(); attr != ndattr.end(); ++attr)
    {
        std::wstring namew = attr->first;
        std::string name(namew.begin(), namew.end());

        JSONObject attrStruct = attr->second->AsObject();
        JSONValue *val = attrStruct[L"value"];
        JSONValue *type = attrStruct[L"dataType"];
        std::wstring vw = type->AsString();
        std::string attrType(vw.begin(), vw.end());
        // could check dataType here, but current JSON library converts all numbers to double
        if (val->IsNumber())
        {
            double v = val->AsNumber();
            attributeList.add(name.c_str(), name.c_str(), NDAttrFloat64, &v);
        }
        else if (val->IsString() && attrType == "string")
        {
            std::wstring vw = val->AsString();
            std::string v(vw.begin(), vw.end());
            attributeList.add(name.c_str(), name.c_str(), NDAttrString, (void *) v.c_str());
        }
        else
        {
            fprintf(stderr, "Invalid \"ndattr\" type\n");
        }
    }

}

/* parse data header */
ChunkInfo ZMQDriver::parseHeader(const char *msg, NDAttributeList &attributeList)
{
    ChunkInfo info;
    info.valid = false; /* indicate an invalid value */

    JSONValue *value = JSON::Parse(msg);
    if (value == NULL)
        return info;

    this->getNDAttrFromJSON(value, info, attributeList);

    delete value;
    return info;
}

asynStatus ZMQDriver::readData()
{

    int rc;
    zmq_msg_t message;
    int msg_len;
    char header[1024];
    ChunkInfo info;
    int nrows, ncols;
    NDColorMode_t colorMode;
    NDArrayInfo_t arrayInfo;
    NDArray *pImage = this->pArrays[0];
    NDAttributeList attributeList;
    const char *functionName = "readData";

    /* receive header */
    rc = zmq_msg_init(&message);
    msg_len = zmq_msg_recv(&message, this->socket, 0);
    if (msg_len == -1)
    {
        zmq_msg_close(&message);
        fprintf(stderr, "%s:%s: %s \n",
                driverName, functionName, zmq_strerror(zmq_errno()));
        return asynError;
    }

    /* is this the message to stop? */
    if (msg_len == 4 &&
        strncmp((const char *) zmq_msg_data(&message), "STOP", 4) == 0)
    {
        zmq_msg_close(&message);
        return asynError;
    }

    /* parse the header */
    strncpy(header, (const char *) zmq_msg_data(&message), msg_len);
    header[msg_len] = '\0';
    info = parseHeader(header, attributeList);

    /* we are done with the header message */
    zmq_msg_close(&message);

    /* receive data */
    rc = zmq_msg_init(&message);
    msg_len = zmq_msg_recv(&message, this->socket, 0);
    if (msg_len == -1)
    {
        zmq_msg_close(&message);
        fprintf(stderr, "%s:%s: %s \n",
                driverName, functionName, zmq_strerror(zmq_errno()));
        return asynError;
    }

    /* is this the message to stop? */
    if (msg_len == 4 &&
        strncmp((const char *) zmq_msg_data(&message), "STOP", 4) == 0)
    {
        printf("got STOP\n");
        zmq_msg_close(&message);
        return asynError;
    }

    /* if header is not parsed correctly then discard data 
     * NOTE: this check isn't done immeditely after parseHeader.
     * If we abort from receiving multipart messages, the next run will crash.
     * As of ZMQ 4.0.4.
     * */
    if (!info.valid)
    {
        zmq_msg_close(&message);
        return asynError;
    }

    ncols = info.dims[0];
    nrows = info.dims[1] == 0 ? 1 : info.dims[1];
    if (info.ndims == 3)
        colorMode = NDColorModeRGB1;
    else
        colorMode = NDColorModeMono;

    this->lock();
    if (pImage) pImage->release();
    this->pArrays[0] = this->pNDArrayPool->alloc(info.ndims, info.dims, info.dataType, 0, NULL);
    pImage = this->pArrays[0];
    asynPrint(this->pasynUserSelf, ASYN_TRACEIO_DRIVER,
              "%s:%s: dimensions=[%lu,%lu,%lu]\n",
              driverName, functionName,
              (unsigned long) info.dims[0], (unsigned long) info.dims[1], (unsigned long) info.dims[2]);

    /* does the received array size actually match the header info ?*/
    pImage->getInfo(&arrayInfo);
    if ((int) arrayInfo.totalBytes != msg_len)
    {
        zmq_msg_close(&message);
        asynPrint(this->pasynUserSelf, ASYN_TRACE_ERROR,
                  "%s:%s: received data size %d does not match header info %ul\n",
                  driverName, functionName, msg_len, arrayInfo.totalBytes);
        return asynError;
    }

    memcpy(pImage->pData, zmq_msg_data(&message), msg_len);
    zmq_msg_close(&message);

    /* image unique id comes from the server */
    pImage->uniqueId = info.frame;
    pImage->pAttributeList->add("ColorMode", "Color mode", NDAttrInt32, &colorMode);
    attributeList.copy(pImage->pAttributeList);

    setIntegerParam(ADSizeX, ncols);
    setIntegerParam(NDArraySizeX, ncols);
    setIntegerParam(ADSizeY, nrows);
    setIntegerParam(NDArraySizeY, nrows);
    setIntegerParam(NDArraySize, (int) arrayInfo.totalBytes);
    setIntegerParam(NDDataType, info.dataType);
    setIntegerParam(NDColorMode, colorMode);
    this->unlock();

    return asynSuccess;
}

static void ZMQTaskC(void *drvPvt)
{
    ZMQDriver *pPvt = (ZMQDriver *) drvPvt;

    pPvt->ZMQTask();
}

void ZMQDriver::stopAcquisition()
{
    zmq_send(this->stopSocket, "STOP", 4, 0);
}

void ZMQDriver::startReceive(const char *receiveFunction)
{
    setIntegerParam(ADStatus, ADStatusIdle);
    callParamCallbacks();
    /* Release the lock while we wait for an event that says acquire has started, then lock again */
    asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
              "%s:%s: waiting for acquire to start\n", driverName, receiveFunction);
    this->unlock();
    epicsEventWait(this->startEventId);
    this->lock();
    setIntegerParam(ADNumImagesCounter, 0);
    if (this->socketType == ZMQ_SUB)
        zmq_connect(this->socket, this->serverHost.c_str());
    else if (this->socketType == ZMQ_PULL)
        zmq_bind(this->socket, this->serverHost.c_str());
}

void ZMQDriver::ZMQTask()
{

    asynStatus dataStatus;
    int numImages, numImagesCounter;
    int imageMode;
    int arrayCallbacks;
    int acquire;
    NDArray *pImage;
    epicsTimeStamp startTime;
    const char *functionName = "ZMQTask";

    this->lock();
    /* Loop forever */
    while (1)
    {
        /* Is acquisition active? */
        getIntegerParam(ADAcquire, &acquire);

        /* If we are not acquiring then wait for a semaphore that is given when acquisition is started */
        if (!acquire)
        {
            this->startReceive(functionName);
        }

        /* We are acquiring. */
        /* Get the current time */
        epicsTimeGetCurrent(&startTime);

        setIntegerParam(ADStatus, ADStatusAcquire);

        /* Call the callbacks to update any changes */
        callParamCallbacks();

        /* Read the image */
        this->unlock();
        dataStatus = this->readData();
        this->lock();

        /* Call the callbacks to update any changes */
        callParamCallbacks();

        if (dataStatus == asynSuccess)
        {
            pImage = this->pArrays[0];

            /* Get the current parameters */
            getIntegerParam(ADNumImages, &numImages);
            getIntegerParam(ADNumImagesCounter, &numImagesCounter);
            getIntegerParam(ADImageMode, &imageMode);
            getIntegerParam(NDArrayCallbacks, &arrayCallbacks);
            numImagesCounter++;
            setIntegerParam(NDArrayCounter, pImage->uniqueId);
            setIntegerParam(ADNumImagesCounter, numImagesCounter);

            /* Put the frame number and time stamp into the buffer */
            pImage->timeStamp = startTime.secPastEpoch + startTime.nsec / 1.e9;

            /* Get any attributes that have been defined for this driver */
            this->getAttributes(pImage->pAttributeList);

            if (arrayCallbacks)
            {
                /* Call the NDArray callback */
                /* Must release the lock here, or we can get into a deadlock, because we can
                 * block on the plugin lock, and the plugin can be calling us */
                this->unlock();
                asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                          "%s:%s: calling imageData callback\n", driverName, functionName);
                doCallbacksGenericPointer(pImage, NDArrayData, 0);
                this->lock();
            }
        }

        /* See if acquisition is done */
        if ((dataStatus != asynSuccess) ||
            (imageMode == ADImageSingle) ||
            ((imageMode == ADImageMultiple) &&
             (numImagesCounter >= numImages)))
        {
            if (this->socketType == ZMQ_SUB)
                zmq_disconnect(this->socket, this->serverHost.c_str());
            else if (this->socketType == ZMQ_PULL)
                zmq_unbind(this->socket, this->serverHost.c_str());
            setIntegerParam(ADAcquire, 0);
            asynPrint(this->pasynUserSelf, ASYN_TRACE_FLOW,
                      "%s:%s: acquisition completed\n", driverName, functionName);
        }

        /* Call the callbacks to update any changes */
        callParamCallbacks();
        getIntegerParam(ADAcquire, &acquire);
    }
}

/* Disconnects the ZMQ connection */
static void shutdown(void *arg)
{
    ZMQDriver *p = (ZMQDriver *) arg;
    if (p) delete p;
}

ZMQDriver::~ZMQDriver()
{
    if (this->socketType == ZMQ_SUB)
    {
        /* stop if socket is blocked in receiving */
        zmq_send(stopSocket, "STOP", 4, 0);
        epicsThreadSleep(1);
        /* disconnect from host */
        zmq_disconnect(socket, this->serverHost.c_str());
        zmq_disconnect(socket, this->stopHost);
        zmq_close(socket);
        /* close stop socket server */
        zmq_unbind(stopSocket, this->stopHost);
        zmq_close(stopSocket);
    }
    else if (this->socketType == ZMQ_PULL)
    {
        /* stop if socket is blocked in receiving */
        zmq_send(stopSocket, "STOP", 4, 0);
        epicsThreadSleep(1);
        /* disconnect stop socket */
        zmq_disconnect(stopSocket, this->stopHost);
        zmq_close(stopSocket);
        /* close socket */
        zmq_unbind(socket, this->serverHost.c_str());
        zmq_close(socket);
    }

    zmq_ctx_destroy(context);
}


/** Called when asyn clients call pasynInt32->write().
  * This function performs actions for some parameters, including ADAcquire, ADBinX, etc.
  * For all parameters it sets the value in the parameter library and calls any registered callbacks..
  * \param[in] pasynUser pasynUser structure that encodes the reason and address.
  * \param[in] value Value to write. */
asynStatus ZMQDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
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
        if (value && (adstatus == ADStatusIdle))
        {
            /* Send an event to wake up the acquisition task.
             * It won't actually start generating new images until we release the lock below */
            epicsEventSignal(this->startEventId);
        }
        if (!value && (adstatus != ADStatusIdle))
        {
            /* This was a command to stop acquisition */
            this->stopAcquisition();
        }
    }
    else
    {
        /* If this parameter belongs to a base class call its method */
        status = ADDriver::writeInt32(pasynUser, value);
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


/** Report status of the driver.
  * Prints details about the driver if details>0.
  * It then calls the ADDriver::report() method.
  * \param[in] fp File pointed passed by caller where the output is written to.
  * \param[in] details If >0 then driver details are printed.
  */
void ZMQDriver::report(FILE *fp, int details)
{
    fprintf(fp, "ZMQ Driver %s\n", this->portName);
    if (details > 0)
    {
        int nx, ny, dataType;
        getIntegerParam(ADSizeX, &nx);
        getIntegerParam(ADSizeY, &ny);
        getIntegerParam(NDDataType, &dataType);
        fprintf(fp, "  Server host:       %s\n", this->serverHost.c_str());
        fprintf(fp, "  Socket type:       %d\n", this->socketType);
        if (this->socketType == ZMQ_SUB)
            fprintf(fp, "  Stop host:         %s\n", this->stopHost);
        fprintf(fp, "  NX, NY:            %d  %d\n", nx, ny);
        fprintf(fp, "  Data type:         %d\n", dataType);
    }

    /* Call the base class method */
    ADDriver::report(fp, details);
}

/** Constructor for ZMQ driver; most parameters are simply passed to ADDriver::ADDriver.
  * After calling the base class constructor this method creates a thread to collect the detector data, 
  * and sets reasonable default values for the parameters defined in this class, asynNDArrayDriver and ADDriver.
  * \param[in] portName The name of the asyn port driver to be created.int
  * \param[in] address The address & port of the ZMQ server, and pattern to be used. address:port.
  * \param[in] transport The protocol to be used for the connection [tcp/udp]
  * \param[in] zmqType The type of the ZeroMQ connection [PULL/SUB]
  * \param[in] maxBuffers The maximum number of NDArray buffers that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited number of buffers.
  * \param[in] maxMemory The maximum amount of memory that the NDArrayPool for this driver is 
  *            allowed to allocate. Set this to -1 to allow an unlimited amount of memory.
  * \param[in] priority The thread priority for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  * \param[in] stackSize The stack size for the asyn port driver thread if ASYN_CANBLOCK is set in asynFlags.
  */
ZMQDriver::ZMQDriver(const char *portName, const char *address, const char *transport, const char *zmqType,
                     int maxBuffers, size_t maxMemory, int priority, int stackSize)
        : ADDriver(portName, 1, 0, maxBuffers, maxMemory,
                   0, 0,               /* No interfaces beyond those set in ADDriver.cpp */
                   ASYN_CANBLOCK, 1,   /* ASYN_CANBLOCK=1, ASYN_MULTIDEVICE=0, autoConnect=1 */
                   priority, stackSize), context(0), socket(0)
{
    int status = asynSuccess;
    static const char *functionName = "zmq";
    char *cp;
    char type[10] = "";

    this->serverHost = std::string(transport) + std::string("://") + std::string(address);

    if (strcmp(zmqType, "SUB") == 0 || strcmp(zmqType, "PUB") == 0)
        this->socketType = ZMQ_SUB;
    else if (strcmp(zmqType, "PULL") == 0 || strcmp(zmqType, "PUSH") == 0)
        this->socketType = ZMQ_PULL;
    else if (strlen(zmqType) == 0)
    {
        /* If type is not specified, make a guess.
         * If "*" is found in host address, then it is assumed to be a PULL server type
         * */
        if (strchr(this->serverHost.c_str(), '*') != NULL)
        {
            this->socketType = ZMQ_PULL;
        }
        else
        {
            this->socketType = ZMQ_SUB;
        }
    }
    else
    {
        fprintf(stderr, "%s: Unsupported socket type %s\n", functionName, type);
        return;
    }

    /* Set some default values for parameters */
    status = setStringParam(ADManufacturer, "ZMQ Driver");
    if (this->socketType == ZMQ_SUB)
    {
        status |= setStringParam(ADModel, "ZeroMQ SUB");
    }
    else if (this->socketType == ZMQ_PULL)
    {
        status |= setStringParam(ADModel, "ZeroMQ PULL");
    }
    if (status)
    {
        fprintf(stderr, "%s: unable to set camera parameters\n", functionName);
        return;
    }

    /* initialize ZMQ */
    this->context = zmq_ctx_new();

    /* create the main socket */
    this->socket = zmq_socket(this->context, this->socketType);

    if (this->socketType == ZMQ_SUB)
    {
        /* filter the message from the server host */
        zmq_setsockopt(this->socket, ZMQ_SUBSCRIBE, "{", 1);

        /* create the pub socket to disconnect from server */
        this->stopSocket = zmq_socket(this->context, ZMQ_PUB);
        sprintf(this->stopHost, "inproc://%s", portName);
        int rc = zmq_bind(this->stopSocket, this->stopHost);
        if (rc != 0)
        {
            fprintf(stderr, "%s: unable to find a free port, %s\n",
                    functionName,
                    zmq_strerror(zmq_errno()));
            return;
        }
        /* connect to the stop pub server */
        zmq_connect(this->socket, stopHost);
        zmq_setsockopt(this->socket, ZMQ_SUBSCRIBE, "STOP", 4);
    }
    else if (this->socketType == ZMQ_PULL)
    {
        /* create the push socket to disconnect from server */
        this->stopSocket = zmq_socket(this->context, ZMQ_PUSH);
        char *p = this->stopHost;
        const char *q = this->serverHost.c_str();

        while (*q)
        {
            if (*q == '*')
            {
                strncpy(p, "127.0.0.1", 9);
                p += 9;
                q++;
            }
            else
                *p++ = *q++;
        }
        *p = '\0';

        zmq_connect(this->stopSocket, this->stopHost);
    }

    /* Create the epicsEvents for signaling to the acquisition task when acquisition starts */
    this->startEventId = epicsEventCreate(epicsEventEmpty);
    if (!this->startEventId)
    {
        fprintf(stderr, "%s:%s epicsEventCreate failure for start event\n",
                driverName, functionName);
        return;
    }

    /* Create the thread that updates the images */
    status = (epicsThreadCreate("ZMQTask",
                                epicsThreadPriorityMedium,
                                epicsThreadGetStackSize(epicsThreadStackMedium),
                                (EPICSTHREADFUNC) ZMQTaskC,
                                this) == NULL);
    if (status)
    {
        printf("%s:%s epicsThreadCreate failure for image task\n",
               driverName, functionName);
        return;
    }

    /* Register the shutdown function for epicsAtExit */
    epicsAtExit(shutdown, (void *) this);
}

extern "C" int ZMQDriverConfig(const char *portName, const char *address, const char *transport, const char *zmqType,
                               int maxBuffers, size_t maxMemory, int priority, int stackSize)
{
    new ZMQDriver(portName, address, transport, zmqType, maxBuffers, maxMemory, priority, stackSize);
    return (asynSuccess);
}


/* Code for iocsh registration */
static const iocshArg ZMQDriverConfigArg0 = {"Port name", iocshArgString};
static const iocshArg ZMQDriverConfigArg1 = {"address", iocshArgString};
static const iocshArg ZMQDriverConfigArg2 = {"transport protocol (tcp/udp)", iocshArgString};
static const iocshArg ZMQDriverConfigArg3 = {"socket type", iocshArgString};
static const iocshArg ZMQDriverConfigArg4 = {"maxBuffers", iocshArgInt};
static const iocshArg ZMQDriverConfigArg5 = {"maxMemory", iocshArgInt};
static const iocshArg ZMQDriverConfigArg6 = {"priority", iocshArgInt};
static const iocshArg ZMQDriverConfigArg7 = {"stackSize", iocshArgInt};
static const iocshArg *const ZMQDriverConfigArgs[] = {&ZMQDriverConfigArg0,
                                                      &ZMQDriverConfigArg1,
                                                      &ZMQDriverConfigArg2,
                                                      &ZMQDriverConfigArg3,
                                                      &ZMQDriverConfigArg4,
                                                      &ZMQDriverConfigArg5,
                                                      &ZMQDriverConfigArg6,
                                                      &ZMQDriverConfigArg7};
static const iocshFuncDef configZMQDriver = {"ZMQDriverConfig", 8, ZMQDriverConfigArgs};

static void configZMQDriverCallFunc(const iocshArgBuf *args)
{
    ZMQDriverConfig(args[0].sval, args[1].sval, args[2].sval, args[3].sval,
                    args[4].ival, args[5].ival, args[6].ival, args[7].ival);
}


static void ZMQDriverRegister(void)
{

    iocshRegister(&configZMQDriver, configZMQDriverCallFunc);
}

extern "C"
{
epicsExportRegistrar(ZMQDriverRegister);
}
