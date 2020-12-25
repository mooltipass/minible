import sys
sys.path.append('../../../blebundlegeneration')
from device_password_computations import *
from mooltipass_hid_device import *
device_key = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
device_iv = "00000000000000000000000000000000"
device_bundle_version = 0xFFFF
device_serial_number = 0
bundle_file = "bundle.img"


def bundleSecurityChecks():
	global device_bundle_version
	global device_serial_number
	global device_key
	
	# HID device constructor
	mooltipass_device = mooltipass_hid_device()	
	
	# Connect to device
	if mooltipass_device.connect(True) == False:
		sys.exit(0)
		
	# Check our assumptions
	plat_info_dict = mooltipass_device.getPlatformInfo()
	print("Connected to device with SN " + str(plat_info_dict["sn"]))
	if device_bundle_version != plat_info_dict["bundle"]:
		print("Incorrect bundle version to start tests")
		sys.exit(0)
	device_serial_number = plat_info_dict["sn"]
	
	# Start with incorrect password
	print("Starting with incorrect password...")
	wrong_password = compute_bundle_upload_key(bytearray.fromhex(device_key), bytearray.fromhex(device_iv), 0x12345678, device_bundle_version, False)
	wrong_password = ''.join('{:02x}'.format(x) for x in wrong_password)
	if mooltipass_device.uploadAndUpgradePlatform(bundle_file, wrong_password) == True:
		print("Somehow bundle upload worked!")
		sys.exit(0)
	

if __name__ == "__main__":
	bundleSecurityChecks()