import os
import sys
import time
import struct
from ctypes import (CDLL, get_errno)
from ctypes.util import find_library
from socket import (
    socket,
    AF_BLUETOOTH,
    SOCK_RAW,
    BTPROTO_HCI,
    SOL_HCI,
    HCI_FILTER,
)

# Find library and setup socket
btlib = find_library("bluetooth")
if not btlib:
    raise Exception(
        "Can't find required bluetooth libraries"
        " (need to install bluez)"
    )
bluez = CDLL(btlib, use_errno=True)
dev_id = bluez.hci_get_route(None)
ble_socket = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)
ble_socket.bind((dev_id,))

def setup_bluetooth_scanning():
	# Root?
	if not os.geteuid() == 0:
		sys.exit("script only works as root")
		
	# set scan parameters
	err = bluez.hci_le_set_scan_parameters(ble_socket.fileno(), 0, 0x10, 0x10, 0, 0, 1000);
	if err < 0:
		# occurs when scanning is still enabled from previous call
		print("Couldn't set scan parameters, restarting hci")
		os.system("sudo hciconfig hci0 down")
		os.system("sudo hciconfig hci0 up")
		
		# set scan parameters, again
		err = bluez.hci_le_set_scan_parameters(ble_socket.fileno(), 0, 0x10, 0x10, 0, 0, 1000);
		
		# Check error again
		if err < 0:
			raise Exception("Set scan parameters failed")

	# allows LE advertising events
	hci_filter = struct.pack(
		"<IQH", 
		0x00000010, 
		0x4000000000000000, 
		0
	)
	ble_socket.setsockopt(SOL_HCI, HCI_FILTER, hci_filter)

	# enable scan
	err = bluez.hci_le_set_scan_enable(
		ble_socket.fileno(),
		1,  # 1 - turn on;  0 - turn off
		0, # 0-filtering disabled, 1-filter out duplicates
		1000  # timeout
	)
	
	# errors?
	if err < 0:
		errnum = get_errno()
		raise Exception("{} {}".format(
			errno.errorcode[errnum],
			os.strerror(errnum)
		))

def find_bluetooth_address(address, nb_times_find, nb_secs_timeout):
	# finding counter
	find_counter = 0
	
	# get current time
	current_second = int(time.time())
	
	while int(time.time()) < current_second + nb_secs_timeout:
		# Get a MAC
		data = ble_socket.recv(1024)
		found_mac = ':'.join("{0:02x}".format(x) for x in data[12:6:-1])
		#print(found_mac)
		
		# Check for match
		if found_mac == address:
			find_counter += 1
			if find_counter >= nb_times_find:
				return True
		
	# timed out
	return False
