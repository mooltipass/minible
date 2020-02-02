from __future__ import print_function
from command_defines import *
import threading
import struct
import serial
import Queue
import sys

enable_ble_seen = False
attach_usb_seen = False

# UARTs
uart_main_mcu = "COM16"
uart_aux_mcu = "COM15"
link_frame_bytes = 560

# Queue for sync
queue = Queue.Queue(1000)

# Open the UARTs
ser_aux = serial.Serial(uart_aux_mcu, 6000000)
ser_main = serial.Serial(uart_main_mcu, 6000000)
ser_aux.set_buffer_size(rx_size = 1000000, tx_size = 1000000)
ser_main.set_buffer_size(rx_size = 1000000, tx_size = 1000000)

	
def is_frame_valid(frame):
	# Extract message type
	[message_type] = struct.unpack("H", frame[0:2])
	
	# To check if frame is valid we only check that the message type is valid
	if message_type == AUX_MCU_MSG_TYPE_USB:
		if not attach_usb_seen:
			return False
		else:
			return True
	elif message_type == AUX_MCU_MSG_TYPE_BLE:
		if not enable_ble_seen:
			return False
		else:
			return True
	elif message_type == 0xFFFF:
		return True
	elif message_type in message_types:
		return True
	else:
		return False

def serial_read(s, mcu):
	global enable_ble_seen
	global attach_usb_seen
	resync_was_done = True
	nb_bytes = 0
	frame = []
	
	while True:	
		# Read the right number of bytes to get a full frame
		bytes = s.read(link_frame_bytes-nb_bytes)
		frame.extend(bytes)
		nb_bytes = len(frame)
		
		# Do we have a full frame?
		if link_frame_bytes == nb_bytes:		
			# Convert frame if needed
			if sys.version_info[0] < 3:
				frame_bis = bytearray(frame)
			else:
				frame_bis = frame
			
			# Check for valid frame
			if not is_frame_valid(frame_bis):
				print(mcu + ": discarding byte 0x" + str.format('{:02X}', frame_bis[0]) + " to try to get a SYNC")
				resync_was_done = True
				frame = frame[1:]
				nb_bytes -= 1
			else:				
				# Decode frame to look for attach usb/ble command
				[frame_message_type, frame_total_payload, frame_command] = struct.unpack("HHH", frame_bis[0:6])		
				if mcu == "MAIN" and frame_message_type == AUX_MCU_MSG_TYPE_MAIN_MCU_CMD and frame_command == MAIN_MCU_COMMAND_ATTACH_USB:
					attach_usb_seen = True
				if mcu == "MAIN" and frame_message_type == AUX_MCU_MSG_TYPE_BLE_CMD and frame_command == BLE_MESSAGE_CMD_ENABLE:
					enable_ble_seen = True
				
				# Add frame to the queue
				queue.put([frame_bis, mcu, resync_was_done])
				resync_was_done = False
				nb_bytes = 0
				frame = []		

# Reading threads
thread1 = threading.Thread(target=serial_read, args=(ser_main,"MAIN",),).start()
thread2 = threading.Thread(target=serial_read, args=(ser_aux,"AUX"),).start()

while True:
	try:
		[frame, mcu, resync_done] = queue.get(True, 1)
		#print(' '.join(str.format('{:02X}', x) for x in frame))
		
		# Resync done?
		if resync_done:
			if mcu == "AUX":
				print("<<<<<<< AUX RESYNC DONE >>>>>>>")
			else:
				print("<<<<<<< MAIN RESYNC DONE >>>>>>>")
		
		# Decode message type
		[message_type] = struct.unpack("H", frame[0:2])
		
		# Look for main mcu reset message
		if message_type == 0xFFFF:
			if mcu == "AUX":
				print("Aux->Main: incorrect reset message")
			else:
				print("Main->Aux:  comms reset message")
			continue
		
		# Apply decoding guidelines and build object
		decode_int_object = struct.unpack(decoding_guidelines[message_type][0], frame[0:decoding_guidelines[message_type][2]])
		decode_object = {}
		for i in range(0, len(decode_int_object)):
			decode_object[decoding_guidelines[message_type][1][i]] = decode_int_object[i]
				
		if mcu == "AUX":
			sys.stdout.write("Aux->Main: " + message_types_descriptions[message_type].rjust(20, ' '))
			
			if "command" in decode_object:
				# Check we have a description for the command
				command = decode_object["command"]
				if command >= len(aux_mcu_command_description[message_type]):
					sys.stdout.write(" - missing command description: " + str(command))
				else:
					sys.stdout.write(" - "+ aux_mcu_command_description[message_type][command])
			
			if message_type == AUX_MCU_MSG_TYPE_NIMH_CHARGE:
				charge_status_lut = ["idle", "current charge ramp start", "current charge ramp", "ramping start error", "current maintaining", "current ramp error", "current maintaining error", "charging done"]
				[type, payload, charge_status, battery_voltage, charge_current] = struct.unpack("HHHHH", frame[0:10])
				battery_voltage = battery_voltage*103/256
				
				print(" - details below:")
				print("                               Current status:", charge_status_lut[charge_status])
				print("                               Battery voltage:", str(battery_voltage)+"mV")
				print("                               Charging current:", str(charge_current*0.4)+"mA")
				
			if message_type == AUX_MCU_MSG_TYPE_PLAT_DETAILS:
				[type, payload, fw_major, fw_minor, did, uid1, uid2, uid3, uid4, blusdk_maj, blusdk_min, blusdk_fw_maj, blusdk_fw_min, blusdk_fw_bld, rf_ver, atbtlc_chip_id, addr1, addr2, addr3, addr4, addr5, addr6 ] = struct.unpack("HHHHIIIIIHHHHHIIBBBBBB", frame[0:54])
			
				print(" - details below:")
				print("                               Aux FW:", str(fw_major) + "." + str(fw_minor))
				print("                               DID:", "0x" + ''.join(str.format('{:08X}', did)))
				print("                               UID:", "0x" + ''.join(str.format('{:08X}', x) for x in [uid1, uid2, uid3, uid4]))
				print("                               BluSDK lib:", str(blusdk_maj) + "." + str(blusdk_min))
				print("                               BluSDK fw:", str(blusdk_fw_maj) + "." + str(blusdk_fw_min), "build", ''.join(str.format('{:04X}', blusdk_fw_bld)))
				print("                               RF version:", "0x" + ''.join(str.format('{:08X}', rf_ver)))
				print("                               ATBTLC1000 chip id:", "0x" + ''.join(str.format('{:08X}', atbtlc_chip_id)))
				print("                               BLE address:", ''.join(str.format('{:02X}', x) for x in [addr1, addr2, addr3, addr4, addr5, addr6]))
		else:
			sys.stdout.write("Main->Aux: " + message_types_descriptions[message_type].rjust(20, ' '))
			
			if "command" in decode_object:
				# Check we have a description for the command
				command = decode_object["command"]
				if command >= len(main_mcu_command_description[message_type]):
					sys.stdout.write(" - missing command description: " + str(command))
				else:
					sys.stdout.write(" - "+ main_mcu_command_description[message_type][command])
					
		print("")		
	except Queue.Empty:
		pass
	except KeyboardInterrupt:
		sys.exit(0)
