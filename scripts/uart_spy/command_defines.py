AUX_MCU_MSG_TYPE_USB            = 0x0000
AUX_MCU_MSG_TYPE_BLE            = 0x0001
AUX_MCU_MSG_TYPE_BOOTLOADER     = 0x0002
AUX_MCU_MSG_TYPE_PLAT_DETAILS   = 0x0003
AUX_MCU_MSG_TYPE_MAIN_MCU_CMD   = 0x0004
AUX_MCU_MSG_TYPE_AUX_MCU_EVENT  = 0x0005
AUX_MCU_MSG_TYPE_NIMH_CHARGE    = 0x0006
AUX_MCU_MSG_TYPE_PING_WITH_INFO = 0x0007
AUX_MCU_MSG_TYPE_KEYBOARD_TYPE  = 0x0008
AUX_MCU_MSG_TYPE_FIDO2          = 0x0009
AUX_MCU_MSG_TYPE_RNG_TRANSFER   = 0x000A
AUX_MCU_MSG_TYPE_BLE_CMD        = 0x000B
message_types = [AUX_MCU_MSG_TYPE_USB, AUX_MCU_MSG_TYPE_BLE, AUX_MCU_MSG_TYPE_BOOTLOADER, AUX_MCU_MSG_TYPE_PLAT_DETAILS, AUX_MCU_MSG_TYPE_MAIN_MCU_CMD, AUX_MCU_MSG_TYPE_AUX_MCU_EVENT, AUX_MCU_MSG_TYPE_NIMH_CHARGE, AUX_MCU_MSG_TYPE_PING_WITH_INFO, AUX_MCU_MSG_TYPE_KEYBOARD_TYPE, AUX_MCU_MSG_TYPE_FIDO2, AUX_MCU_MSG_TYPE_RNG_TRANSFER, AUX_MCU_MSG_TYPE_BLE_CMD]
message_types_descriptions = [	"USB Message",
								"BLE Message",
								"Bootloader Message",
								"Platform Details",
								"Main MCU Command",
								"Aux MCU Event",
								"NiMH Charge Message",
								"Main Ping with Info",
								"Keyboard Typing",
								"FIDO2 Message",
								"RNG Message",
								"BLE Command"]

decoding_guidelines = []
decoding_guidelines.append(["HHHH", ["message_type", "total_payload", "command", "command_payload_length"], 8])	# USB message
decoding_guidelines.append(["HHHH", ["message_type", "total_payload", "command", "command_payload_length"], 8])	# BLE message
decoding_guidelines.append(["HHH",  ["message_type", "total_payload", "command"], 6])							# Bootloader message
decoding_guidelines.append(["HH",   ["message_type", "total_payload"], 4])										# Platform details
decoding_guidelines.append(["HHH",  ["message_type", "total_payload", "command"], 6])							# Command message
decoding_guidelines.append(["HHH",  ["message_type", "total_payload", "command"], 6])							# Aux MCU event
decoding_guidelines.append(["HH",   ["message_type", "total_payload"], 4])										# NiMh charge
decoding_guidelines.append(["HH",   ["message_type", "total_payload"], 4])										# Ping with info
decoding_guidelines.append(["HH",   ["message_type", "total_payload"], 4])										# Keyboard type
decoding_guidelines.append(["HHH",  ["message_type", "total_payload", "command"], 6])							# FIDO2
decoding_guidelines.append(["HH",   ["message_type", "total_payload"], 4])										# RNG transfer
decoding_guidelines.append(["HHH",  ["message_type", "total_payload", "command"], 6])							# BLE command

aux_mcu_command_description = []
# USB message
aux_mcu_command_description.append(	[	"0x0000: Not valid",
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
										"0x0015: set category strings",
										"0x0016: set user language",
										"0x0017: get device language",
										"0x0018: get user keyboard",
										"0x0019: get number of languages",
										"0x001A: get number of layouts",
										"0x001B: get language description",
										"0x001C: get layout description",
										"0x001D: get user keyboard",
										"0x001E: get user language",
										"0x001F: set device language"])
aux_mcu_command_description[0].extend(["invalid"]*(256-len(aux_mcu_command_description[0])))
aux_mcu_command_description[0].extend([	"0x0100: get start parents",
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
										"0x010F: get favorites"])
aux_mcu_command_description[0].extend(["invalid"]*(0x8000-len(aux_mcu_command_description[0])))
aux_mcu_command_description[0].extend([	"0x8000: debug message",
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
										"0x800B: reindex bundle"])
# BLE message
aux_mcu_command_description.append(aux_mcu_command_description[0])
# Bootloader message
aux_mcu_command_description.append([])
# Platform details
aux_mcu_command_description.append([])
# Command message
aux_mcu_command_description.append([])
# Aux MCU event
aux_mcu_command_description.append([	"0x0000: invalid",
										"0x0001: ble enabled",
										"0x0002: ble disabled",
										"0x0003: tx sweep done",
										"0x0004: func test done",
										"0x0005: usb enumerated",
										"0x0006: charge done",
										"0x0007: charge failed",
										"0x0008: sleep command received",
										"0x0009: I'm here!",
										"0x000A: BLE connected",
										"0x000B: BLE disconnected",
										"0x000C: USB detached",
										"0x000D: NiMh charge level update",
										"0x000E: USB timeout!",
										"0x000F: here's my status",
										"0x0010: attach command received",
										"0x0011: charge started",
										"0x0012: no comms info received",
										"0x0013: shortcut typed",
										"0x0014: new status received",
										"0x0015: charge stopped",
										"0x0016: new battery level received"])
# NiMh charge
aux_mcu_command_description.append([])
# Ping with info
aux_mcu_command_description.append([])
# Keyboard type
aux_mcu_command_description.append([])
# FIDO2
aux_mcu_command_description.append([])
# RNG transfer
aux_mcu_command_description.append([])
# BLE command
aux_mcu_command_description.append([	"0x0000: invalid",
										"0x0001: enable bluetooth",
										"0x0002: invalid",
										"0x0003: store bond info",
										"0x0004: recall bond info",
										"0x0005: clear bond info",
										"0x0006: enable pairing",
										"0x0007: disable pairing",
										"0x0008: get irk keys",
										"0x0009: recall bond info irk",
										"0x000A: get 6 digit code", 
										"0x000B: disconnect for next device"										
										])


main_mcu_command_description = []
# USB message
main_mcu_command_description.append([	"0x0000: Not valid",
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
										"0x0015: set category strings answer",
										"0x0016: set user language answer",
										"0x0017: get device language answer",
										"0x0018: get user keyboard answer",
										"0x0019: get number of languages answer",
										"0x001A: get number of layouts answer",
										"0x001B: get language description answer",
										"0x001C: get layout description answer",
										"0x001D: get user keyboard answer",
										"0x001E: get user language answer",
										"0x001F: set device language answer"])
main_mcu_command_description[0].extend(["invalid"]*(256-len(main_mcu_command_description[0])))
main_mcu_command_description[0].extend(["0x0100: get start parents answer",
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
										"0x010F: get favorites answer"])
main_mcu_command_description[0].extend(["invalid"]*(0x8000-len(main_mcu_command_description)))
aux_mcu_command_description[0].extend([	"0x8000: debug message",
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
										"0x800B: reindex bundle answer"])
# BLE message
main_mcu_command_description.append(main_mcu_command_description[0])
# Bootloader message
main_mcu_command_description.append([])
# Platform details
main_mcu_command_description.append([])
# Command message
main_mcu_command_description.append([	"0x0000: invalid",
										"0x0001: sleep",
										"0x0002: attach usb",
										"0x0003: slow nimh charge",
										"0x0004: invalid",
										"0x0005: charge battery",
										"0x0006: no comms signal unavailable",
										"0x0007: type shortcut",
										"0x0008: detach USB",
										"0x0009: functional test start",
										"0x000A: update device status",
										"0x000B: stop battery charge",
										"0x000C: set battery level",
										"0x000D: get AUX status",])
# Aux MCU event
main_mcu_command_description.append([])
# NiMh charge
main_mcu_command_description.append([])
# Ping with info
main_mcu_command_description.append([])
# Keyboard type
main_mcu_command_description.append([])
# FIDO2
main_mcu_command_description.append([])
# RNG transfer
main_mcu_command_description.append([])
# BLE command
main_mcu_command_description.append([	"0x0000: invalid",
										"0x0001: enable bluetooth",
										"0x0002: invalid",
										"0x0003: store bond info",
										"0x0004: recall bond info",
										"0x0005: clear bond info",
										"0x0006: enable pairing",
										"0x0007: disable pairing",
										"0x0008: get irk keys",
										"0x0009: recall bond info irk",
										"0x000A: get 6 digit code", 
										"0x000B: disconnect for next device"										
										])

MAIN_MCU_COMMAND_SLEEP          = 0x0001
MAIN_MCU_COMMAND_ATTACH_USB     = 0x0002
MAIN_MCU_COMMAND_PING           = 0x0003
MAIN_MCU_COMMAND_RESERVED       = 0x0004
MAIN_MCU_COMMAND_NIMH_CHARGE    = 0x0005
MAIN_MCU_COMMAND_NO_COMMS_UNAV  = 0x0006
MAIN_MCU_COMMAND_RESERVED2      = 0x0007
MAIN_MCU_COMMAND_DETACH_USB     = 0x0008
MAIN_MCU_COMMAND_FUNC_TEST      = 0x0009
MAIN_MCU_COMMAND_UPDT_DEV_STAT  = 0x000A
MAIN_MCU_COMMAND_STOP_CHARGE    = 0x000B
MAIN_MCU_COMMAND_SET_BATTERYLVL = 0x000C

AUX_MCU_EVENT_BLE_ENABLED		= 0x0001
AUX_MCU_EVENT_BLE_DISABLED		= 0x0002
AUX_MCU_EVENT_TX_SWEEP_DONE		= 0x0003
AUX_MCU_EVENT_FUNC_TEST_DONE	= 0x0004
AUX_MCU_EVENT_USB_ENUMERATED	= 0x0005
AUX_MCU_EVENT_CHARGE_DONE		= 0x0006
AUX_MCU_EVENT_CHARGE_FAIL		= 0x0007

BLE_MESSAGE_CMD_ENABLE          = 0x0001
BLE_MESSAGE_CMD_DISABLE         = 0x0002
BLE_MESSAGE_STORE_BOND_INFO     = 0x0003
BLE_MESSAGE_RECALL_BOND_INFO    = 0x0004