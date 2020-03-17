#!/dls_sw/prod/tools/RHEL7-x86_64/defaults/bin/dls-python

from pkg_resources import require

require("cothread")
require("scipy")
require("numpy")

from cothread import catools, WaitForQuit
import scipy
import numpy
import zmq
import json
import argparse

header_t = {'htype': ['chunk-1.0'], 'shape': [1, None, None], 'type': None, 'frame': None}
uid = 0

def create_header(array):
	global uid
	header = json.loads(json.dumps(header_t))
	header['shape'][1] = array.shape[0]
	header['shape'][2] = 1
	header['type'] = '%s' % array.dtype
	uid += 1
	header['frame'] = uid
	return header

def push_FFT(array_value, socket):
	header = create_header(array_value)
	fft_value = scipy.fft(array_value)
	array_shape = list(array_value.shape)
	array_shape += [2]
	header['shape'][1] = 2
	array = numpy.zeros(array_shape) 
	array[:,0] = numpy.abs(fft_value)
	array[:,1] = numpy.angle(fft_value)
	socket.send(json.dumps(header), zmq.SNDMORE)
	socket.send(array.tobytes())	

def push_WFM(array_value, socket):
	header = create_header(array_value)
	socket.send(json.dumps(header), zmq.SNDMORE)
	socket.send(array_value.tobytes())

def push_both(array_value, socket):
	header = create_header(array_value)
	fft_value = scipy.fft(array_value)
	array_shape = list(array_value.shape)
	array_shape += [3]
	header['shape'][1] = 3
	array = numpy.zeros(array_shape) 
        array[:,0] = array_value
	array[:,1] = numpy.abs(fft_value)
	array[:,2] = numpy.angle(fft_value)
	socket.send(json.dumps(header), zmq.SNDMORE)
	socket.send(array.tobytes())
	

if __name__ == "__main__":
       
        parser = argparse.ArgumentParser(description="forward waveform data via zmq")
        parser.add_argument("-H", "--host", default="tcp://127.0.0.1:1515",
                            help="host name for zmq endpoint")
        parser.add_argument("-p", "--pv",
                            help="Waveform PV to monitor")
        args = parser.parse_args()
        print("Monitoring PV %s, sending to %s" %(args.pv, args.host))
   
	con = zmq.Context()
	sock = con.socket(zmq.PUSH)
	sock.connect(args.host)

	def push_FFT_w_socket(array_value):
		return push_FFT(array_value, sock)

	def push_WFM_w_socket(array_value):
		return push_WFM(array_value, sock)

	def push_both_w_socket(array_value):
		return push_both(array_value, sock)

	catools.camonitor(args.pv, push_WFM_w_socket)
	WaitForQuit()
