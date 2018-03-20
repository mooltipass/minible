#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import sys
reload(sys)
sys.setdefaultencoding('utf-8')

import unicodedata
import re

from lxml import objectify
from lxml import etree

import statistics

# Set the directory that you extract the cldr keyboards zip file here
CLDR_KEYBOARDS_BASE_PATH = "cldr-keyboards-32.0.1/keyboards"
# the platform filename in the cldr
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
			'cmdR':		1 << 7
			}

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

			platfile = os.path.join(directory, PLATFORM_FILENAME)
			pobj = objectify.fromstring(open(platfile, "r").read())
			map = pobj.find('hardwareMap')
			if map is not None:
				
				iso_to_keycode = {i.attrib.get('iso'): i.attrib.get('keycode') for i in pobj.hardwareMap.getchildren()}
				platform_name = pobj.attrib.get('id')
				
				self.hardware_maps[platform_name] = iso_to_keycode
				if platform_name not in self.layouts.keys():
					self.layouts[platform_name] = {}

				# process each layout file
				for f in filenames:
					if f != PLATFORM_FILENAME:
						keyb_file = os.path.join(directory, f)
						obj = objectify.fromstring(open(keyb_file, "r").read())
						layout_name = obj.find('names').find('name').attrib.get('value')

						maps = obj.find('keyMap')
						for m in maps:
							caps, mf = self.get_modifier_keys(m.attrib.get('modifiers'))
							
							# we don't support characters that require caps lock
							# if some other combination can also yield this key, caps will be false.
							if not caps:
								continue

							for c in m.getchildren():
								# todo: figure out why every other node seems blank, xml parser thing?
								if len(c.attrib) == 0:
									continue

								keycode = iso_to_keycode.get(c.attrib.get('iso'))

								# If no keycode can be found, we can't type it, so it gets skipped.
								# Only known case for this in CLDR 32, is chromeos iso code C12.
								if keycode is None:
									continue

								glyphs, points = self.parse_to_attrib(c.attrib.get('to'))
								
								if len(points) == 1:
									if layout_name not in self.layouts[platform_name].keys():
										self.layouts[platform_name][layout_name] = {} # unicode point -> set(modifier hex byte, keycode hex byte)
									self.layouts[platform_name][layout_name][points[0]] = (mf, hex(int(keycode)))
									


	# Returns if caps is required and the Hex value of modifier HID byte
	def get_modifier_keys(self, mds):
		if mds is None:
			return False, str(hex(0))
		options = mds.split(" ")
		opt = options[0]
		keys = opt.split("+")
		final_keys = ""
		final = 0;
		for key in keys:
			if not key.endswith("?"):
				modcode = keys_map.get(key)
				if modcode:
					final += modcode
				final_keys += key + "+"
		if 'caps' in keys:
			return True, hex(final)
		else:
			return False, hex(final)

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

	def parse_to_attrib(self, glyph):
		''' 
		this just handles the odd formatting in the 'to' attribute on the xml.
		They mix xmlencoded and actual unicode, and also the code point
		encoded with a \u{...} notation.

		Returns: list of glyps, list of code point ints
		'''
		# find all the \u{...} notation characters
		myre = re.compile(r"\\u\{([A-F0-7]*[^\}])", re.UNICODE)
		matches = myre.findall(glyph)
		wordlist = []
		if len(matches):
			for m in matches:
				mt = "\u{" + m + "}"
				m = int(m, 16)
				loc = glyph.find(mt)
				if loc > 0:
					for c in glyph[0:loc]:
						wordlist.append(c)
					glyph = glyph[loc:]
				wordlist.append(unichr(m))
				glyph = glyph[len(mt):]
		else:
			wordlist = [c for c in glyph]

		glyphs = []
		points = []
		glyph = unicode(glyph)
		for c in wordlist:
			if type(c) == unicode:
				point = ord(c)
				glyph = unichr(point)	
			else:
				glyph = unicode(glyph)
				try:
					point = ord(glyph)
				except:
					point = "-"
			glyphs.append(glyph)
			points.append(point)

		return glyphs, points

	def show_platforms(self):
		for k, v in self.layouts.iteritems():
			print self.layouts.keys().index(k), k
		

	def show_layouts(self, platform):
		platform_name = self.layouts.keys()[platform]
		layouts = self.layouts[platform_name]
			
		for k, v in layouts.iteritems():
			print layouts.keys().index(k), k


	def show_lut(self, platform, layout):
		platform_name = self.layouts.keys()[platform]
		layouts = self.layouts[platform_name]
		layout_name = layouts.keys()[layout]
		layout = layouts[layout_name]
		table = []
		table.append(["Glyph", "Unicode", "modifier+keycode", "Description"])
		for k, v in layout.iteritems():
			mod, keycode = v
			try:
				des = unicodedata.name(unichr(k))
			except:
				des = "No Data"
			table.append([unichr(k), k, mod + " " + keycode, des])
		for row in table:
			print("{0: <5} {1: >15} {2: >20} {3: >10}".format(*row))


	def show_stats(self):
		layouts = []
		for platname, plat_dict in self.layouts.iteritems():
			for layoutname, layout_dict in plat_dict.iteritems():
				layouts.append(layout_dict)

		print "Showing stats for %s layouts." % len(layouts)
		print "Maximum number of characters in a layout: %s" % max([len(ldict) for ldict in layouts])
		print "Mean number of characters in a layout: %s" % statistics.mean([len(ldict) for ldict in layouts])
		print "Median number of characters in a layout: %s" % statistics.median([len(ldict) for ldict in layouts])
		print "Now... assuming 2 byte for unicode, 2 bytes for hid (modifier+keycode):"
		print "All maps add up to %s Bytes" % sum([len(ldict) * 4  for ldict in layouts])
		print "Average map size is %s Bytes" % (statistics.mean([len(ldict) for ldict in layouts]) * 4)

		points = set()
		for l in layouts:
			for k,v in l.iteritems():
				points = points.union(set([k]))

		print "Unique unicode characters is %s" % len(points)


# example usage below.

cldr = CLDR()
cldr.parse_cldr_xml()

# now you can just access cldr.layouts directly if you want..

cldr.show_platforms()
cldr.show_layouts(1) # osx

cldr.show_lut(1, 30) # German

cldr.show_stats()

