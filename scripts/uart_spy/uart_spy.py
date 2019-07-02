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

AUX_MCU_EVENT_BLE_ENABLED		= 0x0001
AUX_MCU_EVENT_BLE_DISABLED		= 0x0002
AUX_MCU_EVENT_TX_SWEEP_DONE		= 0x0003
AUX_MCU_EVENT_FUNC_TEST_DONE	= 0x0004
AUX_MCU_EVENT_USB_ENUMERATED	= 0x0005
AUX_MCU_EVENT_CHARGE_DONE		= 0x0006
AUX_MCU_EVENT_CHARGE_FAIL		= 0x0007

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
	
def is_frame_valid(frame):
	# Extract values
	[message_type, total_payload, command, command_payload_length] = struct.unpack("HHHH", frame[0:8])
	
	if message_type == AUX_MCU_MSG_TYPE_USB:
		if command >= 1 and command <= 0x0015:
			return True
		elif command >= 0x0100 and command <= 0x010F:
			return True
		elif command >= 0x8000 and command <= 0x8010:
			return True
		else:
			return False
	elif message_type == AUX_MCU_MSG_TYPE_BLE:
		if command >= 1 and command <= 0x0015:
			return True
		elif command >= 0x0100 and command <= 0x010F:
			return True
		elif command >= 0x8000 and command <= 0x8010:
			return True
		else:
			return False
	elif message_type == AUX_MCU_MSG_TYPE_BOOTLOADER:
		if command >= 0x0000 and command <= 0x0002:
			return True
		else:
			return False
	elif message_type == AUX_MCU_MSG_TYPE_PLAT_DETAILS:
		return True
	elif message_type == AUX_MCU_MSG_TYPE_MAIN_MCU_CMD:
		if command >= 0x0001 and command <= 0x0009:
			return True
		else:
			return False
	elif message_type == AUX_MCU_MSG_TYPE_AUX_MCU_EVENT:
		if command >= 0x0001 and command <= 0x0007:
			return True
		else:
			return False
	elif message_type == AUX_MCU_MSG_TYPE_NIMH_CHARGE:
		return True
	elif message_type == 0xFFFF:
		return True
	else:
		return False

def serial_read(s, mcu):
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
				
			# To possibly be changed later: change bit order
			for i in range(0, link_frame_bytes):
				frame_bis[i] = reverse_mask(frame_bis[i])
			
			# Check for valid frame
			if not is_frame_valid(frame_bis):
				resync_was_done = True
				# remove one byte
				frame = frame[1:]
				nb_bytes -= 1
			else:							   
				# Add to the queue
				queue.put([frame_bis, mcu, resync_was_done])
				nb_bytes = 0
				frame = []		
				resync_was_done = False

# Reading threads
thread1 = threading.Thread(target=serial_read, args=(ser_main,"MAIN",),).start()
thread2 = threading.Thread(target=serial_read, args=(ser_aux,"AUX"),).start()

while True:
	try:
		[frame, mcu, resync_done] = queue.get(True, 1)
		
		# Decode frame
		[message_type, total_payload, command, command_payload_length] = struct.unpack("HHHH", frame[0:8])
		
		# Resync done?
		if resync_done:
			if mcu == "AUX":
				print("<<<<<<< AUX RESYNC DONE >>>>>>>")
			else:
				print("<<<<<<< MAIN RESYNC DONE >>>>>>>")
		
		# Debug depending on message
		if message_type == AUX_MCU_MSG_TYPE_USB or message_type == AUX_MCU_MSG_TYPE_BLE:
			if message_type == AUX_MCU_MSG_TYPE_USB:
				interface_name = "USB"
			else:
				interface_name = "BLE"
				
			if mcu == "AUX":				
				usb_lut = [	"0x0000: Not valid",
							"0x0001: ping",
							"0x0002: invalid - please retry",
							"0x0003: platform info request",
							"0x0004: set date",
							"0x0005: cancel request",
							"0x0006: store credential",
							"0x0007: get credential",
							"0x0008: get 32b rng",
							"0x0009: start mmm",
							"0x000A: get user change nb",
							"0x000B: get card cpz",
							"0x000C: get device settings",
							"0x000D: set device settings",
							"0x000E: reset unknown card",
							"0x000F: get nb free users",
							"0x0010: lock device",
							"0x0011: get device status",
							"0x0012: check password",
							"0x0013: get user settings",
							"0x0014: get category strings",
							"0x0015: set category strings"]
				usb_lut_mmm = [
							"0x0100: get start parents",
							"0x0101: end mmm",
							"0x0102: read node",
							"0x0103: set cred change nb",
							"0x0104: set data change nb",
							"0x0105: set cred start parent",
							"0x0106: set data start parent",
							"0x0107: set start parents",
							"0x0108: get free nodes",
							"0x0109: get ctr value",
							"0x010A: set ctr value",
							"0x010B: set favorite",
							"0x010C: get favorite",
							"0x010D: write node",
							"0x010E: get cpz ctr",
							"0x010F: get favorites"]
				usb_lut_debug = [
							"0x8000: debug message",
							"0x8001: open display buffer",
							"0x8002: send to display buffer",
							"0x8003: close display buffer",
							"0x8004: erase dataflash",
							"0x8005: is dataflash ready",
							"0x8006: dataflash 256B write",
							"0x8007: start bootloader",
							"0x8008: get acc 32 samples",
							"0x8009: flash aux mcu",
							"0x800A: get debug platform info",
							"0x800B: reindex bundle"]
				
				if command >= 0x8000:
					command_msg = usb_lut_debug[command-0x8000]
				elif command >= 0x0100:
					command_msg = usb_lut_mmm[command-0x100]
				else:
					command_msg = usb_lut[command]
					
				print("Aux->Main: " + interface_name + " Message - " + command_msg)
			else:
				usb_lut = [	"0x0000: Not valid",
							"0x0001: pong",
							"0x0002: please retry",
							"0x0003: platform info",
							"0x0004: set date answer",
							"0x0005: invalid - cancel request",
							"0x0006: store credential answer",
							"0x0007: get credential answer",
							"0x0008: 32b rng",
							"0x0009: start mmm answer",
							"0x000A: get user change nb answer",
							"0x000B: get card cpz answer",
							"0x000C: get device settings answer",
							"0x000D: set device settings",
							"0x000E: reset unknown card answer",
							"0x000F: get nb free users answer",
							"0x0010: lock device answer",
							"0x0011: get device status answer",
							"0x0012: check password answer",
							"0x0013: get user settings answer",
							"0x0014: get category strings answer",
							"0x0015: set category strings"]
				usb_lut_mmm = [
							"0x0100: get start parents answer",
							"0x0101: end mmm answer",
							"0x0102: read node answer",
							"0x0103: set cred change nb answer",
							"0x0104: set data change nb answer",
							"0x0105: set cred start parent answer",
							"0x0106: set data start parent answer",
							"0x0107: set start parents answer",
							"0x0108: get free nodes answer",
							"0x0109: get ctr value answer",
							"0x010A: set ctr value answer",
							"0x010B: set favorite answer",
							"0x010C: get favorite answer",
							"0x010D: write node answer",
							"0x010E: get cpz ctr answer",
							"0x010F: get favorites answer"]			
				usb_lut_debug = [
							"0x8000: debug message",
							"0x8001: open display buffer answer",
							"0x8002: send to display buffer answer",
							"0x8003: close display buffer answer",
							"0x8004: erase dataflash answer",
							"0x8005: is dataflash ready answer",
							"0x8006: dataflash 256B write answer",
							"0x8007: start bootloader answer",
							"0x8008: get acc 32 samples answer",
							"0x8009: flash aux mcu answer",
							"0x800A: get debug platform info answer",
							"0x800B: reindex bundle answer"]
				
				if command >= 0x8000:
					command_msg = usb_lut_debug[command-0x8000]
				elif command >= 0x0100:
					command_msg = usb_lut_mmm[command-0x100]
				else:
					command_msg = usb_lut[command]
					
				print("Main->Aux: " + interface_name + " Message - " + command_msg)
		elif message_type == AUX_MCU_MSG_TYPE_NIMH_CHARGE:
			if mcu == "AUX":
				charge_status_lut = ["idle", "current charge ramp start", "current charge ramp", "ramping start error", "current maintaining", "current ramp error", "current maintaining error", "charging done"]
				[type, payload, charge_status, battery_voltage, charge_current] = struct.unpack("HHHHH", frame[0:10])
				battery_voltage = battery_voltage*103/256
				
				print("Aux->Main: charging report")
				print("Current status:", charge_status_lut[charge_status])
				print("Battery voltage:", str(battery_voltage)+"mV")
				print("Charging current:", str(charge_current*0.4)+"mA")
			else:
				print("Main->Aux: charging report request")
		elif message_type == AUX_MCU_MSG_TYPE_PLAT_DETAILS:
			if mcu == "AUX":
				[type, payload, fw_major, fw_minor, did, uid1, uid2, uid3, uid4, blusdk_maj, blusdk_min, blusdk_fw_maj, blusdk_fw_min, blusdk_fw_bld, rf_ver, atbtlc_chip_id, addr1, addr2, addr3, addr4, addr5, addr6 ] = struct.unpack("HHHHIIIIIHHHHHIIBBBBBB", frame[0:54])
			
				print("Aux->Main: platform details")
				print("			  Aux FW:", str(fw_major) + "." + str(fw_minor))
				print("			  DID:", "0x" + ''.join(str.format('{:08X}', did)))
				print("			  UID:", "0x" + ''.join(str.format('{:08X}', x) for x in [uid1, uid2, uid3, uid4]))
				print("			  BluSDK lib:", str(blusdk_maj) + "." + str(blusdk_min))
				print("			  BluSDK fw:", str(blusdk_fw_maj) + "." + str(blusdk_fw_min), "build", ''.join(str.format('{:04X}', blusdk_fw_bld)))
				print("			  RF version:", "0x" + ''.join(str.format('{:08X}', rf_ver)))
				print("			  ATBTLC1000 chip id:", "0x" + ''.join(str.format('{:08X}', atbtlc_chip_id)))
				print("			  BLE address:", ''.join(str.format('{:02X}', x) for x in [addr1, addr2, addr3, addr4, addr5, addr6]))
			else:
				print("Main->Aux: aux MCU details request")
		elif message_type == AUX_MCU_MSG_TYPE_MAIN_MCU_CMD:
			if mcu == "AUX":
				if command == MAIN_MCU_COMMAND_PING:
					print("Aux->Main: pong to Main")
				else:
					print("Aux->Main: invalid command!")
			else:
				command_lut = [	"0x0000: invalid",
								"0x0001: sleep",
								"0x0002: attach usb",
								"0x0003: ping to Aux",
								"0x0004: enable BLE",
								"0x0005: charge battery",
								"0x0006: no comms signal unavailable",
								"0x0007: disable BLE",
								"0x0008: detach USB",
								"0x0009: functional test"]
				print("Main->Aux: command " + command_lut[command])
		elif message_type == AUX_MCU_MSG_TYPE_AUX_MCU_EVENT:
			if mcu == "AUX":
				event_lut = [	"0x0000: invalid",
								"0x0001: ble enabled",
								"0x0002: ble disabled",
								"0x0003: tx sweep done",
								"0x0004: func test done",
								"0x0005: usb enumerated",
								"0x0006: charge done",
								"0x0007: charge failed"]
				print("Aux->Main: event " + event_lut[command])
			else:
				print("Main->Aux: invalid packet (event)")
		else:
			print(mcu, "message type", hex(message_type))
			print(' '.join(str.format('{:02X}', x) for x in frame))
		
	except Queue.Empty:
		pass
	except KeyboardInterrupt:
		sys.exit(0)
