from __future__ import print_function
from os.path import isfile, join, isdir
from resizeimage import resizeimage
from mooltipass_defines import *
from generic_hid_device import *
from datetime import timezone
from pprint import pprint
from array import array
from PIL import Image
import struct
import random
import time
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
		packet["cmd"].frombytes(struct.pack('H', cmd))
		
		# data length
		if data is not None:
			packet["len"].frombytes(struct.pack('H', len(data)))
		else:
			packet["len"].frombytes(struct.pack('H', 0))

		# data
		if data is not None:
			packet["data"].extend(data)
			
		return packet

	# Create a ping packet to be sent to the device
	def createPingPacket(self):	
		return self.getPacketForCommand(CMD_PING, [random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)])
		
	# Initialization code
	def initCode(self):
		# Ask for the info
		packet = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_ID_PLAT_INFO, None), True)	
		print("device SN:", struct.unpack('I', packet["data"][8:12])[0])
		
	# Get platform information
	def getPlatformInfo(self):
		# Ask for the info
		packet = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_ID_PLAT_INFO, None), True)	
		
		# Prepare reply
		reply_dict = {}
		reply_dict["sn"] = struct.unpack('I', packet["data"][8:12])[0]
		reply_dict["main_maj"] = struct.unpack('H', packet["data"][0:2])[0]
		reply_dict["main_min"] = struct.unpack('H', packet["data"][2:4])[0]
		reply_dict["aux_maj"] = struct.unpack('H', packet["data"][4:6])[0]
		reply_dict["aux_min"] = struct.unpack('H', packet["data"][6:8])[0]
		reply_dict["mem"] = struct.unpack('H', packet["data"][12:14])[0]
		reply_dict["bundle"] = struct.unpack('H', packet["data"][14:16])[0]
		return reply_dict
		
	# Send full frame picture to display
	def sendAndMonitorFrame(self, filename, bitdepth):	
		# Check for file
		if not isfile(filename):
			print("File \"" + filename + "\" does not exist")
			return	
			
		# monitor modified time
		modified_time = 0	
			
		while True:
			# modified time change
			if modified_time != os.stat(filename).st_mtime:
				# Store modified time
				time.sleep(0.1)
				modified_time = os.stat(filename).st_mtime
				
				# Create new image
				full_screen = Image.new("RGB", [256,64], "black")
				full_screen = full_screen.convert(mode="RGB", colors=256)
			
				# Open image
				image = Image.open(filename)
				image = image.convert(mode="RGB", colors=256) 
				
				# Get image specs
				img_format = image.format
				img_size = image.size
				img_mode = image.mode
				#print("Format:", img_format, "size:", img_size, "mode:", img_mode
				
				# Check size
				if img_size[0] > 256 or img_size[1] > 64:
					print("Picture too big for display")
					return
					
				# Paste image on top of full screen
				full_screen.paste(image)
				
				# Special case: 48x48 icons: display smaller versions of them next to them
				if img_size[0] == 48 or img_size[1] == 48:
					smaller_image = resizeimage.resize_cover(image, [36, 36])
					full_screen.paste(smaller_image, [100,0])
					smaller_image = resizeimage.resize_cover(image, [32, 32])
					full_screen.paste(smaller_image, [150,0])
					smaller_image = resizeimage.resize_cover(image, [24, 24])
					full_screen.paste(smaller_image, [200,0])
				
				# Initialize vars
				packet_payload = []
				packet_pixel_goal = 530
				
				# Send open buffer to usb
				start_time = time.time()
				print("Sending picture to display...")
				self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_OPEN_DISP_BUFFER, None))
				
				# Loop through the pixels
				for y in range(0, full_screen.size[1]):
					for x in range(0, full_screen.size[0]/2):
						if False:
							pix1 = int(((255 - full_screen.getpixel((x*2, y))[0] + 0) / math.pow(2, 8-bitdepth))*math.pow(2, 4-bitdepth))
						else:
							pix1 = int(((full_screen.getpixel((x*2, y))[0] + 0) / math.pow(2, 8-bitdepth))*math.pow(2, 4-bitdepth))
						if False:
							pix2 = int(((255 - full_screen.getpixel((x*2+1, y))[0] + 0) / math.pow(2, 8-bitdepth))*math.pow(2, 4-bitdepth))
						else:
							pix2 = int(((full_screen.getpixel((x*2+1, y))[0] + 0) / math.pow(2, 8-bitdepth))*math.pow(2, 4-bitdepth))
						
						# Append to current payload
						packet_payload.append((pix1 << 4) | pix2)
						
						# Do we need to send buffer?
						if len(packet_payload) == packet_pixel_goal:
							self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_SEND_TO_DISP_BUFFER, packet_payload))
							packet_payload = []

				# Send remaining pixels
				if len(packet_payload) != 0:
					self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_SEND_TO_DISP_BUFFER, packet_payload))
				
				# Data sent, close buffer
				self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_CLOSE_DISP_BUFFER, None))		
				end_time = time.time()
				print("Picture sent in " + str(int((end_time-start_time)*1000)) + "ms")
				
			else:
				# No modifications, sleep
				time.sleep(0.2)
				
	# Send and update platform
	def uploadAndUpgradePlatform(self, filename, password):
		# Check for file
		if not isfile(filename):
			print("File \"" + filename + "\" does not exist")
			return False
			
		# Transform password
		password = bytearray.fromhex(password)
		if len(password) != 16:
			print("Password has an incorrect size")
			return False
			
		# Open file
		bundlefile = open(filename, 'rb')
				
		# Send erase dataflash command to usb
		start_time = time.time()
		print("Sending start upload command..")
		if self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_ID_START_BUNDLE_UL, password))["data"][0] == CMD_HID_ACK:
			print("Password accepted, starting upload...")
		else:
			print("Incorrect password")
			return False
		
		# First 4 bytes: address for writing, start reading bytes
		byte = bundlefile.read(1)
		current_ts = int(time.time())
		number_of_packets_last_second = 0
		current_address = 0
		bytecounter = 4
		
		# Prepare first packet to send
		packet_to_send = self.getPacketForCommand(CMD_ID_WRITE_BUNDLE_256B, None)
		packet_to_send["data"].frombytes(struct.pack('I', current_address))
		
		# While we haven't finished looping through the bytes
		while len(byte) != 0:
			# Add byte to current packet
			packet_to_send["data"].append(struct.unpack('B', byte)[0])
			# Increment byte counter
			bytecounter = bytecounter + 1
			# Read new byte
			byte = bundlefile.read(1)
			# If packet full, send it
			if bytecounter == 256+4:
				# Set correct payload size and send packet
				packet_to_send["len"] = array('B')
				packet_to_send["len"].frombytes(struct.pack('H', bytecounter))
				self.device.sendHidMessageWaitForAck(packet_to_send)
				number_of_packets_last_second += 1
				# Reset byte counter, increment address
				current_address += 256
				bytecounter = 4
				# Prepare new packet to send
				packet_to_send = self.getPacketForCommand(CMD_ID_WRITE_BUNDLE_256B, None)
				packet_to_send["data"].frombytes(struct.pack('I', current_address))
				# Progress counter
				if current_address == 262144:
					print("25%")
				elif current_address == 262144*2:
					print("50%")
				elif current_address == 262144*3:
					print("75%")
			# Upload stats
			if int(time.time()) != current_ts:
				print(str(number_of_packets_last_second*256) + " bytes per second")
				number_of_packets_last_second = 0
				current_ts = int(time.time())
					
		# Send the remaining bytes if needed
		if bytecounter != 4 + 0:
			packet_to_send["len"] = array('B')
			packet_to_send["len"].frombytes(struct.pack('H', bytecounter))
			self.device.sendHidMessageWaitForAck(packet_to_send)
		
		# Let the device know we're done
		print("Bundle upload done!")
		self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_ID_END_BUNDLE_UL, None))		
		
		# Close file
		bundlefile.close()
		print("Sending done!")	
		return True
		
	def authenticationChallenge(self, challenge):		
		# Try our luck
		answer = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_ID_AUTH_CHALLENGE, challenge))
		if answer["len"] == 1:
			return False
		else:
			return answer["data"]
			
	def printDiagData(self):
		packet = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_ID_GET_DIAG_DATA, None))
		total_nb_ms_screen_on = struct.unpack('I', packet["data"][0:4])[0] << 32
		total_nb_ms_screen_on += struct.unpack('I', packet["data"][4:8])[0]
		total_nb_30mins_bat_on = struct.unpack('I', packet["data"][8:12])[0]
		total_nb_30mins_usb_on = struct.unpack('I', packet["data"][12:16])[0]		
		print("Total ms with screen on: " + str(total_nb_ms_screen_on))	
		print("Total 30mins battery powered: " + str(total_nb_30mins_bat_on))
		print("Total 30mins USB powered: " + str(total_nb_30mins_usb_on))

	# Send bundle to display
	def uploadDebugBundle(self, filename):	
		# Check for file
		if not isfile(filename):
			print("File \"" + filename + "\" does not exist")
			return	
			
		# Open file
		bundlefile = open(filename, 'rb')
				
		# Send erase dataflash command to usb
		start_time = time.time()
		print("Sending erase dataflash command..")
		self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_ERASE_DATA_FLASH, None))
		
		# Wait for erase done
		print("Waiting for erase done...")
		while True:
			time.sleep(.1)
			if self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_IS_DATA_FLASH_READY, None))["data"][0] == CMD_HID_ACK:
				break;
		print("Erase done in " + str(int((time.time()-start_time)*1000)) + "ms")
		print("Sending bundle data...")
		
		# First 4 bytes: address for writing, start reading bytes
		byte = bundlefile.read(1)
		current_address = 0
		bytecounter = 4
		
		# Prepare first packet to send
		packet_to_send = self.getPacketForCommand(CMD_DBG_DATAFLASH_WRITE_256B, None)
		packet_to_send["data"].frombytes(struct.pack('I', current_address))
		
		# While we haven't finished looping through the bytes
		while byte != '':
			# Add byte to current packet
			packet_to_send["data"].append(struct.unpack('B', byte)[0])
			# Increment byte counter
			bytecounter = bytecounter + 1
			# Read new byte
			byte = bundlefile.read(1)
			# If packet full, send it
			if bytecounter == 256+4:
				# Set correct payload size and send packet
				packet_to_send["len"] = array('B')
				packet_to_send["len"].frombytes(struct.pack('H', bytecounter))
				self.device.sendHidMessageWaitForAck(packet_to_send)
				# Reset byte counter, increment address
				current_address += 256
				bytecounter = 4
				# Prepare new packet to send
				packet_to_send = self.getPacketForCommand(CMD_DBG_DATAFLASH_WRITE_256B, None)
				packet_to_send["data"].frombytes(struct.pack('I', current_address))
				# Leave enough time for flash to burn bytes
				time.sleep(0.002)
					
		# Send the remaining bytes
		packet_to_send["len"] = array('B')
		packet_to_send["len"].frombytes(struct.pack('H', bytecounter))
		self.device.sendHidMessageWaitForAck(packet_to_send)
		
		# Let the device know to reindex bundle
		print("Letting the device know to reindex bundle...")
		self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_REINDEX_BUNDLE, None))		
		
		# Close file
		bundlefile.close()
		print("Sending done!")
		
	
	# Reboot to bootloader, no answer from device.
	def rebootToBootloader(self):
		self.device.sendHidMessage(self.getPacketForCommand(CMD_DBG_REBOOT_TO_BOOTLOADER, None))	
		
		
	# Flash the aux MCU from the bundle contents, no answer from device.
	def flashAuxMcuFromBundle(self):
		self.device.sendHidMessage(self.getPacketForCommand(CMD_DBG_FLASH_AUX_MCU, None))	
		
		
	# Get device status
	def getDeviceStatus(self):
		# Ask for the info
		packet = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_GET_DEVICE_STATUS, None))	
	
		# Get raw data
		status_byte = packet["data"][0]
		battery_pct = packet["data"][1]
		user_sec_preferences = struct.unpack('H', packet["data"][2:4])[0]
		settings_changed_flag = packet["data"][4]
		
		# Create answer dict
		answer_dict = {}
		answer_dict["battery_pct"] = battery_pct*10
		answer_dict["no_bundle"] = True if status_byte & 0x20 != 0 else False
		
		# Return status!
		return answer_dict
		
		
	# Plot time difference with time
	def timeDiff(self):
		# log file
		f = open("timelog.txt", "w")
	
		ts_start = round(datetime.timestamp(datetime.now(tz=timezone.utc)))
		nb_of_s_since_last_ts_diff_change = 0
		last_ts_diff_seen = 0
		ts_diff_offset = None
		last_ts = ts_start
		
		while True:
			if last_ts != round(datetime.timestamp(datetime.now(tz=timezone.utc))):				
				# Ask the device time and get local time
				packet = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(HID_CMD_ID_GET_PLAT_TIME, None))	
				utc_now = datetime.now(tz=timezone.utc)
				
				# Handle please retry message
				if packet["cmd"] != CMD_ID_RETRY and packet["cmd"] != CMD_GET_DEVICE_STATUS:			
					# Create timestamps
					device_timestamp = struct.unpack('I', packet["data"][0:4])[0]
					utc_timestamp = round(datetime.timestamp(utc_now))
					ts_difference = utc_timestamp - device_timestamp
					
					# Store offset
					if ts_diff_offset is None:
						ts_diff_offset = ts_difference
						
					# Remove offset from difference
					ts_difference -= ts_diff_offset
					
					if False:
						print("Current UTC time: " + str(utc_now.hour) + "h" + str(utc_now.minute) + "m" + str(utc_now.second) + "s " + str(utc_now.day) + "/" + str(utc_now.month) + "/" + str(utc_now.year))
						print("Current device time: " + str(device_time.hour) + "h" + str(device_time.minute) + "m" + str(device_time.second) + "s " + str(device_time.day) + "/" + str(device_time.month) + "/" + str(device_time.year))
						print("Current timestamp: " + str(utc_timestamp))
						print("Device timestamp: " + str(device_timestamp))
						print("Timestamp difference: " + str(ts_difference))
					
					# Write log
					f.write(str(utc_timestamp-ts_start) + "," + str(ts_difference) + "\r")
					f.flush()
					
					# Stats
					if ts_difference != last_ts_diff_seen:
						print("New ts difference: " + str(ts_difference) + ", had been " + str(nb_of_s_since_last_ts_diff_change) + "s since the last change")
					
						# Store new difference
						last_ts_diff_seen = ts_difference
						nb_of_s_since_last_ts_diff_change = 0
					else:
						nb_of_s_since_last_ts_diff_change += 1
					
					# Store last seen timestamp
					last_ts = round(datetime.timestamp(datetime.now(tz=timezone.utc)))
				else:
					time.sleep(0.1)				
			else:
				time.sleep(0.1)
				
		f.close()
				

	# Get platform info
	def getPlatInfo(self):
		# Ask for the info
		packet = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_GET_PLAT_INFO, None))	
	
		# print(it!
		print("")
		print("Platform Info")
		print("Aux MCU major:", struct.unpack('H', packet["data"][0:2])[0])
		print("Aux MCU minor:", struct.unpack('H', packet["data"][2:4])[0])
		print("Main MCU major:", struct.unpack('H', packet["data"][64:66])[0])
		print("Main MCU minor:", struct.unpack('H', packet["data"][66:68])[0])
		
	def getRandomData(self, nb_bytes_requested):
		nb_bytes_gotten = 0
		return_array = []
		
		while nb_bytes_gotten < nb_bytes_requested:
			nb_bytes_to_get = nb_bytes_requested - nb_bytes_gotten
			
			if nb_bytes_to_get > 32:
				nb_bytes_to_get = 32
				
			# Ask for random bytes
			return_array.extend(self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_GET_RANDOM_32B, None))["data"][0:nb_bytes_to_get])
			
			# Update counter
			nb_bytes_gotten += nb_bytes_to_get

		if False:
			print(''.join('{:02x}'.format(x) for x in return_array))
			
		return return_array

	def flashUniqueData(self):
		device_sn = input("Device serial number: ")
		# First 128B: random data
		data_to_flash = self.getRandomData(128)
		# Device serial number
		data_to_flash.extend(struct.pack('I', int(device_sn)))
		# mac address
		data_to_flash.append(int(device_sn))
		data_to_flash.append(0x00)
		data_to_flash.append(0x30)
		data_to_flash.append(0x12)
		data_to_flash.append(0x79)
		data_to_flash.append(0x68)
		# bundle version
		data_to_flash.extend(struct.pack('H', 0))
		# nothing
		data_to_flash.extend([0]*116)
		
		# Debug
		print()
		print("Device ID: " + str(device_sn))
		print("Signing key: " + ''.join('{:02x}'.format(x) for x in data_to_flash[0:32]))
		print("Device operations key: " + ''.join('{:02x}'.format(x) for x in data_to_flash[32:64]))
		print("Available key: " + ''.join('{:02x}'.format(x) for x in data_to_flash[64:96]))
		print("Other available key: " + ''.join('{:02x}'.format(x) for x in data_to_flash[96:128]))
		print("MAC Addr: " + ''.join('{:02x}'.format(x) for x in data_to_flash[132:138]))
		print()
		
		# Send data
		self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_FLASH_PLAT_UNIQUE_DATA, data_to_flash))
		
	# Get accelerometer data
	def getAccData(self):
		# Random bytes file
		f = open('randombytes.bin', 'wb')
		
		# local vars
		current_second = datetime.now().second
		data_counter = 0
		accXSamples = []
		accYSamples = []
		accZSamples = []
		
		# RNG parameters
		bits_for_rng = 5
		extraction_mask = (1 << bits_for_rng) - 1
		# RNG local vars
		current_word = 0x00
		bit_in_word = 0
		
		while True:
			# Ask for 32 samples
			packet = self.device.sendHidMessageWaitForAck(self.getPacketForCommand(CMD_DBG_GET_ACC_32_SAMPLES, None))	
			
			# Debug print
			if False:
				for i in range(0, len(packet["data"])/6):
					print("X: " + hex(packet["data"][i*6+0] + packet["data"][i*6+1]*256)[2:].zfill(4) + " Y: " + hex(packet["data"][i*6+2] + packet["data"][i*6+3]*256)[2:].zfill(4) + " Z: " + hex(packet["data"][i*6+4] + packet["data"][i*6+5]*256)[2:].zfill(4))
			
			# Store samples
			for i in range(0, len(packet["data"])/6):
					#accXSamples.append(packet["data"][i*6+0] + packet["data"][i*6+1]*256)
					#accYSamples.append(packet["data"][i*6+2] + packet["data"][i*6+3]*256)
					#accZSamples.append(packet["data"][i*6+4] + packet["data"][i*6+5]*256)
					data_counter += 3
					
					# Write random data
					for number in [packet["data"][i*6+0], packet["data"][i*6+2], packet["data"][i*6+4]]:
						# Apply mask
						number = number & extraction_mask
						
						# Check if we have overspill
						overspill = bit_in_word + bits_for_rng - 8
						
						if overspill >= 0:
							current_word = (current_word | (number << bit_in_word)) & 0x0FF
							f.write(struct.pack('B', current_word))
							current_word = number >> (bits_for_rng - overspill)
							bit_in_word = overspill
						else:
							current_word = current_word | (number << bit_in_word)
							bit_in_word += bits_for_rng
			
			# print(out performance
			if current_second != datetime.now().second:
				current_second = datetime.now().second
				print("Accelerometer receive:", data_counter , "samples/s")
				data_counter = 0
				f.flush()
		
		
	### BELOW ARE FEATURES TO RE-IMPLEMENT	
			
	# Ping device
	def pingMooltipass(self):
		ping_packet = self.createPingPacket()
		self.device.sendHidPacket(ping_packet)
		ping_answer = self.device.receiveHidPacketWithTimeout()
		return self.checkPingAnswerPacket(ping_packet, ping_answer)
			
	# Get the Mooltipass version, returns [Flash Mb, Version, Variant]
	def getMooltipassVersionAndVariant(self):
		self.device.sendHidPacket([0, CMD_VERSION]);
		version_data = self.device.receiveHidPacket()[DATA_INDEX:]
		# extract interesting data
		nbMb = version_data[0]
		version = "".join(map(chr, version_data[1:])).split(b"\x00")[0]
		variant = "standard"
		if "_" in version:
			variant = version.split('_', 1)[1]
			version = version.split('_', 1)[0]
		return [nbMb, version, variant]
		
	# Get the Mooltipass Mini Serial number (for kickstarter versions only)
	def getMooltipassMiniSerial(self):
		self.device.sendHidPacket([0, CMD_GET_MINI_SERIAL]);
		serial_data = self.device.receiveHidPacket()[DATA_INDEX:DATA_INDEX+4]
		return serial_data
		
	# Get the Mooltipass UID
	def getUID(self, req_key):
		key = array('B', req_key.decode("hex"))
		if len(key) != UID_REQUEST_KEY_SIZE:
			print("Wrong UID Req Key Length:", len(key))
			return None
			
		# send packet
		self.device.sendHidPacket(self.getPacketForCommand(CMD_GET_UID, UID_REQUEST_KEY_SIZE, key))
		data = self.device.receiveHidPacket()
		if data[LEN_INDEX] == 1:
			print("Couldn't get UID (wrong password?)")
			return None
		else:
			return data[DATA_INDEX:DATA_INDEX+data[LEN_INDEX]]
		
	# Get the user database change db
	def getMooltipassUserDbChangeNumber(self):
		self.device.sendHidPacket([0, CMD_GET_USER_CHANGE_NB])
		user_change_db_answer = self.device.receiveHidPacket()[DATA_INDEX:]
		if user_change_db_answer[0] == 0:
			print("User not logged in!")
		else:
			print("User db change number is", user_change_db_answer[1], "and", user_change_db_answer[2])
			
	def setMooltipassUserDbChangeNumber(self, number, number2):
		# Go to MMM
		self.device.sendHidPacket([0, CMD_START_MEMORYMGMT])
		data = self.device.receiveHidPacket()[DATA_INDEX:]
		if data[0] == 0:
			print("Couldn't go to MMM!")
			return		
		
		self.device.sendHidPacket([2, CMD_SET_USER_CHANGE_NB, number, number2])
		user_change_db_answer = self.device.receiveHidPacket()[DATA_INDEX:]
		if user_change_db_answer[0] == 0:
			print("Fail!")
		else:
			print("Success!")
			
		# Leave MMM
		self.device.sendHidPacket([0, CMD_END_MEMORYMGMT])
		data = self.device.receiveHidPacket()[DATA_INDEX:]		

	# Query the Mooltipass status
	def getMooltipassStatus(self):
		self.device.sendHidPacket([0, CMD_MOOLTIPASS_STATUS])
		status_data = self.device.receiveHidPacket()
		if status_data[DATA_INDEX] == 0:
			return "No card in Mooltipass"
		elif status_data[DATA_INDEX] == 1:
			return "Mooltipass locked"
		elif status_data[DATA_INDEX] == 3:
			return "Mooltipass locked, unlocking screen"
		elif status_data[DATA_INDEX] == 5:
			return "Mooltipass unlocked"
		elif status_data[DATA_INDEX] == 9:
			return "Unknown smartcard inserted"
			
	# Send custom packet
	def sendCustomPacket(self):
		command = raw_input("CMD ID: ")
		packet = array('B')
		temp_bool = 0
		length = 0

		#fill packet
		packet.append(0)
		packet.append(int(command, 16))

		#loop until packet is filled
		while temp_bool == 0 :
			try :
				intval = int(raw_input("Byte %s: " %length), 16)
				packet.append(intval)
				length = length + 1
			except ValueError :
				temp_bool = 1

		#update packet length
		packet[0] = length

		#ask for how many packets to receiveHidPacket
		packetstoreceive = input("How many packets to be received: ")

		#send packet
		self.device.sendHidPacket(packet)

		#receive packets
		for i in range(0, packetstoreceive):
			received_data = self.device.receiveHidPacket()
			print("Packet #" + str(i) + " in hex: " + ' '.join(hex(x) for x in received_data))
			
	# Get number of free slots for new users
	def getFreeUserSlots(self):
		self.device.sendHidPacket([0, CMD_GET_FREE_NB_USR_SLT])
		print(self.device.receiveHidPacket()[DATA_INDEX], "slots for new users available")
		
	# Enable tutorial
	def enableTutorial(self):
		self.device.sendHidPacket([2, CMD_SET_MOOLTIPASS_PARM, 16, 1])
		self.device.receiveHidPacket()[DATA_INDEX]	
		
	# Get card CPZ (can only be queried when inserted card is unknown)
	def getCardCpz(self):
		self.device.sendHidPacket([0, CMD_GET_CUR_CPZ])
		data = self.device.receiveHidPacket()
		if data[LEN_INDEX] == 1:
			print("Couldn't query CPZ")
		else:
			print("CPZ: " + ' '.join(hex(x) for x in data[DATA_INDEX:DATA_INDEX+data[LEN_INDEX]]))
		
	# Lock device
	def lock(self):
		self.device.sendHidPacket([0, CMD_LOCK_DEVICE])
		self.device.receiveHidPacket()
		
	# Get card credentials:
	def getCardCreds(self):		
		self.device.sendHidPacket([0, CMD_READ_CARD_CREDS])
		data = self.device.receiveHidPacket()
		if data[LEN_INDEX] == 1:
			print("Couldn't get card credentials!")
		else:
			data[len(data)-1] = 0
			print("Login:", self.getTextFromUsbPacket(data))
			data = self.device.receiveHidPacket()
			data[len(data)-1] = 0
			print("Password:", self.getTextFromUsbPacket(data)	)	
	
	# Set card login
	def setCardLogin(self, login):
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_CARD_LOGIN, len(login)+1, self.textToByteArray(login)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set login")
			return
	
	# Set card password
	def setCardPassword(self, password):
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_CARD_PASS, len(password)+1, self.textToByteArray(password)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set password")
			return
		
	# Change description for given login
	def changeLoginDescription(self):
		context = raw_input("Context: ")
		login = raw_input("Login: ")
		desc = raw_input("Description: ")
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_CONTEXT, len(context)+1, self.textToByteArray(context)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set context")
			return
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_LOGIN, len(login)+1, self.textToByteArray(login)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set login")
			return
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_DESCRIPTION, len(desc)+1, self.textToByteArray(desc)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set description")
			return
		
		print("Description changed!")
		
	# Add new credential
	def addCredential(self):
		context = raw_input("Context: ")
		login = raw_input("Login: ")
		desc = raw_input("Description: ")
		password = raw_input("Password: ")
				
		self.device.sendHidPacket(self.getPacketForCommand(CMD_ADD_CONTEXT, len(context)+1, self.textToByteArray(context)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set context")
			return
			
		self.device.sendHidPacket(self.getPacketForCommand(CMD_CONTEXT, len(context)+1, self.textToByteArray(context)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set context")
			return
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_LOGIN, len(login)+1, self.textToByteArray(login)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set login")
			return
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_DESCRIPTION, len(desc)+1, self.textToByteArray(desc)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set description")
			return
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_PASSWORD, len(password)+1, self.textToByteArray(password)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set password")
			return			
		
		print("Done!")
		
	# Test please retry MPM feature
	def testPleaseRetry(self):
		context = raw_input("Context: ")
			
		self.device.sendHidPacket(self.getPacketForCommand(CMD_CONTEXT, len(context)+1, self.textToByteArray(context)))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x00:
			print("Couldn't set context")
			return
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_GET_LOGIN, 0, None))
		print("Don't touch the device!")
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_GET_LOGIN, 0, None))
		data = self.device.receiveHidPacket()
		if data[CMD_INDEX] == CMD_PLEASE_RETRY:
			print("Please retry received!")
			
			self.device.sendHidPacket(self.getPacketForCommand(CMD_CANCEL_REQUEST, 0, None))
			print("Cancel request sent!")
			data = self.device.receiveHidPacket()			
		else:
			print("Didn't receive please retry!")
			print("Received data:", ' '.join(hex(x) for x in data))
			
	# Upload bundle
	def	uploadBundle(self, password, filename, verbose):	
		password_set = False
		mooltipass_variant = self.getMooltipassVersionAndVariant()[2]
		
		# Check if a file name was passed
		if filename is not None:
			if not os.path.isfile(filename):
				print("Filename passed as arg isn't valid")
				filename = None
			else:
				bundlefile = open(filename, 'rb')
			
		# List available img files
		if filename is None:
			file_list = glob.glob("*.img")
			if len(file_list) == 0:
				print("No img file available!")
				return False
			elif len(file_list) == 1:
				print("Using bundle file", file_list[0])
				bundlefile = open(file_list[0], 'rb')
			else:
				for i in range(0, len(file_list)):
					print(str(i) + ": " + file_list[i])
				picked_file = raw_input("Choose file: ")
				if int(picked_file) >= len(file_list):
					print("Out of bounds")
					return False
				else:
					bundlefile = open(file_list[int(picked_file)], 'rb')
		
		# Ask for Mooltipass password
		if password is None:
			mp_password = raw_input("Enter Mooltipass Password, press enter if None: ")
			if len(mp_password) == DEVICE_PASSWORD_SIZE*2 or len(mp_password) == MINI_DEVICE_PASSWORD_SIZE*2:
				password_len = len(mp_password)/2
				password_set = True
			else:
				print("Empty or erroneous password, using zeros")
		else:
			if len(password) == DEVICE_PASSWORD_SIZE*2 or len(password) == MINI_DEVICE_PASSWORD_SIZE*2:
				password_len = len(password)/2
				mp_password = password
				password_set = True
			else:
				print("Erroneous password length for password:", len(password)/2)
				return False
		
		# Prepare the password
		if verbose == True:
			print("Starting upload...")
		mooltipass_password = array('B')
		if mooltipass_variant == "mini":
			mooltipass_password.append(MINI_DEVICE_PASSWORD_SIZE)
		else:
			mooltipass_password.append(DEVICE_PASSWORD_SIZE)
		mooltipass_password.append(CMD_IMPORT_MEDIA_START)
		if password_set:
			for i in range(password_len):
				mooltipass_password.append(int(mp_password[i*2:i*2+2], 16))
		else:
			for i in range(DEVICE_PASSWORD_SIZE):
				mooltipass_password.append(0)
			
		# Success status boolean
		success_status = False
		
		# Send import media start packet
		self.device.sendHidPacket(mooltipass_password)
		
		# Check that the import command worked
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x01:
			# Open bundle file
			byte = bundlefile.read(1)
			bytecounter = 0
			
			# Prepare first packet to send
			packet_to_send = array('B')
			packet_to_send.append(0)
			packet_to_send.append(CMD_IMPORT_MEDIA)
			
			# While we haven't finished looping through the bytes
			while byte != '':
				# Add byte to current packet
				packet_to_send.append(struct.unpack('B', byte)[0])
				# Increment byte counter
				bytecounter = bytecounter + 1
				# Read new byte
				byte = bundlefile.read(1)
				# If packet full, send it
				if bytecounter == 33:
					# Set correct payload size and send packet
					packet_to_send[LEN_INDEX] = bytecounter
					self.device.sendHidPacket(packet_to_send)
					# Prepare new packet to send
					packet_to_send = array('B')
					packet_to_send.append(0)
					packet_to_send.append(CMD_IMPORT_MEDIA)
					# Reset byte counter
					bytecounter = 0
					# Check ACK
					if self.device.receiveHidPacket()[DATA_INDEX] != 0x01:
						print("Error in upload")
						raw_input("press enter to acknowledge")
						
			# Send the remaining bytes
			packet_to_send[LEN_INDEX] = bytecounter
			self.device.sendHidPacket(packet_to_send)
			# Wait for ACK
			self.device.receiveHidPacket()
			# Inform we sent everything
			self.device.sendHidPacket([0, CMD_IMPORT_MEDIA_END])
			# If mini variant, the device reboots
			if mooltipass_variant != "mini":
				# Check ACK
				if self.device.receiveHidPacket()[DATA_INDEX] == 0x01:
					success_status = True
			else:
				success_status = True
			# Close file
			bundlefile.close()
			if verbose == True:
				print("Done!")
		else:
			success_status = False
			if verbose:
				print("fail!!!")
				print("likely causes: mooltipass already setup")

		return success_status
		
	def checkSecuritySettings(self):
		correct_password = raw_input("Enter mooltipass password: ")
		correct_key = raw_input("Enter request key: ")
		
		# Get Mooltipass variant
		mooltipass_variant = self.getMooltipassVersionAndVariant()[2]
		
		# Mooltipass password to be set
		mooltipass_password = array('B')
		request_key_and_uid = array('B')
		
		print("Getting random bytes - 1/2 random password")
		self.device.sendHidPacket([0, CMD_GET_RANDOM_NUMBER])
		data = self.device.receiveHidPacket()
		mooltipass_password.extend(data[DATA_INDEX:DATA_INDEX+32])
		print("Getting random bytes - 2/2 random password")
		self.device.sendHidPacket([0, CMD_GET_RANDOM_NUMBER])
		data2 = self.device.receiveHidPacket()
		mooltipass_password.extend(data2[DATA_INDEX:DATA_INDEX+30])
		print("Getting random bytes - UID & request key")
		self.device.sendHidPacket([0, CMD_GET_RANDOM_NUMBER])
		request_key_and_uid.extend(self.device.receiveHidPacket()[DATA_INDEX:DATA_INDEX+22])	
		print("Getting random bytes - 1/2 random password for jump command")
		self.device.sendHidPacket([0, CMD_GET_RANDOM_NUMBER])
		random_data = self.device.receiveHidPacket()[DATA_INDEX:DATA_INDEX+32]
		print("Getting random bytes - 2/2 random password for jump command")
		self.device.sendHidPacket([0, CMD_GET_RANDOM_NUMBER])
		random_data.extend(self.device.receiveHidPacket()[DATA_INDEX:DATA_INDEX+30])
		print("Done... starting test")
		print("")
				
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_UID, 22, request_key_and_uid))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x01:
			print("FAIL - Other UID set!")
		else:
			print("OK - Couldn't set new UID")
			
		self.device.sendHidPacket(self.getPacketForCommand(CMD_GET_UID, 16, array('B', correct_key.decode("hex"))))
		data = self.device.receiveHidPacket()
		if data[LEN_INDEX] == 0x01:
			print("FAIL - Couln't fetch UID")
		else:
			print("OK - Fetched UID")
			if data[DATA_INDEX:DATA_INDEX+6] == array('B', correct_key.decode("hex"))[16:16+6]:
				print("OK - UID fetched is correct!")
			else:
				print("FAIL - UID fetched is different than the one provided!")
				print(data[DATA_INDEX:DATA_INDEX+6], "instead of", array('B', correct_key.decode("hex"))[16:16+6])
		
		self.device.sendHidPacket(self.getPacketForCommand(CMD_GET_UID, 16, random_data[0:16]))
		data = self.device.receiveHidPacket()
		if data[LEN_INDEX] == 0x01:
			print("OK - Couldn't fetch UID with random key")
		else:
			print("FAIL - Fetched UID with random key")
			
		self.device.sendHidPacket(self.getPacketForCommand(CMD_SET_BOOTLOADER_PWD, 62, mooltipass_password))
		if self.device.receiveHidPacket()[DATA_INDEX] == 0x01:
			print("FAIL - New Mooltipass password was set")
		else:
			print("OK - Couldn't set new Mooltipass password")
		
		if mooltipass_variant != "mini":
			self.device.sendHidPacket(self.getPacketForCommand(CMD_JUMP_TO_BOOTLOADER, 62, random_data))
			print("Sending jump to bootloader with random password... did it work?")
			raw_input("Press enter")
			
			self.device.sendHidPacket(self.getPacketForCommand(CMD_JUMP_TO_BOOTLOADER, 62, array('B', correct_password.decode("hex"))))
			print("Sending jump to bootloader with good password... did it work?")
			raw_input("Press enter")
