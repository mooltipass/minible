from __future__ import print_function
from mooltipass_hid_device import *
from datetime import datetime
from array import array
if platform.system() == "Linux":
	from bluetooth_scan import *
	from label_printer import *
import platform
import usb.core
import usb.util
import random
import time
import sys
import os
nonConnectionCommands = ["deviceSNLabelPrinting"]

def main():
	skipConnection = False
	waitForDeviceConnection = False

	# HID device constructor
	mooltipass_device = mooltipass_hid_device()

	# If an arg is supplied and if the command doesn't require to connect to a device
	if len(sys.argv) > 1 and sys.argv[1] in nonConnectionCommands:
		skipConnection = True

	if not skipConnection:

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
				
		elif sys.argv[1] == "spamWithTraffic":
			while True:
				packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_GET_DEVICE_LANG, None), True)
				if packet["cmd"] == CMD_GET_DEVICE_STATUS:
					packet =  mooltipass_device.device.receiveHidMessage(True)		

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

		elif sys.argv[1] == "getSN":
			# Ask for the info
			packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_GET_DEVICE_SN, None), True)
			if packet["cmd"] == CMD_GET_DEVICE_STATUS:
				packet =  mooltipass_device.device.receiveHidMessage(True)
			print("Device Serial Number: " + str(struct.unpack('I', packet["data"][0:4])[0]))

		elif sys.argv[1] == "deviceSNLabelPrinting":
			last_serial_number = -1
			while True:
				while mooltipass_device.connect(False, read_timeout=1000) == False:
					time.sleep(.1)

				# Ask for the info
				packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_PLAT_INFO, None), True)
				if packet["cmd"] == CMD_GET_DEVICE_STATUS:
					packet =  mooltipass_device.device.receiveHidMessage(True)
				device_serial_number = struct.unpack('I', packet["data"][8:12])[0]

				# Print if different from before
				if last_serial_number != device_serial_number:
					print("device SN:", device_serial_number)
					print_labels_for_ble_device(device_serial_number)
					last_serial_number = device_serial_number

				# Wait for disconnect
				mooltipass_device.waitForDisconnect()
				time.sleep(1)

		elif sys.argv[1] == "printDiagData":
			mooltipass_device.printDiagData()

		elif sys.argv[1] == "switchOffAfterDisconnect":
			mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x0039, None), True)

		elif sys.argv[1] == "recondition":
			mooltipass_device.recondition()
			
		elif sys.argv[1] == "printMiniBleLabels":			
			while True:
				# Check if we can actually flash the SN...
				packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_PREPARE_SN_FLASH, None), True)
				if packet["cmd"] == CMD_GET_DEVICE_STATUS:
					packet =  mooltipass_device.device.receiveHidMessage(True)
					
				# Check for answer
				unit_already_programmed = True
				if packet["data"][0] == CMD_HID_ACK:
					print("Flash prepare: unit is not already programmed!")
					unit_already_programmed = False
						
					# Switch off after disconnect
					mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x0039, None), True)
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)

				if unit_already_programmed:
					# Ask for the programmed serial number
					packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_PLAT_INFO, None))	
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)
					device_sn = struct.unpack('I', packet["data"][8:12])[0]

					# Print labels
					print_labels_for_ble_device(device_sn)
						
					# Switch off after disconnect
					mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x0039, None), True)
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)
						
					# Debug
					print("Done, please disconnect")
					print("")

				# Wait for disconnect
				mooltipass_device.waitForDisconnect()
				time.sleep(1)
				
				# Wait for next connection
				while mooltipass_device.connect(False, read_timeout=1000) == False:
					time.sleep(.1)

		elif sys.argv[1] == "massProdProg":
			# Setup bluetooth scanning
			setup_bluetooth_scanning()
		
			# Create export directory
			if not os.path.isdir("export"):
				os.mkdir("export")
			
			last_serial_number = -1
			while True:
				# Ask for platform internal SN
				packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_GET_DEVICE_INT_SN, None), True)
				if packet["cmd"] == CMD_GET_DEVICE_STATUS:
					packet =  mooltipass_device.device.receiveHidMessage(True)
				device_internal_serial_number = struct.unpack('I', packet["data"][0:4])[0]
				print("Device internal serial number: " + str(device_internal_serial_number))

				# Do operations if device different than before
				if last_serial_number != device_internal_serial_number:
					# Inform device to prepare for SN flash
					packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_PREPARE_SN_FLASH, None), True)
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)

					# Check for answer
					if packet["data"][0] != CMD_HID_ACK:
						print("Flash prepare: unit already programmed!")
						mooltipass_device.waitForDisconnect()
						time.sleep(1)
						continue

					# Wait for device to come up over bluetooth...
					print("Waiting for Bluetooth to be picked up...")
					if not find_bluetooth_address("68:79:12:30:" + "{0:02x}".format((device_internal_serial_number >> 8) & 0x0FF) + ":" + "{0:02x}".format(device_internal_serial_number & 0x0FF), 30, 10):
						print("ATBTLC1000 error")
						mooltipass_device.waitForDisconnect()
						time.sleep(1)
						continue

					# Ask for SN input and program it
					device_programmed_sn = int(input("Input new device serial number: "))
					packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_SET_DEVICE_INT_SN, struct.pack('I', device_programmed_sn)), True)
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)

					# Check for answer
					if packet["data"][0] != CMD_HID_ACK:
						print("Couldn't program serial number!")
						mooltipass_device.waitForDisconnect()
						time.sleep(1)
						continue
					
					# Write LUT file
					with open(os.path.join("export", time.strftime("%Y-%m-%d-%H-%M-%S-Mooltipass-")) + str(device_internal_serial_number) + " to " + str(device_programmed_sn) +  ".txt", 'w') as f:
						# Write it down
						f.write(str(device_internal_serial_number) + ":" + str(device_programmed_sn) + "\r\n")

					# Finally, print labels
					print_labels_for_ble_device(device_programmed_sn)
					last_serial_number = device_internal_serial_number
						
					# Switch off after disconnect
					mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x0039, None), True)
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)
						
					# Debug
					print("Done, please disconnect")
					print("")

				# Wait for disconnect
				mooltipass_device.waitForDisconnect()
				time.sleep(1)
				
				# Wait for next connection
				while mooltipass_device.connect(False, read_timeout=1000) == False:
						time.sleep(.1)

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
