#!/usr/bin/python
# -*- coding: utf-8 -*-
from mini_mooltipass_coms import *
from iso_to_hid_lut import *
from lxml import objectify
from pprint import pprint
from lxml import etree
from struct import *
import unicodedata
import statistics
import sys
import re
import os

# Set the directory that you extract the cldr keyboards zip file here
CLDR_KEYBOARDS_BASE_PATH = os.path.join("cldr-keyboards-35.1", "keyboards")
# the platform filename in the cldr, contains HID code to physical key code LUT
PLATFORM_FILENAME = "_platform.xml"
# keycodes that can't be used in the mooltipass
forbidden_isocodes = ["B11"]

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
		self.transforms = {}

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
					self.transforms[platform_name] = {}

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
									
						# Create transform dictionary if not already created
						if layout_name not in self.transforms[platform_name].keys():
							self.transforms[platform_name][layout_name] = {}
								
						# Deal with transforms first
						for m in obj.iter("transforms"):
							transform_type = m.attrib.get('type')							
							for c in m.getchildren():
								# todo: figure out why every other node seems blank, xml parser thing?
								if len(c.attrib) == 0:
									continue
									
								# Extract and transform transformation
								from_glyphs, to_glyphs = self.transform_from_to_transform(c.attrib.get('from'), c.attrib.get('to'))
								#print("From " + from_glyphs + " to " + to_glyphs)
								
								# For example: deadkey before key transforms into key before deadkey... we skip them as we can still type them by knowing what is a deadkey
								if len(to_glyphs) > 1:
									continue
									
								# Yep... these exist...
								if len(to_glyphs) == 0:
									continue
									
								# Add transform
								self.transforms[platform_name][layout_name][to_glyphs] = from_glyphs

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
									
								# Check for forbidden keycode
								if c.attrib.get('iso') in forbidden_isocodes:
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
										# Check for deadkey
										is_dead_key = False
										for to_transform in self.transforms[platform_name][layout_name]:
											if to_transform == chr(points[0]) and self.transforms[platform_name][layout_name][to_transform][1] == " ":
												is_dead_key = True
										
										if is_dead_key:
											# Found deadkey: remove it from our transform list and add tag it
											del self.transforms[platform_name][layout_name][chr(points[0])]
											self.layouts[platform_name][layout_name][points[0]] = (mf, int(keycode), c.attrib.get('iso'), hidcode, True)
										else:
											self.layouts[platform_name][layout_name][points[0]] = (mf, int(keycode), c.attrib.get('iso'), hidcode, False)
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
								self.layouts[platform_name][layout_name][i] = ([], 0, "", 0, False)
						
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
	def get_unicode_intervals(self, platform_name, layout_name, max_interval):
		# Get and sort direct mappings and transforms
		layout = self.layouts[platform_name][layout_name]
		sorted_keys = sorted(layout)
		transforms = self.transforms[platform_name][layout_name]
		sorted_transform_keys = sorted(transforms)
		glyph_ords = []
		
		# Add all items
		for key in sorted_keys:
			glyph_ords.append(key)
		for transform in sorted_transform_keys:
			glyph_ords.append(ord(transform))
		
		# get first element
		start_key = glyph_ords[0]
		last_key = glyph_ords[0]
		
		# list of intervals
		interval_list = []
		
		# start looping
		for key in sorted(glyph_ords):
			# check for new interval
			if key > last_key + max_interval:
				interval_list.append([start_key, last_key])
				start_key = key
				last_key = key
			else:
				last_key = key
		
		# add last interval
		interval_list.append([start_key, last_key])
		
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
		
	def transform_from_to_transform(self, from_glyphs, to_glyphs):
		# find all the \u{...} notation characters
		myre = re.compile(r"\\u\{([A-F0-7]*[^\}])", re.UNICODE)
		matches = myre.findall(from_glyphs)
		return_from_glyphs = ""
		if len(matches):
			for m in matches:
				mt = "\\u{" + m + "}"
				m = int(m, 16)
				loc = from_glyphs.find(mt)
				if loc > 0:
					for c in from_glyphs[0:loc]:
						return_from_glyphs += c
					from_glyphs = from_glyphs[loc:]
				return_from_glyphs += chr(m)
				from_glyphs = from_glyphs[len(mt):]
			if len(from_glyphs) > 0:
				for glyph in from_glyphs:
					return_from_glyphs += glyph
		else:
			return_from_glyphs = from_glyphs
			
		# Do the same for "to"		
		matches = myre.findall(to_glyphs)
		return_to_glyphs = ""
		if len(matches):
			for m in matches:
				mt = "\\u{" + m + "}"
				m = int(m, 16)
				loc = to_glyphs.find(mt)
				if loc > 0:
					for c in to_glyphs[0:loc]:
						return_to_glyphs += c
					to_glyphs = to_glyphs[loc:]
				return_to_glyphs += chr(m)
				to_glyphs = to_glyphs[len(mt):]
			if len(to_glyphs) > 0:
				for glyph in to_glyphs:
					return_to_glyphs += glyph
		else:
			return_to_glyphs = to_glyphs
			
		return return_from_glyphs, return_to_glyphs

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
			if len(glyph) > 0:
				for g in glyph:
					wordlist.append(g)
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
		layout = self.layouts[platform_name][layout_name]
		sorted_keys = sorted(layout)
		table = []
		glyphs = ""
		table.append(["Glyph", "Unicode", "HID code", "modifier+isocode", "modifier+scancode", "Description"])
		for key in sorted_keys:
			mod, keycode, isocode, hidcode, deadkey = layout[key]
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
			
		# Glyphs generated by transforms
		transforms = self.transforms[platform_name][layout_name] 
		sorted_transforms = sorted(transforms)
		glyphs_from_transforms = ""
		for key in sorted_transforms:
			glyphs_from_transforms += key
			
		# Generate raw HID code + modifier to glyph mapping
		hid_to_glyph_lut = {}
		modifier_map = {	'ctrlL':	0x00,
							'shiftL':	KEY_SHIFT,
							'shift':	KEY_SHIFT,
							'atlL':		KEY_RIGHT_ALT,
							'opt':		0x00,
							'cmd':		0x00,
							'ctrlR':	0x00,
							'shiftR':	KEY_SHIFT,
							'altR':		KEY_RIGHT_ALT,
							'cmdR':		0x00}	
		for key in sorted_keys:
			mod, keycode, isocode, hidcode, deadkey = layout[key]
			modifier_mask = 0x00
			for modifier in mod:
				modifier_mask |= modifier_map[modifier]
			hid_to_glyph_lut[chr(key)] = [[modifier_mask,hidcode]]
		#print(hid_to_glyph_lut)
		
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
			mod, keycode, isocode, hidcode, deadkey = layout[key]
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
		return {"mini_lut_bin": mini_lut_array_bin, "covered_glyphs":glyphs, "hid_to_glyph_lut":hid_to_glyph_lut, "glyphs_from_transforms":glyphs_from_transforms, "transforms":transforms}


	def generate_mini_ble_lut(self, platform_name, layout_name, debug_print, layout_description, export_file_name):
		layout = self.layouts[platform_name][layout_name]
		sorted_keys = sorted(layout)
		lut_bin_dict = {}
		dead_keys = []
		mini_modifier_map = {	'ctrlL':	0x00,
								'ctrl':		0x00,
								'shiftL':	0x80,
								'shift':	0x80,
								'atlL':		0x40,
								'optL':		0x40,
								'opt':		0x00,
								'cmd':		0x00,
								'ctrlR':	0x00,
								'shiftR':	0x80,
								'altR':		0x40,
								'optR':		0x40,
								'cmdR':		0x00}
		# Check for " " as starting key
		if sorted_keys[0] != ord(" "):
			print("Incorrect starting key")
			sys.exit(0)
			
		# Start by adding standard keys
		for key in sorted_keys:
			mod, keycode, isocode, hidcode, deadkey = layout[key]
			modifier_mask = 0x00
			for modifier in mod:
				modifier_mask |= mini_modifier_map[modifier]
			# Europe key hack
			if hidcode == 0x64:
				hidcode = 0x03
			# Check for problematic key code
			if hidcode >= 0x40:
				print("incorrect hid key code!")
				sys.exit(0)
			# Apply modifier mask
			hidcode |= modifier_mask
			# Add to our dictionary
			lut_bin_dict[key] = [hidcode]
			# If deadkey, add to our array
			if deadkey == True:
				dead_keys.append(key)
				
		# Now move to the transforms
		transforms = self.transforms[platform_name][layout_name]
		sorted_transform_keys = sorted(transforms)
		for transform in sorted_transform_keys:
			key_sequence = []
			for glyph in transforms[transform]:
				key_of_interest = 0
				if ord(glyph) not in lut_bin_dict:
					# This is not important as the transform that may error will have another that will be ok
					print("Transform for: \"" + transform + "\" couldn't find \"" + glyph + "\" (ord " + str(ord(glyph)) + ") in lut dic - likely unimportant")
					key_of_interest = lut_bin_dict[ord('a')]
				else:
					key_of_interest = lut_bin_dict[ord(glyph)]
				
				key_sequence.extend(key_of_interest)	
			lut_bin_dict[ord(transform)] = key_sequence
			
		# Debug prints
		if debug_print:
			print(lut_bin_dict)
			print("Deadkeys: " + "/".join(chr(i) for i in dead_keys))
					
		# Language description (max 19 chars unicode, then we append 0)
		language_desc_item = array('B')
		i = 0
		for c in layout_description:
			language_desc_item.frombytes(pack('H', ord(c)))
			i += 1
		if i > 19:
			print("Language description too long!")
			sys.exit(0)
		for remain_i in range(i, 20):
			language_desc_item.frombytes(pack('H', 0))
			
		# Intervals (max 15 intervals, max 20 keys non described)
		intervals_item = array('B')		
		start_key = sorted(lut_bin_dict)[0]
		last_key = start_key
		intervals_added = 0
		interval_list = []
		
		# start looping
		for key in sorted(lut_bin_dict):
			# check for new interval
			if key > last_key + 20:
				intervals_item.frombytes(pack('H', start_key))
				intervals_item.frombytes(pack('H', last_key))
				interval_list.append([start_key, last_key])
				intervals_added += 1
				start_key = key
				last_key = key
			else:
				last_key = key
		
		# add last interval
		intervals_item.frombytes(pack('H', start_key))
		intervals_item.frombytes(pack('H', last_key))
		interval_list.append([start_key, last_key])
		intervals_added += 1
		
		# 0 pad interval list
		for remain_interval in range(intervals_added, 15):
			intervals_item.frombytes(pack('H', 0xFFFF))
			intervals_item.frombytes(pack('H', 0xFFFF))
			
		# Debug
		if debug_print:
			print(interval_list)
		
		# All is left is LUT populate
		lut_item = array('B')		
		for interval in interval_list:
			for glyph_ord in range(interval[0], interval[1]+1):
				# Do we handle this glyph?
				if glyph_ord not in lut_bin_dict or lut_bin_dict[glyph_ord] == 0:
					lut_item.frombytes(pack('H', 0xFFFF))
					#print(glyph_ord)
				else:
					# Check for key combination
					if len(lut_bin_dict[glyph_ord]) == 1:
						# Check for deadkey
						if glyph_ord in dead_keys:
							lut_item.frombytes(pack('H', 0x8000 | lut_bin_dict[glyph_ord][0]))
							#print(glyph_ord)
						else:
							lut_item.frombytes(pack('H', lut_bin_dict[glyph_ord][0]))
					else:
						# endianness...
						lut_item.frombytes(pack('B', lut_bin_dict[glyph_ord][1]))
						lut_item.frombytes(pack('B', lut_bin_dict[glyph_ord][0]))
		
		# Just write the damn file
		bfd = open(export_file_name, "wb")
		language_desc_item.tofile(bfd)
		intervals_item.tofile(bfd)
		lut_item.tofile(bfd)
		bfd.close()
			
		# Return LUT and deadkeys
		return lut_bin_dict, dead_keys


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

# Generation descriptiona array: platform / layout / description
keyboard_generation_array = [ \
								[ "windows", "Belgian French", "Belgium French"],\
								[ "windows", "Portuguese (Brazil ABNT)", "Brazil"],\
								[ "windows", "Canadian Multilingual Standard", "Canada"],\
								[ "windows", "Czech", "Czech"],\
								[ "windows", "Czech (QWERTY)", "Czech QWERTY"],\
								[ "windows", "Danish", "Denmark"],\
								[ "windows", "United States-Dvorak", "Dvorak"],\
								[ "windows", "French", "France"],\
								[ "windows", "German", "Germany"],\
								[ "windows", "Hungarian", "Hungary"],\
								[ "windows", "Icelandic", "Iceland"],\
								[ "windows", "Italian", "Italy"],\
								[ "windows", "Latin American", "Latin America"],\
								[ "windows", "Norwegian", "Norway"],\
								[ "windows", "Polish (Programmers)", "Poland"],\
								[ "windows", "Portuguese", "Portugal"],\
								[ "windows", "Romanian (Standard)", "Romania"],\
								[ "windows", "Slovenian", "Slovenia"],\
								[ "windows", "Spanish", "Spain"],\
								[ "windows", "Finnish", "Sweden & Finland"],\
								[ "windows", "Swiss French", "Swiss French"],\
								[ "windows", "United Kingdom Extended", "UK Extended"],\
								[ "windows", "US", "USA"],\
								[ "windows", "United States-International", "US International"],\
								[ "osx", "Belgian", "Belgium (MacOS)"],\
								[ "osx", "Brazilian", "Brazil (MacOS)"],\
								[ "osx", "Canadian", "Canada (MacOS)"],\
								[ "osx", "Czech", "Czech (MacOS)"],\
								[ "osx", "Czech-QWERTY", "Czech QWERT (MacOS)"],\
								[ "osx", "Danish", "Denmark (MacOS)"],\
								[ "osx", "Dvorak", "Dvorak (MacOS)"],\
								[ "osx", "French", "French (MacOS)"],\
								[ "osx", "German", "German (MacOS)"],\
								[ "osx", "Hungarian", "Hungarian (MacOS)"],\
								[ "osx", "Icelandic", "Iceland (MacOS)"],\
								[ "osx", "Italian", "Italy (MacOS)"],\
								[ "osx", "Norwegian", "Norway (MacOS)"],\
								[ "osx", "Polish", "Polish (MacOS)"],\
								[ "osx", "Portuguese", "Portugal (MacOS)"],\
								[ "osx", "Romanian", "Romania (MacOS)"],\
								[ "osx", "Slovenian", "Slovenia (MacOS)"],\
								[ "osx", "Spanish", "Spain (MacOS)"],\
								[ "osx", "Finnish", "Sweden/Fin (MacOS)"],\
								[ "osx", "Swiss French", "Swiss FR (MacOS)"],\
								[ "osx", "British", "UK (MacOS)"],\
								[ "osx", "U.S.", "USA (MacOS)"],\
								[ "osx", "US Extended", "US Extended (MacOS)"],\
							]

counter = 0
for array_item in keyboard_generation_array:
	cldr.generate_mini_ble_lut(array_item[0], array_item[1], False, array_item[2], str(counter) + "_" + array_item[0] + "_" + array_item[1].lower() + ".img")
	print("Generated " + str(counter) + "_" + array_item[0] + "_" + array_item[1].lower() + ".img")
	counter += 1

if False:
	nb_intervals_array = []
	for layout_name in cldr.layouts["windows"]:
		returned_intervals = cldr.get_unicode_intervals("windows", layout_name, 20)
		print(layout_name)	
		print(returned_intervals)
		nb_intervals_array.append(len(returned_intervals))
		if len(returned_intervals) > 15:
			print("\r\n\r\n")
	print(nb_intervals_array)

if False:
	lut_bin_dict, dead_keys = cldr.generate_mini_ble_lut("windows", "French", True)
	#lut_bin_dict, dead_keys = cldr.generate_mini_ble_lut("windows", "Czech", True)
	if mini_check_lut(lut_bin_dict, dead_keys) == False:
		print("Check failed!")

if False:
	done_mini_luts = ["19_FR_FR_keyb_lut.img", "18_EN_US_keyb_lut.img", "20_ES_ES_keyb_lut.img", "21_DE_DE_keyb_lut.img", "22_ES_AR_keyb_lut.img", "24_FR_BE_keyb_lut.img", "25_PO_BR_keyb_lut.img", "27_CZ_CZ_keyb_lut.img", "28_DA_DK_keyb_lut.img", "29_FI_FI_keyb_lut.img", "30_HU_HU_keyb_lut.img", "31_IS_IS_keyb_lut.img", "32_IT_IT_keyb_lut.img", "33_NL_NL_keyb_lut.img", "34_NO_NO_keyb_lut.img", "35_PO_PO_keyb_lut.img", "36_RO_RO_keyb_lut.img", "37_SL_SL_keyb_lut.img", "38_FRDE_CH_keyb_lut.img", "39_EN_UK_keyb_lut.img", "45_CA_FR_keyb_lut.img", "49_POR_keyb_lut.img", "51_CZ_QWERTY_keyb_lut.img", ]
	done_matching_cldr_names = ["French", "US", "Spanish", "German", "Latin American", "Belgian French", "Portuguese (Brazil ABNT)", "Czech", "Danish", "Finnish", "Hungarian", "Icelandic", "Italian", "United States-International", "Norwegian", "Polish (Programmers)", "Romanian (Standard)", "Slovenian", "Swiss French", "United Kingdom Extended", "Canadian Multilingual Standard", "Portuguese", "Czech (QWERTY)", ]
	# Test code: compare LUT generated this way to an original file
	mini_luts = ["52_EN_DV_keyb_lut.img"]
	matching_cldr_names = ["United States-Dvorak"]

	for lut, cldr_name in zip(mini_luts, matching_cldr_names):
		print("\r\nTackling mini LUT " + lut + " with matching cldr name " + cldr_name)
		output_dict = cldr.show_lut("windows", cldr_name, False)
		print("Glyphs: " + output_dict["covered_glyphs"])
		print("Unicode points: " + " ".join(str(ord(x)) for x in output_dict["covered_glyphs"]))
		print("Glyphs from transforms: " + output_dict["glyphs_from_transforms"])
		print("Unicode points: " + " ".join(str(ord(x)) for x in output_dict["glyphs_from_transforms"]))
		
		cldr_lut = []
		with open("..\\..\\..\\mooltipass\\bitmaps\\mini\\" + lut, "rb") as f:
			counter = 0
			byte = f.read(1)
			match_bool = True
			while byte != b"":
				if ord(byte) != output_dict["mini_lut_bin"][counter]:
					print("Mismatch for character " + chr(0x20 + counter) + ", ord: " + hex(0x20 + counter) + ", cldr: " + hex(output_dict["mini_lut_bin"][counter]) + " lut: " + hex(ord(byte)))
					match_bool = False
				cldr_lut.append(output_dict["mini_lut_bin"][counter])
				byte = f.read(1)
				counter += 1
		if match_bool:
			print("Match with Mini LUT!")
		else:
			print("No Match with Mini LUT!")
			#input("Confirm:")
			
		# Double checking with actual device
		input("Change computer layout and confirm:")		
		if mini_check_lut(output_dict["hid_to_glyph_lut"], output_dict["transforms"]) == False:
			print("Check failed!")
