import iocbuilder
import iocbuilder.modules.ADCore as ADCore
from iocbuilder.modules.asyn import AsynPort
from iocbuilder.modules.calc import Calc
from iocbuilder.arginfo import Simple, Ident

class _NDZMQGui(iocbuilder.AutoSubstitution):

    TemplateFile = "NDZMQ.template"

class NDZMQPlugin(AsynPort):
    Dependencies = [ADCore.ADCore]
    LibFileList = ["ADZMQ"]
    UniqueName = "PORT"
    DbdFileList = ["ADZMQSupport"]
    SysLibFileList = ["zmq"]
    _SpecificTemplate = ADCore.NDPluginBaseTemplate

    def __init__(self, PORT, DEST_ADDR, NDARRAY_PORT, QUEUE=2, BLOCK=0,
                 NDARRAY_ADDR=0, **kwargs):
        # Init the superclass (AsynPort)
        self.__super.__init__(PORT)
        # Update the attributes of self from the commandline args
        self.__dict__.update(locals())
        ADCore.makeTemplateInstance(self._SpecificTemplate, locals(), kwargs)
	_NDZMQGui(PORT=PORT, P=kwargs["P"], R=kwargs["R"])

    ArgInfo = (_SpecificTemplate.ArgInfo
               + iocbuilder.makeArgInfo(__init__,
                                        PORT=Simple('Port name for the NDProcess plugin', str),
                                        DEST_ADDR=Simple('Address to which to send ZMQ data', str),
                                        QUEUE=Simple('Input array queue size', int),
                                        BLOCK=Simple('Blocking callbacks?', int),
                                        NDARRAY_PORT=Ident('Input array port', AsynPort),
                                        NDARRAY_ADDR=Simple('Input array port address', int)))

    def Initialise(self):
        print ('# NDZMQConfigure(portName,serverHost, queueSize, blockingCallbacks, NDArrayPort, NDArrayAddr, maxBuffers, maxMemory, priority, stackSize)')
        print ('NDZMQConfigure("%(PORT)s", "%(DEST_ADDR)s", %(QUEUE)d, %(BLOCK)d, "%(NDARRAY_PORT)s", %(NDARRAY_ADDR)s, 0, 0, 0, 0)' % self.__dict__)
