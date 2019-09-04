#!/usr/bin/python
# -*- coding: utf-8 -*-
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

# These are the HID modifier keys.  We create a single byte value
# with the combination of the modifier keys pressed.
keys_map = {'ctrlL': 	1 << 0,
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
		#     A dictonary:
		#		Key: Name of the platform
		#		Value: Layout dictionary
		#	  Layout Dictonary:
		#	 	Key: Name of the layout
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
								if len(points) == 1 and points[0] > 0x20:
									if layout_name not in self.layouts[platform_name].keys():
										self.layouts[platform_name][layout_name] = {} # unicode point -> set(modifier hex byte, keycode hex byte)
										
									# check for presence in dictionary 
									if points[0] in self.layouts[platform_name][layout_name]:
										if False and obj.attrib.get('locale') == "fr-t-k0-windows":
											print(glyphs, "-", points[0], "mapped to", mf, int(keycode), c.attrib.get('iso'), "already present in our dictionary:", self.layouts[platform_name][layout_name][points[0]][0], self.layouts[platform_name][layout_name][points[0]][1], self.layouts[platform_name][layout_name][points[0]][2]) 
									else:
										self.layouts[platform_name][layout_name][points[0]] = (mf, int(keycode), c.attrib.get('iso'))
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


	def show_lut(self, platform_name, layout_name):
		layouts = self.layouts[platform_name]
		layout = layouts[layout_name]
		table = []
		table.append(["Glyph", "Unicode", "modifier+keycode", "Description"])
		for key in layout:
			mod, keycode, isocode = layout[key]
			try:
				des = unicodedata.name(chr(key))
			except:
				des = "No Data"
			table.append([chr(key), key, "+".join(mod) + " " + str(keycode), des])
		for row in table:
			print("{0: <5} {1: >15} {2: >20} {3: >40}".format(*row))


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
		print(" decomposition.  The code points can be represented with ")
		print(" %s unique glyph images." % len(npoints))


# First action is to parse...
print("Parsing CLDR files...")
cldr = CLDR()
cldr.parse_cldr_xml()

# now you can just access cldr.layouts directly if you want..

#cldr.show_platforms()
#cldr.show_layouts(1) # osx
#
cldr.show_lut("windows", "French") # French
#
#cldr.show_stats()

