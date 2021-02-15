from __future__ import print_function
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
	waitForDeviceConnection = False
	
	# If an arg is supplied and if the command doesn't require to connect to a device
	if len(sys.argv) > 1 and sys.argv[1] in nonConnectionCommands:
		skipConnection = True
		
	# Waiting for device connection
	if len(sys.argv) > 1 and sys.argv[1] == "initCode":
		waitForDeviceConnection = True
	

	if not skipConnection:
		# HID device constructor
		mooltipass_device = mooltipass_hid_device()	
		
		# Connect to device
		if waitForDeviceConnection == False:
			if mooltipass_device.connect(True) == False:
				sys.exit(0)
		else:
			while mooltipass_device.connect(False) == False:
				time.sleep(1)
			
		print("Connected to device")
		
		while len(sys.argv) > 1 and sys.argv[1] == "log":
			mooltipass_device.getInternalDevice().receiveHidMessage(exit_on_timeout=False)
			#try:
			#	mooltipass_device.getInternalDevice().receiveHidMessage()
			#except KeyboardInterrupt:
			#	sys.exit(0)
			#except Exception as e:
			#	continue
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
				print("Please specify picture filename")
		
		elif sys.argv[1] == "uploadDebugBundle":
			# mooltipass_tool.py uploadDebugBundle filename
			if len(sys.argv) > 2:
				filename = sys.argv[2]
				mooltipass_device.uploadDebugBundle(filename)
			else:
				print("Please specify bundle filename")
		
		elif sys.argv[1] == "uploadBundle":
			# mooltipass_tool.py uploadDebugBundle filename
			if len(sys.argv) > 3:
				filename = sys.argv[2]
				passwd = sys.argv[3]
				mooltipass_device.uploadAndUpgradePlatform(filename, passwd)
			else:
				print("Please specify bundle filename")
		
		elif sys.argv[1] == "rebootToBootloader":
			mooltipass_device.rebootToBootloader()
			
		elif sys.argv[1] == "flashAuxMcuFromBundle":
			mooltipass_device.flashAuxMcuFromBundle()
			
		elif sys.argv[1] == "platInfo":
			mooltipass_device.getPlatInfo()
			
		elif sys.argv[1] == "accGet":
			mooltipass_device.getAccData()
			
		elif sys.argv[1] == "flashUniqueData":
			mooltipass_device.flashUniqueData()
			
		elif sys.argv[1] == "timediff":
			mooltipass_device.timeDiff()
			
		elif sys.argv[1] == "initCode":
			mooltipass_device.initCode()
			
		elif sys.argv[1] == "printDiagData":
			mooltipass_device.printDiagData()
			
		elif sys.argv[1] == "recondition":
			mooltipass_device.recondition()
			
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
