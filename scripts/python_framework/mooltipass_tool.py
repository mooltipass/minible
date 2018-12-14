#!/usr/bin/env python2
from mooltipass_hid_device import *
from datetime import datetime
from array import array
import platform
import usb.core
import usb.util
import random
import time
import sys
nonConnectionCommands = []

def main():
	skipConnection = False
	
	# If an arg is supplied and if the command doesn't require to connect to a device
	if len(sys.argv) > 1 and sys.argv[1] in nonConnectionCommands:
		skipConnection = True

	if not skipConnection:
		# HID device constructor
		mooltipass_device = mooltipass_hid_device()	
		
		# Connect to device
		if mooltipass_device.connect(True) == False:
			sys.exit(0)
			
		print "Connected to device"
		
		while False:
			try:
				mooltipass_device.getInternalDevice().receiveHidMessage()
			except KeyboardInterrupt:
				sys.exit(0)
			except:
				continue
		#mooltipass_device.getInternalDevice().benchmarkPingPongSpeed(mooltipass_device.createPingPacket())
			
		# Get Mooltipass Version
		#version_data = mooltipass_device.getMooltipassVersionAndVariant()
		#print "Mooltipass version: " + version_data[1] + ", variant: " + version_data[2] + ", " + str(version_data[0]) + "Mb of Flash"
			
		# Print Mooltipass status
		#print "Mooltipass status:", mooltipass_device.getMooltipassStatus()
		#print ""
	
	# See if args were passed
	if len(sys.argv) > 1:
		if sys.argv[1] == "sendMonitorFrame":
			# mooltipass_tool.py sendMonitorFrame filename
			if len(sys.argv) > 2:
				bitdepth = 4
				mooltipass_device.sendAndMonitorFrame(sys.argv[2], bitdepth)
			else:
				print "Please specify picture filename"
		
		elif sys.argv[1] == "uploadDebugBundle":
			# mooltipass_tool.py uploadDebugBundle filename
			if len(sys.argv) > 2:
				filename = sys.argv[2]
				mooltipass_device.uploadDebugBundle(filename)
			else:
				print "Please specify bundle filename"
		
		elif sys.argv[1] == "rebootToBootloader":
			mooltipass_device.rebootToBootloader()
			
		elif sys.argv[1] == "flashAuxMcuFromBundle":
			mooltipass_device.flashAuxMcuFromBundle()
			
		elif sys.argv[1] == "platInfo":
			mooltipass_device.getPlatInfo()
			
		elif sys.argv[1] == "accGet":
			mooltipass_device.getAccData()
			
		elif sys.argv[1] == "debugListen":
			while True:
				try:
					mooltipass_device.getInternalDevice().receiveHidMessage()
				except KeyboardInterrupt:
					sys.exit(0)
				except:
					continue
		
	#if not skipConnection:
	#	mooltipass_device.disconnect()

if __name__ == "__main__":
	main()
