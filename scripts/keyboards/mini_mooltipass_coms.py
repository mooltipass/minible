from mini_keyboard import *
from array import array
from time import sleep
import platform
import os.path
import random
import struct
import string
import copy
import time
import sys
import os

# USB defines
USB_VID                 = 0x16D0
USB_PID                 = 0x09A0
CMD_PING                = 0xA1
CMD_USB_KEYBOARD_PRESS  = 0x9B
CMD_INDEX               = 0x01

# Different imports depending on OS
if platform.system() == "Windows":
	from pywinusb import hid
	using_pywinusb = True
else:
	import hid
	using_pywinusb = False

# Buffer containing the received data, filled asynchronously
pywinusb_received_data = None
data_receiving_object = None
data_sending_object = None

def keyboardSend(data1, data2):
	packetToSend = array('B')
	packetToSend.append(data1)
	packetToSend.append(data2)
	sendHidPacket(CMD_USB_KEYBOARD_PRESS, 0x02, packetToSend)
	receiveHidPacket()
	time.sleep(0.05)

def keyboardTestKey(glyph, encoded_hid_key):
	for i in range(0,4):
		for item in encoded_hid_key:
			modifier = 0x00
			if item & SHIFT_MASK != 0:
				modifier |= KEY_SHIFT
			if item & ALTGR_MASK != 0:
				modifier |= KEY_RIGHT_ALT
			key = item & ~(SHIFT_MASK|ALTGR_MASK)
			if key == KEY_EUROPE_2:
				key = KEY_EUROPE_2_REAL		
			keyboardSend(key, modifier)
	keyboardSend(KEY_RETURN, 0)
	string = input("Testing " + glyph + ": ")
	if (string == ''):
		return string
	elif len(string) < 2:
		return ''
	elif string[0] != string[1]:
		return ''
	else:	
		return string[0]
	
def keyboardCheck(lut):
	perfect_match = True
	sorted_glyphs = sorted(lut)
	for glyph in sorted_glyphs:
		hid_key_array = lut[glyph]
		typed_glyph = keyboardTestKey(chr(glyph), hid_key_array)
		if typed_glyph != chr(glyph):
			print(chr(glyph) + " doesn't match with typed " + typed_glyph)
			perfect_match = False
			input("confirm:")		
			
	return perfect_match

def data_handler(data):
	#print("Raw data: {0}".format(data))
	global pywinusb_received_data
	pywinusb_received_data = data[1:]
		
def receiveHidPacket():
	global pywinusb_received_data
	global data_receiving_object
	if using_pywinusb:
		while pywinusb_received_data == None:
			time.sleep(0.01)
		data_copy = pywinusb_received_data
		pywinusb_received_data = None
		return data_copy
	else:
		try :
			data = data_receiving_object.read(64, timeout_ms=15000)
			return data
		except :
			sys.exit("Mooltipass didn't send a packet")

def sendHidPacket(cmd, length, data):
	if using_pywinusb:
		buffer = [0x00]*65
		buffer[0] = 0
		
		# if command copy it otherwise copy the data
		if cmd != 0:
			buffer[1] = length
			buffer[2] = cmd
			buffer[3:3+len(data)] = data[:]
		else:
			buffer[1:1+len(data)] = data[:]
			
		data_sending_object.set_raw_data(buffer)
		data_sending_object.send()
	else:		
		# data to send
		arraytosend = array('B')

		# if command copy it otherwise copy the data
		if cmd != 0:
			arraytosend.append(length)
			arraytosend.append(cmd)

		# add the data
		if data is not None:
			arraytosend.extend(data)

		#print(arraytosend)
		#print(arraytosend)

		# send data
		data_sending_object.write(arraytosend)
		
def mini_check_lut(test_lut):
	global data_receiving_object
	global data_sending_object
	
	# Main function
	print("")
	print("Mooltipass Keyboard LUT Generation Tool")
	
	if using_pywinusb:
		# Look for our device
		filter = hid.HidDeviceFilter(vendor_id = 0x16d0, product_id = 0x09a0)
		hid_device = filter.get_devices()
		
		if len(hid_device) == 0:
			print("Mooltipass device not found")
			sys.exit(0)
			
		# Open device
		print("Mooltipass device found")
		device = hid_device[0]
		device.open()
		device.set_raw_data_handler(data_handler)		
		report = device.find_output_reports()		
		
		# Set data sending object
		data_sending_object = report[0]
		data_receiving_object = None
	else:
		# Look for our device and open it
		try:
			hid_device = hid.device(vendor_id=0x16d0, product_id=0x09a0)
			hid_device.open(vendor_id=0x16d0, product_id=0x09a0)
		except IOError as ex:
			print(ex)
			sys.exit(0)

		print("Device Found and Opened")
		print("Manufacturer: %s" % hid_device.get_manufacturer_string())
		print("Product: %s" % hid_device.get_product_string())
		print("Serial No: %s" % hid_device.get_serial_number_string())
		print("")
		
		# Set data sending object
		data_sending_object = hid_device
		data_receiving_object = hid_device
		
	sendHidPacket(CMD_PING, 4, [0,1,2,3])
	if receiveHidPacket()[CMD_INDEX] == CMD_PING:
		print("Device responded to our ping")
	else:
		print("Bad answer to ping")
		sys.exit(0)
	
	#keyboardTest(data_sending_object)
	return_val = keyboardCheck(test_lut)
	
	# Close device
	if not using_pywinusb:
		data_sending_object.close()
	else:
		device.close()
		
	return return_val

if __name__ == '__main__':
	main_function()

