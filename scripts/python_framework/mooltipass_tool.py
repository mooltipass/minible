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
		mooltipass_device.getInternalDevice().benchmarkPingPongSpeed(mooltipass_device.createPingPacket())
			
		# Get Mooltipass Version
		#version_data = mooltipass_device.getMooltipassVersionAndVariant()
		#print "Mooltipass version: " + version_data[1] + ", variant: " + version_data[2] + ", " + str(version_data[0]) + "Mb of Flash"
			
		# Print Mooltipass status
		#print "Mooltipass status:", mooltipass_device.getMooltipassStatus()
		#print ""
	
	# See if args were passed
	if len(sys.argv) > 1:
		if sys.argv[1] == "uploadBundle":
			# extract args
			if len(sys.argv) > 2:
				filename = sys.argv[2]
			else:
				filename = None
			if len(sys.argv) > 3:
				password = sys.argv[3]
			else:
				password = None
			# start upload
			#mooltipass_device.uploadBundle(password, filename, True)
		
	#if not skipConnection:
	#	mooltipass_device.disconnect()

if __name__ == "__main__":
	main()
