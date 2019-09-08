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
	
def keyboardCheck(lut, transforms):
	perfect_match = True
	sorted_glyphs = sorted(lut)
	for glyph in sorted_glyphs:
		hid_key_array = lut[glyph]
		print("Glyph " + glyph + " with modifier " + hex(hid_key_array[0][0]) + " and hid key " + hex(hid_key_array[0][1]))
		typed_glyph = keyboardTestKey(hid_key_array)
		if typed_glyph != glyph:
			print(glyph + " doesn't match with typed " + typed_glyph)
			perfect_match = False
			input("confirm:")		

	print("Tackling transforms")
	sorted_transforms = sorted(transforms)
	for transform in sorted_transforms:
		key_sequence = []
		for glyph in transforms[transform]:
			for lut_item in lut[glyph]:
				# This script allows multiple lut items but the main script doesnt
				key_sequence.append(lut_item)		
		print("Transform glyph " + transform)
		typed_glyph = keyboardTestKey(key_sequence)
		if typed_glyph != transform:
			print(transform + " doesn't match with typed " + typed_glyph)
			perfect_match = False
			input("confirm:")	
	return perfect_match

def keyboardTestKey(modifier_key_array):
	for i in range(0,4):
		for item in modifier_key_array:
			if(item[1] in KEYTEST_BAN_LIST): return ''
			keyboardSend(item[1], item[0])
	keyboardSend(KEY_RETURN, 0)
	string = input()
	if (string == ''):
		return string
	elif len(string) < 2:
		return ''
	elif string[0] != string[1]:
		return ''
	else:	
		return string[0]

def keyboardKeyMap(key):
	if ( (key & 0x3F) == KEY_EUROPE_2 ):
		if (key & (SHIFT_MASK|ALTGR_MASK) == (SHIFT_MASK|ALTGR_MASK)):
			return keyboardTestKey(KEY_EUROPE_2_REAL, KEY_SHIFT|KEY_RIGHT_ALT)
		elif (key & SHIFT_MASK):
			return keyboardTestKey(KEY_EUROPE_2_REAL, KEY_SHIFT)
		elif (key & ALTGR_MASK):
			return keyboardTestKey(KEY_EUROPE_2_REAL, KEY_RIGHT_ALT)
		else:
			return keyboardTestKey(KEY_EUROPE_2_REAL, 0)

	elif (key & (SHIFT_MASK|ALTGR_MASK) == (SHIFT_MASK|ALTGR_MASK)):
		return keyboardTestKey(key & ~(SHIFT_MASK|ALTGR_MASK), KEY_SHIFT|KEY_RIGHT_ALT)
			
	elif (key & SHIFT_MASK):
		return keyboardTestKey(key & ~SHIFT_MASK, KEY_SHIFT)

	if (key & ALTGR_MASK):
		return keyboardTestKey(key & ~ALTGR_MASK, KEY_RIGHT_ALT)

	else:
		return keyboardTestKey(key, 0)

def keyboardTest():
	fileName = input("Name of the Keyboard (example: ES): ");

	# dictionary to store the
	Layout_dict = {}

	# No modifier combinations
	for bruteforce in range(KEY_EUROPE_2, KEY_SLASH+1):
		output = keyboardKeyMap(bruteforce)
		if (output == ''): continue
		if output in Layout_dict:
			print("'" + output + "'" + " already stored")
			Layout_dict[output].append(bruteforce)
		else:
			Layout_dict[output] = []
			Layout_dict[output].append(bruteforce)

	# SHIFT combinations
	for bruteforce in range(KEY_EUROPE_2, KEY_SLASH+1):
		output = keyboardKeyMap(SHIFT_MASK|bruteforce)
		if (output == ''): continue
		if output in Layout_dict:
			print("'" + output + "'" + " already stored")
			Layout_dict[output].append(SHIFT_MASK|bruteforce)
		else:
			Layout_dict[output] = []
			Layout_dict[output].append(SHIFT_MASK|bruteforce)

	# ALTGR combinations
	for bruteforce in range(KEY_EUROPE_2, KEY_SLASH+1):
		output = keyboardKeyMap(ALTGR_MASK|bruteforce)
		if (output == ''): continue
		if output in Layout_dict:
			print("'" + output + "'" + " already stored")
			Layout_dict[output].append(ALTGR_MASK|bruteforce)
		else:
			Layout_dict[output] = []
			Layout_dict[output].append(ALTGR_MASK|bruteforce)

	# ALTGR + SHIFT combinations
	for bruteforce in range(KEY_EUROPE_2, KEY_SLASH+1):
		output = keyboardKeyMap(SHIFT_MASK|ALTGR_MASK|bruteforce)
		if (output == ''): continue
		if output in Layout_dict:
			print("'" + output + "'" + " already stored")
			Layout_dict[output].append(SHIFT_MASK|ALTGR_MASK|bruteforce)
		else:
			Layout_dict[output] = []
			Layout_dict[output].append(SHIFT_MASK|ALTGR_MASK|bruteforce)


	hid_define_str = "const uint8_t PROGMEM keyboardLUT_"+fileName+"[95] = \n{\n"
	img_contents = array('B')
	final_keyb_dict = {}

	for key in KeyboardAscii:
		if(key not in Layout_dict):
			#print(key + " Not found"
			final_keyb_dict[key] = 0
			Layout_dict[key] = [0]
		#else:
			#print("BruteForced: " + key
			
		if len(Layout_dict[key]) > 1:
			print("Multiple keys for '" + key + "'")
			i = 0
			for x in Layout_dict[key]:
				if x & (SHIFT_MASK|ALTGR_MASK) == (SHIFT_MASK|ALTGR_MASK):
					print(str(i) + ": Shift + Altgr + ", key_val_to_key_text[x & ~SHIFT_MASK & ~ALTGR_MASK])
				elif x & (SHIFT_MASK) == (SHIFT_MASK):
					print(str(i) + ": Shift + ", key_val_to_key_text[x & ~SHIFT_MASK & ~ALTGR_MASK])
				elif x & (ALTGR_MASK) == (ALTGR_MASK):
					print(str(i) + ": Altgr + ", key_val_to_key_text[x & ~SHIFT_MASK & ~ALTGR_MASK])
				else:
					print(str(i) + ": " + key_val_to_key_text[x & ~SHIFT_MASK & ~ALTGR_MASK])
				i += 1
				
			choice = input("Please select correct combination: ")
		else:
			choice = 0
			
		# Store choice
		final_keyb_dict[key] = Layout_dict[key][choice]
				
	# Double check our generated LUT
	print("Double checking keys...")
	for key in KeyboardAscii:
		output = keyboardKeyMap(final_keyb_dict[key])
		if output != key:
			print("Non matching keys:", "\"" + output + "\"", "instead of \"" + key + "\"")
			return
	print("Check complete!")
		
	
	# Generate expot file
	for key in KeyboardAscii:
		""" Format C code """
		keycode = hex(final_keyb_dict[key])+","

		# Write img file
		img_contents.append(final_keyb_dict[key])

		# Handle special case
		if(key == '\\'):
			comment = " // " + hex(ord(key)) + " '" + key + "'\n"
		else:
			comment = " // " + hex(ord(key)) + " " + key + "\n"

		newline = "%4s%-5s" % (" ", keycode) + comment
		# add new line into existing string
		hid_define_str = hid_define_str + newline

	# finish C array
	hid_define_str = hid_define_str + "};"
	print(hid_define_str)

	# finish img file
	img_file = open("_"+fileName+"_keyb_lut.img", "wb")
	img_file.write(img_contents)
	img_file.close()

	# Save C array into .c file
	text_file = open("keymap_"+fileName+".c", "w")
	text_file.write(hid_define_str)
	text_file.close()

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
		
def mini_check_lut(test_lut, test_transforms):
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
	return_val = keyboardCheck(test_lut, test_transforms)
	
	# Close device
	if not using_pywinusb:
		data_sending_object.close()
	else:
		device.close()
		
	return return_val

if __name__ == '__main__':
	main_function()

