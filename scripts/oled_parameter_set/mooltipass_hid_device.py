from __future__ import print_function
from os.path import isfile, join, isdir
from resizeimage import resizeimage
from mooltipass_defines import *
from generic_hid_device import *
from pprint import pprint
from array import array
from PIL import Image
import struct
import random
import glob
import math
import os

# Custom HID device class
class mooltipass_hid_device:

	# Device constructor
	def __init__(self):
		# HID device constructor
		self.device = generic_hid_device()
	
	# Try to connect to Mooltipass device
	def connect(self, verbose):
		return self.device.connect(verbose, USB_VID, USB_PID, USB_READ_TIMEOUT, self.createPingPacket());
		
	# Disconnect
	def disconnect(self):
		self.device.disconnect()
		
	# Get private device object
	def getInternalDevice(self):
		return self.device
		
	# Set private device object
	def setInternalDevice(self, device):
		self.device = device
		
	# Get text from byte array
	def getTextFromUsbPacket(self, usb_packet):
		return "".join(map(chr, usb_packet[DATA_INDEX:])).split(b"\x00")[0]
		
	# Transform text to byte array
	def textToByteArray(self, text):
		ret_dat = array('B') 
		ret_dat.extend(map(ord,text))
		ret_dat.append(0)
		return ret_dat
		
	# Get a packet to send for a given command and payload
	def getPacketForCommand(self, cmd, data):
		# Create packet object
		packet = {}
		packet["cmd"] = array('B')
		packet["len"] = array('B')
		packet["data"] = array('B')
			
		# command
		packet["cmd"].fromstring(struct.pack('H', cmd))
		
		# data length
		if data is not None:
			packet["len"].fromstring(struct.pack('H', len(data)))
		else:
			packet["len"].fromstring(struct.pack('H', 0))

		# data
		if data is not None:
			packet["data"].extend(data)
			
		return packet

	# Create a ping packet to be sent to the device
	def createPingPacket(self):	
		return self.getPacketForCommand(CMD_PING, [random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)])
		
	def sendOledParameters(self, contrast, vcomh, vsegm, precharge_per, discharge_per, discharge):
		self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_SET_OLED_PARAMS, [contrast, vcomh, vsegm, precharge_per, discharge_per, discharge]))
		
	