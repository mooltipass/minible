from __future__ import print_function
from time import gmtime, strftime, localtime
from command_defines import *
from datetime import datetime
import threading
import struct
import serial
import time
import glob
import sys
if sys.version_info[0] < 3:
	import Queue as queue
else:
	import asyncio as queue
	import asyncio

# Python main loop
if sys.version_info[0] >= 3:
	loop = asyncio.get_event_loop()

# If we've seen a message to enable USB or BLE
enable_ble_seen = False
attach_usb_seen = False

# Create a list of possible com ports
if sys.platform.startswith('win'):
	ports = ['COM%s' % (i + 1) for i in range(256)]
elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
	# this excludes your current terminal "/dev/tty"
	ports = glob.glob('/dev/tty[A-Za-z]*')
elif sys.platform.startswith('darwin'):
	ports = glob.glob('/dev/tty.*')
else:
	raise EnvironmentError('Unsupported platform')

# Try to open them to see which ones are actually available
available_ports = []
for port in ports:
	try:
		s = serial.Serial(port)
		s.close()
		available_ports.append(port)
	except (OSError, serial.SerialException):
		pass

# Check for ports
if len(available_ports) < 2:
	print("Couldn't find COM ports!")
	sys.exit(0)

# Try to guess which one is ours
if sys.platform.startswith('win'):
	for i in range(0, 256):
		if "COM" + str(i) in available_ports and "COM" + str(i+1) in available_ports and "COM" + str(i+3) in available_ports:
			uart_main_mcu = "COM" + str(i+1)
			uart_aux_mcu = "COM" + str(i)
			uart_debug = "COM" + str(i+2)
			break
elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
	# TODO
        uart_main_mcu = available_ports[3]
        uart_aux_mcu = available_ports[2]
        uart_debug = available_ports[1]
elif sys.platform.startswith('darwin'):
	# TODO
	uart_main_mcu = available_ports[1]
	uart_aux_mcu = available_ports[0]
	uart_debug = available_ports[2]

# Queue for sync
msgqueue = queue.Queue(1000)

# Open the UARTs
link_frame_bytes = 560
ser_dbg = serial.Serial(uart_debug, 3000000)
ser_aux = serial.Serial(uart_aux_mcu, 6000000)
ser_main = serial.Serial(uart_main_mcu, 6000000)
if not (sys.platform.startswith('linux') or sys.platform.startswith('cygwin')):
    ser_dbg.set_buffer_size(rx_size = 1000000, tx_size = 1000000)
    ser_aux.set_buffer_size(rx_size = 1000000, tx_size = 1000000)
    ser_main.set_buffer_size(rx_size = 1000000, tx_size = 1000000)


def is_frame_valid(frame):
	# Extract message type
	[message_type] = struct.unpack("H", frame[0:2])

	# To check if frame is valid we only check that the message type is valid
	#if message_type == AUX_MCU_MSG_TYPE_USB:
	#	if not attach_usb_seen:
	#		return False
	#	else:
	#		return True
	#elif message_type == AUX_MCU_MSG_TYPE_BLE:
	#	if not enable_ble_seen:
	#		return False
	#	else:
	#		return True
	if message_type == 0xFFFF:
		return True
	elif message_type in message_types:
		return True
	else:
		return False

def debug_serial_read(s, str):
	global loop
	while True:
		line = s.readline()
		if sys.version_info[0] >= 3:
			loop.call_soon_threadsafe(msgqueue.put_nowait([line.decode("utf-8"), "DBG", False]))
		else:
			msgqueue.put([line, "DBG", False])

def serial_read(s, mcu):
	global enable_ble_seen
	global attach_usb_seen
	global loop
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
			frame_bis = bytearray(frame)

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
				if sys.version_info[0] >= 3:
					loop.call_soon_threadsafe(msgqueue.put_nowait([frame_bis, mcu, resync_was_done]))
				else:
					msgqueue.put([frame_bis, mcu, resync_was_done])
				resync_was_done = False
				nb_bytes = 0
				frame = []

# Reading threads
thread1 = threading.Thread(target=serial_read, args=(ser_main,"MAIN"),).start()
thread2 = threading.Thread(target=serial_read, args=(ser_aux,"AUX"),).start()
thread3 = threading.Thread(target=debug_serial_read, args=(ser_dbg, "DBG"),).start()

while True:
	try:
		if sys.version_info[0] >= 3:
			[frame, mcu, resync_done] = msgqueue.get_nowait()
		else:
			[frame, mcu, resync_done] = msgqueue.get(True, 1)

		if mcu != "DBG":
			print(' '.join(str.format('{:02X}', x) for x in frame[0:80]))

		if mcu == "DBG":
			print(datetime.now().strftime("%H:%M:%S.%f")[:-3] + ": debug MSG:    " + frame.replace("\r","").replace("\n",""))
		else:
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
				sys.stdout.write(datetime.now().strftime("%H:%M:%S.%f")[:-3] + ": Aux->Main: " + message_types_descriptions[message_type].rjust(20, ' '))

				if "command" in decode_object:
					# Check we have a description for the command
					command = decode_object["command"]
					if command >= len(aux_mcu_command_description[message_type]):
						sys.stdout.write(" - missing command description: " + str(command))
					else:
						sys.stdout.write(" - "+ aux_mcu_command_description[message_type][command])

				if message_type == AUX_MCU_MSG_TYPE_NIMH_CHARGE:
					charge_status_lut = ["idle", "current charge ramp start", "current charge ramp", "ramping start error", "current maintaining", "current ramp error", "current maintaining error", "charging done", "peak timer triggered", "temp rest"]
					[type, payload, charge_status, battery_voltage, charge_current] = struct.unpack("HHHHH", frame[0:10])
					battery_voltage = battery_voltage*3300/(1.48*4095)

					print(" - details below:")
					print("	                                  Current status:", charge_status_lut[charge_status])
					print("	                                  Battery voltage:", str(battery_voltage)+"mV")
					print("	                                  Charging current:", str(charge_current*0.5445)+"mA")

				if message_type == AUX_MCU_MSG_TYPE_PLAT_DETAILS:
					[type, payload, fw_major, fw_minor, did, uid1, uid2, uid3, uid4, blusdk_maj, blusdk_min, blusdk_fw_maj, blusdk_fw_min, blusdk_fw_bld, rf_ver, atbtlc_chip_id, addr1, addr2, addr3, addr4, addr5, addr6 ] = struct.unpack("HHHHIIIIIHHHHHIIBBBBBB", frame[0:54])

					print(" - details below:")
					print("	                                  Aux FW:", str(fw_major) + "." + str(fw_minor))
					print("	                                  DID:", "0x" + ''.join(str.format('{:08X}', did)))
					print("	                                  UID:", "0x" + ''.join(str.format('{:08X}', x) for x in [uid1, uid2, uid3, uid4]))
					print("	                                  BluSDK lib:", str(blusdk_maj) + "." + str(blusdk_min))
					print("	                                  BluSDK fw:", str(blusdk_fw_maj) + "." + str(blusdk_fw_min), "build", ''.join(str.format('{:04X}', blusdk_fw_bld)))
					print("	                                  RF version:", "0x" + ''.join(str.format('{:08X}', rf_ver)))
					print("	                                  ATBTLC1000 chip id:", "0x" + ''.join(str.format('{:08X}', atbtlc_chip_id)))
					print("	                                  BLE address:", ''.join(str.format('{:02X}', x) for x in [addr1, addr2, addr3, addr4, addr5, addr6]))

				if message_type == AUX_MCU_MSG_TYPE_AUX_MCU_EVENT and command == 0x000D:
					[type, payload, command, charge_status] = struct.unpack("HHHB", frame[0:7])
					sys.stdout.write(" " + str(charge_status*10) + "pct")
				elif message_type == AUX_MCU_MSG_TYPE_USB and command == 0x8000:
					sys.stdout.write(": " + frame[7:])
			else:
				sys.stdout.write(datetime.now().strftime("%H:%M:%S.%f")[:-3] + ": Main->Aux: " + message_types_descriptions[message_type].rjust(20, ' '))

				if "command" in decode_object:
					# Check we have a description for the command
					command = decode_object["command"]
					if command >= len(main_mcu_command_description[message_type]):
						sys.stdout.write(" - missing command description: " + str(command))
					else:
						sys.stdout.write(" - "+ main_mcu_command_description[message_type][command])

						if command == 0x000C:
							[type, payload, command, charge_status] = struct.unpack("HHHB", frame[0:7])
							sys.stdout.write(" " + str(charge_status) + "pct")

			print("")
	except Exception as e:
		if e == KeyboardInterrupt:
			sys.exit(0)
		else:
			# It's cheating I know....
			time.sleep(.000001)
