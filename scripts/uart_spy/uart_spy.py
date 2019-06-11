from __future__ import print_function
import threading
import struct
import serial
import Queue
import sys

AUX_MCU_MSG_TYPE_USB			= 0x0000
AUX_MCU_MSG_TYPE_BLE			= 0x0001
AUX_MCU_MSG_TYPE_BOOTLOADER		= 0x0002
AUX_MCU_MSG_TYPE_PLAT_DETAILS	= 0x0003
AUX_MCU_MSG_TYPE_MAIN_MCU_CMD	= 0x0004
AUX_MCU_MSG_TYPE_AUX_MCU_EVENT	= 0x0005
AUX_MCU_MSG_TYPE_NIMH_CHARGE	= 0x0006

MAIN_MCU_COMMAND_SLEEP			= 0x0001
MAIN_MCU_COMMAND_ATTACH_USB		= 0x0002
MAIN_MCU_COMMAND_PING			= 0x0003
MAIN_MCU_COMMAND_ENABLE_BLE		= 0x0004
MAIN_MCU_COMMAND_NIMH_CHARGE	= 0x0005
MAIN_MCU_COMMAND_NO_COMMS_UNAV	= 0x0006
MAIN_MCU_COMMAND_DISABLE_BLE	= 0x0007
MAIN_MCU_COMMAND_DETACH_USB		= 0x0008
MAIN_MCU_COMMAND_FUNC_TEST		= 0x0009

# UARTs
uart_main_mcu = "COM7"
uart_aux_mcu = "COM8"
link_frame_bytes = 544

# Queue for sync
queue = Queue.Queue(1000)

# Open the UARTs
ser_main = serial.Serial(uart_main_mcu, 6000000)
ser_main.set_buffer_size(rx_size = 1000000, tx_size = 1000000)
ser_aux = serial.Serial(uart_aux_mcu, 6000000)
ser_aux.set_buffer_size(rx_size = 1000000, tx_size = 1000000)


def reverse_mask(x):
	x = ((x & 0x55) << 1) | ((x & 0xAA) >> 1)
	x = ((x & 0x33) << 2) | ((x & 0xCC) >> 2)
	x = ((x & 0x0F) << 4) | ((x & 0xF0) >> 4)
	return x

def serial_read(s, mcu):
	in_sync = False
	nb_bytes = 0
	frame = []
	while True:
		bytes = s.read(link_frame_bytes-nb_bytes)
		frame.extend(bytes)
		nb_bytes = len(frame)
		
		if link_frame_bytes == nb_bytes:
			# Convert frame if needed
			if sys.version_info[0] < 3:
				frame_bis = bytearray(frame)
			else:
				frame_bis = frame
				
			# To be changed later: change bit order
			for i in range(0, link_frame_bytes):
				frame_bis[i] = reverse_mask(frame_bis[i])
				
			if in_sync == False:
				message_type = frame_bis[0] + frame_bis[1]*256	
				# First message to be read
				if message_type < 100:
					in_sync = True
				else:
					frame = frame[1:]
					nb_bytes -= 1
			
			if in_sync == True:									   
				# Add to the queue
				queue.put([frame_bis, mcu])
				nb_bytes = 0
				frame = []

# Reading threads
thread1 = threading.Thread(target=serial_read, args=(ser_main,"MAIN",),).start()
thread2 = threading.Thread(target=serial_read, args=(ser_aux,"AUX"),).start()

while True:
	try:
		[frame, mcu] = queue.get(True, 1)
		
		# Decode frame
		message_type = struct.unpack("H", frame[0:2])[0]
		
		# Debug depending on message
		if message_type == AUX_MCU_MSG_TYPE_NIMH_CHARGE:
			if mcu == "AUX":
				charge_status_lut = ["idle", "current charge ramp start", "current charge ramp", "ramping start error", "current maintaining", "current ramp error", "current maintaining error", "charging done"]
				[type, payload, charge_status, battery_voltage, charge_current] = struct.unpack("HHHHH", frame[0:10])
				battery_voltage = battery_voltage*103/256
				
				print("Aux MCU: charging report")
				print("Current status:", charge_status_lut[charge_status])
				print("Battery voltage:", str(battery_voltage)+"mV")
				print("Charging current:", str(charge_current*0.4)+"mA")
			else:
				print("Main MCU: charging report request")
		elif message_type == AUX_MCU_MSG_TYPE_MAIN_MCU_CMD:
			[type, payload, command] = struct.unpack("HHH", frame[0:6])
			
			# Switch on command
			if command == MAIN_MCU_COMMAND_PING:
				if mcu == "AUX":
					print("Aux MCU: PONG")
				else:
					print("Main MCU: PING")
			else:
				print(mcu, "message type", hex(message_type))
				print(' '.join(str.format('{:02X}', x) for x in frame))
		elif message_type == AUX_MCU_MSG_TYPE_PLAT_DETAILS:
			if mcu == "AUX":
				[type, payload, fw_major, fw_minor, did, uid1, uid2, uid3, uid4, blusdk_maj, blusdk_min, blusdk_fw_maj, blusdk_fw_min, blusdk_fw_bld, rf_ver, atbtlc_chip_id, addr1, addr2, addr3, addr4, addr5, addr6 ] = struct.unpack("HHHHIIIIIHHHHHIIBBBBBB", frame[0:54])
			
				print("Aux MCU: platform details")
				print("Aux FW:", str(fw_major) + "." + str(fw_minor))
				print("DID:", "0x" + ''.join(str.format('{:08X}', did)))
				print("UID:", "0x" + ''.join(str.format('{:08X}', x) for x in [uid1, uid2, uid3, uid4]))
				print("BluSDK lib:", str(blusdk_maj) + "." + str(blusdk_min))
				print("BluSDK fw:", str(blusdk_fw_maj) + "." + str(blusdk_fw_min), "build", ''.join(str.format('{:04X}', blusdk_fw_bld)))
				print("RF version:", "0x" + ''.join(str.format('{:08X}', rf_ver)))
				print("ATBTLC1000 chip id:", "0x" + ''.join(str.format('{:08X}', atbtlc_chip_id)))
				print("BLE address:", ''.join(str.format('{:02X}', x) for x in [addr1, addr2, addr3, addr4, addr5, addr6]))
			else:
				print("Main MCU: aux MCU details request")
		else:
			print(mcu, "message type", hex(message_type))
			print(' '.join(str.format('{:02X}', x) for x in frame))
		
		print("")
	except Queue.Empty:
		pass
	except KeyboardInterrupt:
		sys.exit(0)
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		