#!/usr/bin/python
# -*- coding: utf-8 -*-
from iso_to_hid_lut import *
from lxml import objectify
from pprint import pprint
from lxml import etree
import unicodedata
import statistics
import sys
import re
import os

# Set the directory that you extract the cldr keyboards zip file here
CLDR_KEYBOARDS_BASE_PATH = os.path.join("cldr-keyboards-35.1", "keyboards")
# the platform filename in the cldr, contains HID code to physical key code LUT
PLATFORM_FILENAME = "_platform.xml"

# These are the HID modifier keys.	We create a single byte value
# with the combination of the modifier keys pressed.
keys_map = {'ctrlL':	1 << 0,
			'shiftL':	1 << 1,
			'shift':	1 << 1,
			'atlL':		1 << 2,
			'opt':		1 << 2,
			'cmd':		1 << 3,
			'ctrlR':	1 << 4,
			'shiftR':	1 << 5,
			'altR':		1 << 6,
			'cmdR':		1 << 7}

# This class contains a simple parser for the cldr keyboard files. 
# The main data structure that we get as a result is 'layouts'
class CLDR():
	def __init__(self):
		
		# The format of the layouts.
		#	  A dictonary:
		#		Key: Name of the platform
		#		Value: Layout dictionary
		#	  Layout Dictonary:
		#		Key: Name of the layout
		#		Value: Unicode to hid dictonary
		#	  Unicode to hid dictonary:
		#		Key: unicode point (integer)
		#		Value: hid modfier byte and hid keycode byte (tuple of strings (hex of byte value))
		#
		# platform name -> {layout -> {unicode point -> HID Bytes (modifier(s), keycode)}
		#
		self.layouts = {}

		# The hardware map are ISO codes mapped to HID keycodes
		#
		# platform name -> {iso code string -> hid key code integer as string}
		#
		self.hardware_maps = {}
		
	def parse_cldr_xml(self):

		for directory, dirnames, filenames in os.walk(CLDR_KEYBOARDS_BASE_PATH):
			# for platforms that have no hid map, we can't use them
			if PLATFORM_FILENAME not in filenames:
				continue

			# we know there's the hid map, we open it and check for the hardwareMap keywoard
			platfile = os.path.join(directory, PLATFORM_FILENAME)
			pobj = etree.parse(platfile).getroot()
			map = pobj.find('hardwareMap')
			
			if map is not None:
				# generate iso to keycode LUT
				iso_to_keycode = {i.attrib.get('iso'): i.attrib.get('keycode') for i in map.getchildren()}
				#print(iso_to_keycode)
				
				# extract platform name
				platform_name = pobj.attrib.get('id')
				
				# store iso to keycode LUT for this platform
				self.hardware_maps[platform_name] = iso_to_keycode
				if platform_name not in self.layouts.keys():
					self.layouts[platform_name] = {}

				# process each layout file
				for f in filenames:
					# only parse keyboard files
					if f != PLATFORM_FILENAME:
						# open keyboard file
						keyb_file = os.path.join(directory, f)
						obj = etree.parse(keyb_file).getroot()
						
						# get layout name
						layout_name = obj.find('names').find('name').attrib.get('value')
						#print("Parsing layout " + layout_name + " on platform " + platform_name + ", file " + f)

						# Iterate over the keymaps
						for m in obj.iter("keyMap"):
							# get modifier keys, check for caps lock
							caps, mf = self.get_modifier_keys(m.attrib.get('modifiers'))
							#print(m.attrib.get('modifiers'))
							
							# we don't support characters that require caps lock
							# if some other combination can also yield this key, caps will be false.
							if caps:
								continue

							for c in m.getchildren():
								# todo: figure out why every other node seems blank, xml parser thing?
								if len(c.attrib) == 0:
									continue

								# hid keycode
								keycode = iso_to_keycode.get(c.attrib.get('iso'))
								hidcode = iso_to_hid_dic[c.attrib.get('iso')]

								# If no keycode can be found, we can't type it, so it gets skipped.
								# Only known case for this in CLDR 32, is chromeos iso code C12.
								if keycode is None:
									continue
									
								# Debug
								if False:
									print("locale", obj.attrib.get('locale'))
									print("modifier", mf)
									print("iso", c.attrib.get('iso'))

								# Get glyphs and points for the "to" attribute
								glyphs, points = self.parse_to_attrib(c.attrib.get('to'), obj.attrib.get('locale'), mf, c.attrib.get('iso'))
								
								#if platform_name == "windows" and layout_name == "French":
								#	print keycode, c.attrib.get('to')
								#	print glyphs
								#	print points
								#	print ""
								
								# Check for a single point: more than one point come from a bad python it seems (arabic, myanmar....)
								if len(points) == 1 and points[0] >= 0x20:
									if layout_name not in self.layouts[platform_name].keys():
										self.layouts[platform_name][layout_name] = {} # unicode point -> set(modifier hex byte, keycode hex byte)
										
									# check for presence in dictionary 
									if points[0] in self.layouts[platform_name][layout_name] and len(self.layouts[platform_name][layout_name][points[0]][0]) < len(mf):
										if False and obj.attrib.get('locale') == "fr-t-k0-windows":
											print(glyphs, "-", points[0], "mapped to", mf, int(keycode), c.attrib.get('iso'), "already present in our dictionary:", self.layouts[platform_name][layout_name][points[0]][0], self.layouts[platform_name][layout_name][points[0]][1], self.layouts[platform_name][layout_name][points[0]][2]) 
									else:
										self.layouts[platform_name][layout_name][points[0]] = (mf, int(keycode), c.attrib.get('iso'), hidcode)
								else:
									if False:
										print("multiple points")
										print("locale", obj.attrib.get('locale'))
										print("modifier", mf)
										print("glyphs", glyphs)
										print("points", points)
										print("iso", c.attrib.get('iso'))
										#print "to", unicode(c.attrib.get('to'))
										print("")
										
						# Check that all basic ASCII codes are present, and fill them if not
						for i in range(0x20, 0x7F):
							if i not in self.layouts[platform_name][layout_name]:
								self.layouts[platform_name][layout_name][i] = ([], 0, "", 0)
						
						# print our LUT, debug
						if False and obj.attrib.get('locale') == "fr-t-k0-windows":
							for key in sorted(self.layouts[platform_name][layout_name]):
								print(hex(key))
								pprint(self.layouts[platform_name][layout_name][key])
								print("")
							#pprint(self.layouts[platform_name][layout_name])
							#sys.exit(0)
							print("")
							print("")
							
						# print intervals, debug
						if False and obj.attrib.get('locale') == "fr-t-k0-windows":
							print(self.get_unicode_intervals(self.layouts[platform_name][layout_name], 100))
						if False:
							print(len(self.get_unicode_intervals(self.layouts[platform_name][layout_name], 100)))
							print(obj.attrib.get('locale'), self.get_unicode_intervals(self.layouts[platform_name][layout_name], 100))
							
						if False:
							sys.exit(0)

							
	# print unicode values intervals for a given max interval
	def get_unicode_intervals(self, dictionary, max_interval):
		# sort dictionary by keys (unicode values)
		ordered_keys = sorted(dictionary)
		
		# get first element
		start_key = ordered_keys[0]
		last_key = ordered_keys[0]
		
		# list of intervals
		interval_list = []
		
		# start looping
		for key in ordered_keys:
			# check for new interval
			if key > last_key + max_interval:
				interval_list.append([hex(start_key), hex(last_key)])
				start_key = key
				last_key = key
			else:
				last_key = key
		
		# add last interval
		interval_list.append([hex(start_key), hex(last_key)])
		
		return interval_list

	# Returns if caps is required and the Hex value of modifier HID byte
	def get_modifier_keys(self, mds):
		# No modifier keys :)
		if mds is None:
			return False, []
			
		# Different options for modifier, just take the first one (simplest one!)
		options = mds.split(" ")
		opt = options[0]
		
		# Modifier key(s)
		keys = opt.split("+")
		final_keys = []
		for key in keys:
			# from our tests, when a modifier is finished with ? we should discard it
			if not key.endswith("?"):
				final_keys.append(key)
				
		# return the modifier key and if caps lock is neeeded
		if 'caps' in keys:
			return True, final_keys
		else:
			return False, final_keys

	def describe(self, glyphs):
		glyphs, points = parse_to_attrib(glyphs)
		if len(points) == 1:
			try:
				desc = unicodedata.category(glyphs[0]) + ": " + unicodedata.name(glyphs[0])
			except:
				desc = "can't describe " + repr(glyphs[0])
		else:
			desc = "Multiple"

		return desc

	def parse_to_attrib(self, glyph, locale, modifier, iso):
		# this just handles the odd formatting in the 'to' attribute on the xml.
		# They mix xmlencoded and actual unicode, and also the code point
		# encoded with a \u{...} notation.
		# 
		# Returns: list of glyps, list of code point ints
		#print("Dealing with char " + glyph + " with modifier " + " ".join(modifier))
		
		# find all the \u{...} notation characters
		myre = re.compile(r"\\u\{([A-F0-7]*[^\}])", re.UNICODE)
		matches = myre.findall(glyph)
		wordlist = []
		if len(matches):
			for m in matches:
				mt = "\\u{" + m + "}"
				m = int(m, 16)
				loc = glyph.find(mt)
				if loc > 0:
					for c in glyph[0:loc]:
						wordlist.append(c)
					glyph = glyph[loc:]
				wordlist.append(chr(m))
				glyph_copy = glyph
				glyph = glyph[len(mt):]
			#print(glyph_copy + " converted to " + " ".join(wordlist))
		else:
			wordlist = [c for c in glyph]

		glyphs = []
		points = []
		for c in wordlist:
			try:
				point = ord(c)
			except:
				if True:
					print("error getting ord of", c)
					print("main reason: multiple keys")
					print("locale", locale)
					print("modifier", modifier)
					print("iso", iso)
					print("")
					sys.exit(0)
				point = ord("-")
			glyphs.append(c)
			points.append(point)

		return glyphs, points

	def show_platforms(self):
		for k, v in self.layouts.iteritems():
			print(self.layouts.keys().index(k), k)
		

	def show_layouts(self, platform):
		platform_name = self.layouts.keys()[platform]
		layouts = self.layouts[platform_name]
			
		for k, v in layouts.iteritems():
			print(layouts.keys().index(k), k)


	def show_lut(self, platform_name, layout_name, debug_print):
		layouts = self.layouts[platform_name]
		layout = layouts[layout_name]
		sorted_keys = sorted(layout)
		table = []
		glyphs = ""
		table.append(["Glyph", "Unicode", "HID code", "modifier+isocode", "modifier+scancode", "Description"])
		for key in sorted_keys:
			mod, keycode, isocode, hidcode = layout[key]
			try:
				des = unicodedata.name(chr(key))
			except:
				des = "No Data"
			table.append([chr(key), key, f"{hidcode:#0{4}x}", "+".join(mod) + " " + str(isocode),"+".join(mod) + " " + str(keycode), des])
			glyphs+=chr(key)
		if debug_print:
			for row in table:
				print("{0: >10} {1: >10} {2: >10} {3: >20} {4: >20} {5: >40}".format(*row))			
			print("\r\nAll glyphs:\r\n" + ''.join(sorted(glyphs)))
		
		# Part below is to compare with mooltipass mini storage
		mini_lut_array_bin = []
		if debug_print:
			print("\r\nMooltipass Mini Old LUT:")
		mini_modifier_map = {	'ctrlL':	0x00,
								'shiftL':	0x80,
								'shift':	0x80,
								'atlL':		0x40,
								'opt':		0x00,
								'cmd':		0x00,
								'ctrlR':	0x00,
								'shiftR':	0x80,
								'altR':		0x40,
								'cmdR':		0x00}	
		mini_lut = ""
		for key in sorted_keys:
			mod, keycode, isocode, hidcode = layout[key]
			modifier_mask = 0x00
			for modifier in mod:
				modifier_mask |= mini_modifier_map[modifier]
			# Europe key hack
			if hidcode == 0x64:
				hidcode = 0x03
			# Apply modifier mask
			hidcode |= modifier_mask
			mini_lut += f"{hidcode:#0{4}x} "
			mini_lut_array_bin.append(hidcode)
		if debug_print:
			print(mini_lut)
		
		# Return dictionary
		return {"mini_lut_bin": mini_lut_array_bin, "covered_glyphs":glyphs}

	def raw_layouts(self):
		layouts = []
		for platname, plat_dict in self.layouts.iteritems():
			for layoutname, layout_dict in plat_dict.iteritems():
				layouts.append(layout_dict)

		points = set()
		for l in layouts:
			for k,v in l.iteritems():
				points = points.union(set([k]))

		return layouts, points


	def show_stats(self):
		layouts, points = self.raw_layouts()

		print("Showing stats for %s layouts." % len(layouts))
		print("Maximum number of characters in a layout: %s" % max([len(ldict) for ldict in layouts]))
		print("Mean number of characters in a layout: %s" % statistics.mean([len(ldict) for ldict in layouts]))
		print("Median number of characters in a layout: %s" % statistics.median([len(ldict) for ldict in layouts]))
		print("Now... assuming 2 byte for unicode, 2 bytes for hid (modifier+keycode):")
		print("All maps add up to %s Bytes" % sum([len(ldict) * 4  for ldict in layouts]))
		print("Average map size is %s Bytes" % (statistics.mean([len(ldict) for ldict in layouts]) * 4))
		print("\nUnique unicode characters is %s" % len(points))

		npoints = set()
		for p in points:
			npoints = npoints.union(set(unicodedata.normalize('NFKD', unichr(p))))

		print("\nNormalizing those unique unicode characters via ")
		print("the normal form KD (NFKD) will apply the compatibility ")
		print(" decomposition.	The code points can be represented with ")
		print(" %s unique glyph images." % len(npoints))


# First action is to parse...
print("Parsing CLDR files...")
cldr = CLDR()
cldr.parse_cldr_xml()

# now you can just access cldr.layouts directly if you want..

#cldr.show_platforms()
#cldr.show_layouts(1) # osx
#


if True:
	# Test code: compare LUT generated this way to an original file
	mini_luts = ["18_EN_US_keyb_lut.img", "19_FR_FR_keyb_lut.img", "20_ES_ES_keyb_lut.img", "21_DE_DE_keyb_lut.img", "22_ES_AR_keyb_lut.img", "23_EN_AU_keyb_lut.img", "24_FR_BE_keyb_lut.img", "25_PO_BR_keyb_lut.img", "26_EN_CA_keyb_lut.img", "27_CZ_CZ_keyb_lut.img", "28_DA_DK_keyb_lut.img", "29_FI_FI_keyb_lut.img", "30_HU_HU_keyb_lut.img", "31_IS_IS_keyb_lut.img", "32_IT_IT_keyb_lut.img", "33_NL_NL_keyb_lut.img", "34_NO_NO_keyb_lut.img", "35_PO_PO_keyb_lut.img", "36_RO_RO_keyb_lut.img", "37_SL_SL_keyb_lut.img", "38_FRDE_CH_keyb_lut.img", "39_EN_UK_keyb_lut.img", "45_CA_FR_keyb_lut.img", "49_POR_keyb_lut.img", "51_CZ_QWERTY_keyb_lut.img", "52_EN_DV_keyb_lut.img"]
	matching_cldr_names = ["United States-International", "French", "Spanish", "German", "Latin American", "United States-International", "Belgian French", "Portuguese (Brazil ABNT2)", "Canadian Multilingual Standard", "Czech", "Danish", "Finnish", "Hungarian", "Icelandic", "Italian", "Dutch", "Norwegian", "Polish (Programmers)", "Romanian (Programmers)", "Slovenian", "Swiss French", "United Kingdom Extended", "Canadian French", "Portuguese", "Czech (QWERTY)", "United States-Dvorak"]

	for lut, cldr_name in zip(mini_luts, matching_cldr_names):
		print("\r\nTackling mini LUT " + lut + " with matching cldr name " + cldr_name)
		output_dict = cldr.show_lut("windows", cldr_name, False)
		print("Glyphs: " + output_dict["covered_glyphs"])
		with open(os.path.join("mini_luts", lut), "rb") as f:
			counter = 0
			byte = f.read(1)
			match_bool = True
			while byte != b"":
				if ord(byte) != output_dict["mini_lut_bin"][counter]:
					print("Mismatch for character " + chr(0x20 + counter) + ", ord: " + hex(0x20 + counter) + ", cldr: " + hex(output_dict["mini_lut_bin"][counter]) + " lut: " + hex(ord(byte)))
					match_bool = False
				byte = f.read(1)
				counter += 1
		if match_bool:
			print("Match!")
		else:
			print("No Match!")
			#input("Confirm:")

