from __future__ import print_function
from mooltipass_hid_device import *
from datetime import datetime
from array import array
if platform.system() == "Linux":
	from label_printer import *
import platform
import usb.core
import usb.util
import random
import time
import sys
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
			
		elif sys.argv[1] == "postLabelPrinting":
			last_serial_number = -1
			while True:
				while mooltipass_device.connect(False, read_timeout=1000) == False:
						time.sleep(.1)
					
				# Ask for the platfor info
				packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_PLAT_INFO, None), True)	
				if packet["cmd"] == CMD_GET_DEVICE_STATUS:
					packet =  mooltipass_device.device.receiveHidMessage(True)
				device_serial_number = struct.unpack('I', packet["data"][8:12])[0]
				
				# Reset default settings
				device_default_settings = [0]*64
				device_default_settings[0] = 0      # SETTING_RESERVED_ID                
				device_default_settings[1] = 0      # SETTING_RANDOM_PIN_ID              
				device_default_settings[2] = 15     # SETTING_USER_INTERACTION_TIMEOUT_ID
				device_default_settings[3] = 1      # SETTING_FLASH_SCREEN_ID            
				device_default_settings[4] = 0      # SETTING_DEVICE_DEFAULT_LANGUAGE    
				device_default_settings[5] = 0x09   # SETTINGS_CHAR_AFTER_LOGIN_PRESS    
				device_default_settings[6] = 0x0A   # SETTINGS_CHAR_AFTER_PASS_PRESS     
				device_default_settings[7] = 15     # SETTINGS_DELAY_BETWEEN_PRESSES     
				device_default_settings[8] = 1      # SETTINGS_BOOT_ANIMATION            
				device_default_settings[9] = 0x90   # SETTINGS_MASTER_CURRENT            
				device_default_settings[10] = 1     # SETTINGS_LOCK_ON_DISCONNECT        
				device_default_settings[11] = 9     # SETTINGS_KNOCK_DETECT_SENSITIVITY  
				device_default_settings[12] = 0     # SETTINGS_LEFT_HANDED_ON_BATTERY    
				device_default_settings[13] = 0     # SETTINGS_LEFT_HANDED_ON_USB        
				device_default_settings[14] = 0     # SETTINGS_PIN_SHOWN_WHEN_BACK       
				device_default_settings[15] = 0     # SETTINGS_UNLOCK_FEATURE_PARAM      
				device_default_settings[16] = 1     # SETTINGS_DEVICE_TUTORIAL           
				device_default_settings[17] = 0     # SETTINGS_SHOW_PIN_ON_ENTRY         
				device_default_settings[18] = 0     # SETTINGS_DISABLE_BLE_ON_CARD_REMOVE
				device_default_settings[19] = 0     # SETTINGS_DISABLE_BLE_ON_LOCK  
				device_default_settings[20] = 0     # SETTINGS_NB_MINUTES_FOR_LOCK 
				device_default_settings[21] = 0     # SETTINGS_SWITCH_OFF_AFTER_USB_DISC
				device_default_settings[22] = 0     # SETTINGS_HASH_DISPLAY_FEATURE
				device_default_settings[23] = 30    # SETTINGS_INFORMATION_TIME_DELAY
				device_default_settings[24] = 0     # SETTINGS_BLUETOOTH_SHORTCUTS
				device_default_settings[25] = 0     # SETTINGS_SCREEN_SAVER_ID
				device_default_settings[26] = 1      # SETTINGS_PREF_ST_SERV_FEATURE
				mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x0D, device_default_settings), True)	
				
				# Reset default language
				mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x1F, [0]), True)	
				
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
			
		elif sys.argv[1] == "massProdProg":
			last_serial_number = -1
			while True:
				while mooltipass_device.connect(False, read_timeout=1000) == False:
						time.sleep(.1)
					
				# Ask for the platform info
				packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_PLAT_INFO, None), True)	
				if packet["cmd"] == CMD_GET_DEVICE_STATUS:
					packet =  mooltipass_device.device.receiveHidMessage(True)
				device_serial_number = struct.unpack('I', packet["data"][8:12])[0]
				print("Device external serial number: " + str(device_serial_number))
				
				# Ask for platform internal SN
				packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_GET_DEVICE_INT_SN, None), True)	
				if packet["cmd"] == CMD_GET_DEVICE_STATUS:
					packet =  mooltipass_device.device.receiveHidMessage(True)
				device_internal_serial_number = struct.unpack('I', packet["data"][0:4])[0]
				print("Device internal serial number: " + str(device_internal_serial_number))
				
				# Check for same SNs (device already programmed)
				if device_serial_number == device_internal_serial_number and device_internal_serial_number != 0xFFFFFFFF:
					print("Connected device: unit already programmed!")
					mooltipass_device.waitForDisconnect()
					time.sleep(1)
					continue
				
				# Do operations if device different than before
				if last_serial_number != device_internal_serial_number:
					# Change timeout
					mooltipass_device.device.setReadTimeout(3000)
				
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
						
					# Wait for device to come up over bluetooth... TODO
					
					# Ask for SN input and program it
					device_programmed_sn = int(input("Input device serial number: "))
					packet = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_SET_DEVICE_INT_SN, struct.pack('I', device_programmed_sn)), True)	
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)
					
					# Check for answer
					if packet["data"][0] != CMD_HID_ACK:
						print("Couldn't program serial number!")
						mooltipass_device.waitForDisconnect()
						time.sleep(1)
						continue
						
					# Finally, print labels
					print_labels_for_ble_device(device_serial_number)
					last_serial_number = device_internal_serial_number
					
					# Switch off after disconnect
					mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x0039, None), True)	
					if packet["cmd"] == CMD_GET_DEVICE_STATUS:
						packet =  mooltipass_device.device.receiveHidMessage(True)
					
				# Wait for disconnect
				mooltipass_device.waitForDisconnect()
				time.sleep(1)
			
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
