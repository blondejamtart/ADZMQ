import iocbuilder
import iocbuilder.modules.ADCore as ADCore
from iocbuilder.modules.asyn import AsynPort
from iocbuilder.modules.calc import Calc
from iocbuilder.arginfo import Simple, Ident


class _NDZMQGui(iocbuilder.AutoSubstitution):
    TemplateFile = "NDPluginZMQ.template"


class _ZMQDriverGui(iocbuilder.AutoSubstitution):
    TemplateFile = "ZMQDriver.template"


class NDZMQPlugin(AsynPort):
    Dependencies = [ADCore.ADCore]
    LibFileList = ["ADZMQ"]
    UniqueName = "PORT"
    DbdFileList = ["ADZMQSupport"]
    SysLibFileList = ["zmq"]
    _SpecificTemplate = ADCore.NDPluginBaseTemplate

    def __init__(self, PORT, DEST_ADDR, TRANSPORT, ZMQ_TYPE, NDARRAY_PORT, QUEUE=2, BLOCK=0,
                 NDARRAY_ADDR=0, ADDR=0, TIMEOUT=1, **kwargs):
        # Init the superclass (AsynPort)
        self.__super.__init__(PORT)
        # Update the attributes of self from the commandline args
        self.__dict__.update(locals())
        ADCore.makeTemplateInstance(self._SpecificTemplate, locals(), kwargs)
        _NDZMQGui(PORT=PORT, P=kwargs["P"], R=kwargs["R"], ADDR=ADDR, TIMEOUT=TIMEOUT)

    ArgInfo = (_SpecificTemplate.ArgInfo
               + iocbuilder.makeArgInfo(__init__,
                                        PORT=Simple('Port name for the NDProcess plugin', str),
                                        DEST_ADDR=Simple('Address to which to send ZMQ data', str),
                                        TRANSPORT=Simple('Transport Protocol', str),
                                        ZMQ_TYPE=Simple('ZMQ Socket Type', str),
                                        QUEUE=Simple('Input array queue size', int),
                                        BLOCK=Simple('Blocking callbacks?', int),
                                        NDARRAY_PORT=Ident('Input array port', AsynPort),
                                        NDARRAY_ADDR=Simple('Input array port address', int),
                                        ADDR=Simple('Asyn param address', int),
                                        TIMEOUT=Simple('Asyn parm timeout', int)))

    def Initialise(self):
        print (
            '# NDZMQConfigure(portName,serverHost, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxBuffers, maxMemory, priority, stackSize)')
        print (
                    'NDZMQConfigure("%(PORT)s", "%(DEST_ADDR)s", %(TRANSPORT)s, %(ZMQ_TYPE)s, %(QUEUE)d, %(BLOCK)d, "%(NDARRAY_PORT)s", %(NDARRAY_ADDR)s, 0, 0, 0, 0)' % self.__dict__)


class ZMQDriver(AsynPort):
    Dependencies = [ADCore.ADCore]
    LibFileList = ["ADZMQ"]
    UniqueName = "PORT"
    DbdFileList = ["ADZMQSupport"]
    SysLibFileList = ["zmq"]
    _SpecificTemplate = ADCore.ADBaseTemplate

    def __init__(self, PORT, SOURCE_ADDR, TRANSPORT, ZMQ_TYPE, BUSY_ACQUIRE=False,
                 QUEUE=2, ADDR=0, TIMEOUT=1, **kwargs):
        # Init the superclass (AsynPort)
        self.__super.__init__(PORT)
        # Update the attributes of self from the commandline args
        self.__dict__.update(locals())
        ADCore.makeTemplateInstance(self._SpecificTemplate, locals(), kwargs)
        _ZMQDriverGui(PORT=PORT, P=kwargs["P"], R=kwargs["R"])

    ArgInfo = (_SpecificTemplate.ArgInfo
               + iocbuilder.makeArgInfo(__init__,
                                        PORT=Simple('Port name for the NDProcess plugin', str),
                                        SOURCE_ADDR=Simple('Address to which to send ZMQ data', str),
                                        TRANSPORT=Simple('Transport Protocol', str),
                                        ZMQ_TYPE=Simple('ZMQ Socket Type', str),
                                        BUSY_ACQUIRE=Simple('Allow sending acquire when busy', bool),
                                        QUEUE=Simple('Input array queue size', int),
                                        ADDR=Simple('Asyn param address', int),
                                        TIMEOUT=Simple('Asyn parm timeout', int)))

    def Initialise(self):
        print (
            '# ZMQDriverConfig(portName, serverHost, queueSize, maxBuffers, maxMemory, priority, stackSize)')
        print (
                'ZMQDriverConfig("%(PORT)s", "%(SOURCE_ADDR)s", %(TRANSPORT)s, %(ZMQ_TYPE)s, %(QUEUE)d, 0, 0, 0, 0)' % self.__dict__)


class ZMQControlledDriver(ZMQDriver):
    def Initialise(self):
        print (
            '# ZMQControlledDriverConfig(portName, serverHost, queueSize, maxBuffers, maxMemory, priority, stackSize)')
        print (
                'ZMQControlledDriverConfig("%(PORT)s", "%(SOURCE_ADDR)s", %(TRANSPORT)s, %(ZMQ_TYPE)s, %(BUSY_ACQUIRE)d, %(QUEUE)d, 0, 0, 0, 0)' % self.__dict__)

