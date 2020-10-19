# Moolticute Export File Decoding Utility for Windows
# Required hardware
# - ACR38U-I1 (not any other!): https://www.ebay.com/itm/ACR38U-I1-Protable-Contact-Smart-IC-Chip-Card-Reader-Writer-Support-MAC-Linux-OS/291249029385
#
# Install Process (15/02/2020):
# - Install Python 3.6.x, at the time of writing version 3.6.9 and 3.6.10 couldn't be downloaded from the official website. You may take 3.6.8 from https://www.python.org/ftp/python/3.6.8/python-3.6.8-amd64.exe
# - Install pyscard from https://sourceforge.net/projects/pyscard/files/pyscard/ (see https://github.com/LudovicRousseau/pyscard/blob/master/INSTALL.md - https://sourceforge.net/projects/pyscard/files/pyscard/pyscard%201.9.9/pyscard-1.9.9.win-amd64-py3.6.msi/download used for this tutorial)
# - Install pycryptodome: python -m pip install pycryptodome
#
from smartcard.CardConnectionObserver import ConsoleCardConnectionObserver
from smartcard.Exceptions import CardRequestTimeoutException
from smartcard.util import toHexString, toBytes
from smartcard.CardRequest import CardRequest
from smartcard.CardType import AnyCardType
from tkinter import filedialog
from simplecrypt import *
import Crypto.Util.Counter
import Crypto.Cipher.AES
import tkinter as tk
import json
import sys
cardservice = None


def reverse_bit_order_in_byte_buffer(data):
	for i in range(0, len(data)):
		data[i] = int('{:08b}'.format(data[i])[::-1], 2)
	return data
	
def verify_security_code(pin_code):
	# Verify Security code
	print("")
	response, sw1, sw2 = cardservice.connection.transmit([0xFF, 0x20, 0x08, 0x0A, 0x02, int('{:08b}'.format((int(pin_code / 256)))[::-1], 2), int('{:08b}'.format((pin_code & 0x00FF))[::-1], 2)])
	print("")
	if sw1 == 0x90 and sw2 == 0x00:
		return False
	elif sw1 == 0x63 and sw2 == 0x00:
		return True
		
def read_memory_val(addr, size):
	# Read Memory Card
	print("")
	response, sw1, sw2 = cardservice.connection.transmit([0xFF, 0xB0, 0x00, addr, size])
	print("")
	response = reverse_bit_order_in_byte_buffer(response)
	#print ''.join('0x{:02x} '.format(x) for x in response[10:])			
	return response
		
def read_scac_value():
	# Read SCAC
	response = read_memory_val(12, 2)
	card_scac = response[0:2]
	number_of_tries_left = bin((card_scac[0] >> 4)&0x0F).count("1")			
	return number_of_tries_left

try:
	print("")
	print("Please insert your smartcard")
	cardrequest = CardRequest(timeout=10, cardType=AnyCardType())
	cardservice = cardrequest.waitforcard()

	# Attach the console tracer
	observer = ConsoleCardConnectionObserver()
	cardservice.connection.addObserver(observer)

	# Connect to the card and perform a few transmits
	cardservice.connection.connect()

	# Select card type
	print("")
	response, sw1, sw2 = cardservice.connection.transmit([0xFF, 0xA4, 0x00, 0x00, 0x01, 0x09])
	print("")
	if sw1 == 0x90 and sw2 == 00:
		print("Correctly changed card type to AT88SC102")
	else:
		print("Problem setting the card type to AT88SC102")
	
	# Read Memory Card
	print("")
	response, sw1, sw2 = cardservice.connection.transmit([0xFF, 0xB0, 0x00, 0x00, 22])
	print("")
	response = reverse_bit_order_in_byte_buffer(response)
	card_fz = response[0:2]
	card_iz = response[2:10]
	card_iz.append(0)
	card_iz_string = "".join(map(chr, card_iz))
	card_sc = response[10:12]
	card_scac = response[12:14]
	card_cpz = response[14:22]
	
	# Debug
	if False:
		print("FZ:", ''.join('0x{:02x} '.format(x) for x in response[0:2]))
		print("IZ:", ''.join('0x{:02x} '.format(x) for x in response[2:10]))
		print("IZ:", "".join(map(chr, response[2:10])))
		print("SC:", ''.join('0x{:02x} '.format(x) for x in response[10:12]))
		print("SCAC:", ''.join('0x{:02x} '.format(x) for x in response[12:14]))
		print("CPZ:", ''.join('0x{:02x} '.format(x) for x in response[14:22]))
		print(response)
		
	if card_fz[0] == 0x0F and card_fz[1] == 0x0F:
		print("Correct AT88SC102 card inserted")
	else:
		print("Error with AT88SC102 card")
		sys.exit(0)
		
	if str(card_iz_string == "hackaday") or str(card_iz_string) == "limpkin":
		print("Card Initialized by a Mooltipass Device")
	else:
		print("Card Not Initialized by a Mooltipass Device")
		sys.exit(0)
		
	number_of_tries_left = bin((card_scac[0] >> 4)&0x0F).count("1")
	cur_scac_val = number_of_tries_left
	if number_of_tries_left == 0:
		print("Card Blocked!!!")
		sys.exit(0)
	else:
		print("Number of PIN tries left:", number_of_tries_left	)
		
	if card_cpz[0] == 0xFF and card_cpz[1] == 0xFF and card_cpz[2] == 0xFF and card_cpz[3] == 0xFF and card_cpz[4] == 0xFF and card_cpz[5] == 0xFF and card_cpz[6] == 0xFF and card_cpz[7] == 0xFF:
		print("Empty Card Detected")
		sys.exit(0)
	else:
		print("User Card Detected")
	
	# Ask the user to enter the path to the JSON file
	root = tk.Tk()
	root.withdraw()
	file_path = filedialog.askopenfilename()
	with open(file_path) as json_file:
		json_data = json.load(json_file)
		if "encryption" not in json_data or (json_data["encryption"] != "SimpleCrypt" and json_data["encryption"] != "SimpleCryptV2"):
			print("Incorrect backup file")
		backup_file_payload = json_data["payload"]
		
	# Warning part
	print("")
	print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  WARNING  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	print("!                                                                                                       !")
	print("! Using this tool effectively renders your Mooltipass device useless.                                   !")
	print("! After accepting the following prompt, your AES key will be fetched from your card.                    !")
	print("! Both your credential database and its decryption key will therefore be in your computer memory.       !")
	print("! If your computer is infected, all your logins & passwords can be decrypted without your knowledge.    !")
	print("! Type \"I understand\" in the following prompt to proceed                                                !")
	print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
	text_input = input("Text input: ")
	if text_input != "I understand":
		sys.exit(0)
	
	# Ask pin code
	print("")
	pin_code = input("Please Enter Your PIN: ")
	pin_code = int(pin_code, 16)
	
	# Unlock card
	card_blocked = verify_security_code(pin_code)
	new_scac_val = read_scac_value()
	if card_blocked == True:
		print("Card Blocked")
		sys.exit(0)
	else:
		if new_scac_val == 4:
			print("Correct PIN")
		else:
			print("Wrong PIN,", new_scac_val, "tries left")
			sys.exit(0)
			
	# Unlocked card, read AES key
	aes_key = read_memory_val(24, 32)
	aes_key = aes_key[0:32]
	aes_key_string = ''.join('{:02x}'.format(x) for x in aes_key)
	print("")
	print("AES key extracted:", aes_key_string.upper())
	
	# Convert CPZ for file decryption
	cpz_value = 0
	if json_data["encryption"] == "SimpleCrypt":
		for i in range(0, 4):
			cpz_value += (card_cpz[i] << (i*8))
		for i in range(0, 4):
			cpz_value += (card_cpz[4+i] << (i*8))
	elif json_data["encryption"] == "SimpleCryptV2":
		for i in range(0, 8):
			cpz_value += (card_cpz[i] << (i*8))

	# Decrypt backup file payload
	print("Decrypting file with CPZ", ''.join('{:02x}'.format(x) for x in card_cpz))
	crypto = SimpleCrypt(cpz_value)
	decrypted_backup_file = json.loads(crypto.decrypt_to_string(backup_file_payload))
	
	# Fetch nonce
	nonce = []
	for i in range(8, 24):
		nonce.append(decrypted_backup_file[1][0][str(i)])
	nonce_string = ''.join('{:02x}'.format(x) for x in nonce)
	print("Nonce:", nonce_string.upper())
	
	# Loop through service nodes
	print("")
	print("Decrypting memory file contents...")
	for service_id in range(0, len(decrypted_backup_file[5])):
		# Reconstruct node data
		node_data = []
		for j in range(0, 132):
			node_data.append(decrypted_backup_file[5][service_id]["data"][str(j)])
			
		child_address = node_data[6:8]
		if child_address[0] == 0 and child_address[1] == 0:
			print("... doesn't have children")
		else:
			# Decrypt children
			while(child_address[0] != 0 or child_address[1] != 0):	
				# Loop through children to find addresses
				for j in range(0, len(decrypted_backup_file[6])):
					# Compare the wanted and actual address
					if decrypted_backup_file[6][j]["address"][0] == child_address[0] and decrypted_backup_file[6][j]["address"][1] == child_address[1]:
						# Rebuild node data
						node_data = []
						node_data_len = len(decrypted_backup_file[6][j]["data"])
						for k in range(0, node_data_len):
							node_data.append(decrypted_backup_file[6][j]["data"][str(k)])
						
						# decrypt data
						iv = nonce[:]
						if node_data_len == 132:
							iv[13] ^= node_data[34]
							iv[14] ^= node_data[35]
							iv[15] ^= node_data[36]
						else:
							remainder = 0
							for i in range(0, 3):
								tempsum = iv[15-i] + node_data[269-i] + remainder
								iv[15-i] = tempsum & 0x0FF
								remainder = int(tempsum/256) 
							iv[12] += remainder
						#print "CTR value:", ''.join('{:02x}'.format(x) for x in node_data[34:37])
						#print "IV value:", ''.join('{:02x}'.format(x) for x in iv)
						ctr = Crypto.Util.Counter.new(128, initial_value=int.from_bytes(iv, byteorder='big', signed=False))	
						cipher = Crypto.Cipher.AES.new(bytes(aes_key), Crypto.Cipher.AES.MODE_CTR, counter=ctr)
						if node_data_len == 132:
							password = cipher.decrypt(bytes(node_data[100:132]))
							decoded_password = password.decode('raw_unicode_escape').split("\x00")[0]
						else:
							password = cipher.decrypt(bytes(node_data[270:398]))
							decoded_password = ""
							for i in range(0, int(len(password)/2)):
								code_point = password[i*2] + password[i*2+1]*256
								if code_point == 0:
									break
								else:
									decoded_password += chr(code_point)
						
						# print data
						print(decrypted_backup_file[5][service_id]["name"] + ":", decrypted_backup_file[6][j]["name"], "/", decoded_password)
						child_address = node_data[4:6]
						break

	print("")

except CardRequestTimeoutException:
	print("No card was inserted")
