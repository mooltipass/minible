from mooltipass_hid_device import *
import threading
import time
import sys
import os
waiting_for_disconnect = False


def device_connect_try_and_recondition():
	global waiting_for_disconnect
	
	# Temporary device constructor
	mooltipass_device = mooltipass_hid_device()
	
	# Try to connect to a new device	
	if mooltipass_device.connect(False) == False:
		return
	
	# Receive status message
	print("Connected to a new USB device")
	time.sleep(3)
	
	# While loop
	while True:
		# Start reconditioning
		nb_secs = mooltipass_device.recondition()
		
		# Check for length
		if nb_secs > 2500:
			print("Reconditioning completed with " + str(nb_secs) + " seconds, stopping there")
			break
		else:
			print("Reconditioning completed with " + str(nb_secs) + " seconds, continuing....")

	# Tell the device to switch off after disconnect
	mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(0x0039, None), True)
	
	# Enable tutorial
	device_settings = mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_GET_DEVICE_SETTINGS, None), True)
	device_settings["data"] = list(device_settings["data"])
	device_settings["data"][16] = 0xFF
	mooltipass_device.device.sendHidMessageWaitForAck(mooltipass_device.getPacketForCommand(CMD_ID_SET_DEVICE_SETTINGS, device_settings["data"]), True)

	# Wait for disconnection (do not change the order below!)
	print("Please disconnect device")
	mooltipass_device.waitForDisconnect()
	waiting_for_disconnect = True
	time.sleep(2)
	waiting_for_disconnect = False

def main():	
	while True:
		if waiting_for_disconnect == False:
			threading.Thread(name="Device connection try thread", target=device_connect_try_and_recondition, daemon=True).start()			
		time.sleep(4)

if __name__ == "__main__":
	main()
