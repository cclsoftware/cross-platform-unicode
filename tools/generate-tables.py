#!/usr/bin/env python3
#************************************************************************************************
#
# Cross-platform Unicode Library
#
# Copyright (c) 2026 CCL Software Licensing GmbH. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# - Neither the name of the cross-platform-unicode project nor the names of its
#   contributors may be used to endorse or promote products derived from this
#   software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS",
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# Filename    : tools/generate-tables.py
# Description : Generates the Unicode and codepage tables
#
#************************************************************************************************

"""Generates the unicode-*.inc and codepage tables in the tables folder.

Character properties, case mappings and normalization data are read from the
Unicode Character Database (UCD). Pass a folder containing the UCD files with
--ucd, otherwise the files of UNICODE_VERSION are downloaded from unicode.org.

Codepage tables (Windows-1252, Mac Roman, Shift-JIS/CP932) are generated from
the codecs of the Python standard library.

To update to a new Unicode version, change UNICODE_VERSION, run this script and
commit the changed .inc files.
"""

import argparse
import os
import sys
import tempfile
import urllib.request

UNICODE_VERSION = "16.0.0"
UCD_URL = "https://www.unicode.org/Public/{version}/ucd/{file}"
UCD_FILES = [
	"UnicodeData.txt",
	"DerivedCoreProperties.txt",
	"DerivedNormalizationProps.txt",
	"CaseFolding.txt",
	"EastAsianWidth.txt",
	"auxiliary/GraphemeBreakProperty.txt",
	"emoji/emoji-data.txt",
]

# order must match GraphemeBreakClass in unicodegrapheme.h
GRAPHEME_BREAK_CLASSES = [
	"Other",
	"CR",
	"LF",
	"Control",
	"Extend",
	"ZWJ",
	"Regional_Indicator",
	"Prepend",
	"SpacingMark",
	"L",
	"V",
	"T",
	"LV",
	"LVT",
	"Extended_Pictographic",
]

OUTPUT_DIR = os.path.normpath (os.path.join (os.path.dirname (os.path.abspath (__file__)), "../tables"))

HANGUL_SYLLABLE_FIRST = 0xAC00
HANGUL_SYLLABLE_LAST = 0xD7A3
MAX_DECOMPOSITION_LENGTH = 18 # must match kMaxDecompositionLength in unicodenormalization.h

#################################################################################################
# UCD parsing
#################################################################################################

def read_ucd_lines (folder, name):
	with open (os.path.join (folder, name), encoding = "utf-8") as f:
		for line in f:
			line = line.split ("#", 1)[0].strip ()
			if line:
				yield [field.strip () for field in line.split (";")]

def parse_range (text):
	if ".." in text:
		first, last = text.split ("..")
		return int (first, 16), int (last, 16)
	value = int (text, 16)
	return value, value

def parse_code_points (text):
	return [int (value, 16) for value in text.split ()]

class UnicodeData:
	def __init__ (self, folder):
		self.category = {}
		self.combining_class = {}
		self.decomposition = {}			# code point -> (is_compatibility, [code points])
		self.upper = {}
		self.lower = {}
		self.title = {}
		self.properties = {}			# property name -> set of code points
		self.simple_fold = {}
		self.full_fold = {}
		self.read_unicode_data (folder)
		self.read_properties (folder, "DerivedCoreProperties.txt", ["Alphabetic", "Lowercase", "Uppercase", "Cased", "Case_Ignorable"])
		self.read_properties (folder, "DerivedNormalizationProps.txt", ["Full_Composition_Exclusion"])
		self.read_case_folding (folder)
		self.read_ranges (folder, "EastAsianWidth.txt", "east_asian_width")
		self.read_ranges (folder, "GraphemeBreakProperty.txt", "grapheme_break")
		self.read_properties (folder, "emoji-data.txt", ["Extended_Pictographic"])

	def read_unicode_data (self, folder):
		range_start = None
		for fields in read_ucd_lines (folder, "UnicodeData.txt"):
			code = int (fields[0], 16)
			name = fields[1]
			category = fields[2]
			if name.endswith (", First>"):
				range_start = code
				continue
			codes = range (range_start, code + 1) if name.endswith (", Last>") else [code]
			range_start = None
			for c in codes:
				self.category[c] = category
				if int (fields[3]) != 0:
					self.combining_class[c] = int (fields[3])
			if fields[5]:
				decomposition = fields[5].split ()
				is_compatibility = decomposition[0].startswith ("<")
				if is_compatibility:
					decomposition = decomposition[1:]
				self.decomposition[code] = (is_compatibility, [int (value, 16) for value in decomposition])
			if fields[12]:
				self.upper[code] = int (fields[12], 16)
			if fields[13]:
				self.lower[code] = int (fields[13], 16)
			if fields[14]:
				self.title[code] = int (fields[14], 16)

	def read_properties (self, folder, file_name, names):
		for name in names:
			self.properties[name] = set ()
		for fields in read_ucd_lines (folder, file_name):
			if fields[1] in names:
				first, last = parse_range (fields[0])
				self.properties[fields[1]].update (range (first, last + 1))

	def read_ranges (self, folder, file_name, attribute):
		values = {}
		for fields in read_ucd_lines (folder, file_name):
			first, last = parse_range (fields[0])
			for c in range (first, last + 1):
				values[c] = fields[1]
		setattr (self, attribute, values)

	def read_case_folding (self, folder):
		for fields in read_ucd_lines (folder, "CaseFolding.txt"):
			code = int (fields[0], 16)
			status = fields[1]
			mapping = parse_code_points (fields[2])
			if status in ("C", "S"):
				self.simple_fold[code] = mapping[0]
			if status in ("C", "F"):
				self.full_fold[code] = mapping

	def get_category_set (self, category):
		return {c for c, value in self.category.items () if value == category}

	def get_full_decomposition (self, code, compatibility):
		entry = self.decomposition.get (code)
		if entry is None or (entry[0] and not compatibility):
			return [code]
		result = []
		for c in entry[1]:
			result += self.get_full_decomposition (c, compatibility)
		return result

#################################################################################################
# Output helpers
#################################################################################################

def write_table (file_name, description, items, items_per_line = 1):
	path = os.path.join (OUTPUT_DIR, file_name)
	with open (path, "w", encoding = "utf-8", newline = "\n") as f:
		f.write ("// %s\n" % description)
		f.write ("// Generated by tools/generate-tables.py, do not edit!\n")
		for i in range (0, len (items), items_per_line):
			f.write (" ".join (item + "," for item in items[i:i + items_per_line]) + "\n")
	print ("%s: %d entries" % (file_name, len (items)))

def to_ranges (code_points):
	ranges = []
	for c in sorted (code_points):
		if ranges and ranges[-1][1] == c - 1:
			ranges[-1][1] = c
		else:
			ranges.append ([c, c])
	return ranges

def format_range_table (code_points):
	return ["{0x%04X, 0x%04X}" % (first, last) for first, last in to_ranges (code_points)]

def format_mapping_table (mapping):
	return ["{0x%04X, 0x%04X}" % (key, mapping[key]) for key in sorted (mapping)]

def to_utf16 (code_points):
	units = []
	for c in code_points:
		if c >= 0x10000:
			c -= 0x10000
			units += [0xD800 + (c >> 10), 0xDC00 + (c & 0x3FF)]
		else:
			units.append (c)
	return units

def utf16_length (c):
	return 2 if c >= 0x10000 else 1

#################################################################################################
# Character properties and case mappings
#################################################################################################

def generate_properties (data):
	source = "Unicode %s" % UNICODE_VERSION
	write_table ("unicode-alphabetic.inc", "Alphabetic ranges (%s)" % source, format_range_table (data.properties["Alphabetic"]), 4)
	write_table ("unicode-lowercase.inc", "Lowercase ranges (%s)" % source, format_range_table (data.properties["Lowercase"]), 4)
	write_table ("unicode-uppercase.inc", "Uppercase ranges (%s)" % source, format_range_table (data.properties["Uppercase"]), 4)
	write_table ("unicode-cased.inc", "Cased ranges (%s)" % source, format_range_table (data.properties["Cased"]), 4)
	write_table ("unicode-case-ignorable.inc", "Case_Ignorable ranges (%s)" % source, format_range_table (data.properties["Case_Ignorable"]), 4)
	write_table ("unicode-decimal.inc", "Decimal number (Nd) ranges (%s)" % source, format_range_table (data.get_category_set ("Nd")), 4)

def generate_case_mappings (data):
	source = "Unicode %s" % UNICODE_VERSION

	# Simple case mappings must not change the number of UTF-16 code units, strings are converted in place
	for name, mapping in (("upper", data.upper), ("lower", data.lower), ("title", data.title), ("fold", data.simple_fold)):
		for key, value in mapping.items ():
			if utf16_length (key) != utf16_length (value):
				sys.exit ("Simple %s case mapping of U+%04X changes UTF-16 length" % (name, key))

	write_table ("unicode-toupper.inc", "Simple uppercase mappings (%s)" % source, format_mapping_table (data.upper), 4)
	write_table ("unicode-tolower.inc", "Simple lowercase mappings (%s)" % source, format_mapping_table (data.lower), 4)

	# Titlecase defaults to uppercase, store differences only
	title = {}
	for c in set (data.title) | set (data.upper):
		value = data.title.get (c, data.upper.get (c, c))
		if value != data.upper.get (c, c):
			title[c] = value
	write_table ("unicode-totitle.inc", "Simple titlecase mappings that differ from uppercase (%s)" % source, format_mapping_table (title), 4)

	# Simple case folding defaults to lowercase, store differences only
	fold = {}
	for c in set (data.simple_fold) | set (data.lower):
		value = data.simple_fold.get (c, c)
		if value != data.lower.get (c, c):
			fold[c] = value
	write_table ("unicode-casefold.inc", "Simple case foldings that differ from lowercase (%s)" % source, format_mapping_table (fold), 4)

	# Full case foldings that expand to multiple characters
	items = []
	for c in sorted (data.full_fold):
		mapping = data.full_fold[c]
		if len (mapping) > 1:
			if len (mapping) > 3:
				sys.exit ("Full case folding of U+%04X is too long" % c)
			mapping = mapping + [0] * (3 - len (mapping))
			items.append ("{0x%04X, {0x%04X, 0x%04X, 0x%04X}}" % tuple ([c] + mapping))
	write_table ("unicode-casefold-full.inc", "Full case foldings with multiple characters (%s)" % source, items, 2)

#################################################################################################
# Normalization
#################################################################################################

def generate_normalization (data):
	source = "Unicode %s" % UNICODE_VERSION

	# Canonical combining classes
	items = []
	ranges = []
	for c in sorted (data.combining_class):
		value = data.combining_class[c]
		if ranges and ranges[-1][1] == c - 1 and ranges[-1][2] == value:
			ranges[-1][1] = c
		else:
			ranges.append ([c, c, value])
	for first, last, value in ranges:
		items.append ("{0x%04X, 0x%04X, %d}" % (first, last, value))
	write_table ("unicode-combining-class.inc", "Canonical combining class ranges (%s)" % source, items, 4)

	# Full decompositions (Hangul syllables are decomposed algorithmically)
	pool = []
	items = []
	for c in sorted (data.decomposition):
		if HANGUL_SYLLABLE_FIRST <= c <= HANGUL_SYLLABLE_LAST:
			continue
		canonical = data.get_full_decomposition (c, False)
		compatibility = data.get_full_decomposition (c, True)
		canonical_units = to_utf16 (canonical) if canonical != [c] else []
		compatibility_units = to_utf16 (compatibility) if compatibility != canonical else []
		if len (canonical) > MAX_DECOMPOSITION_LENGTH or len (compatibility) > MAX_DECOMPOSITION_LENGTH:
			sys.exit ("Decomposition of U+%04X is too long" % c)
		items.append ("{0x%04X, %d, %d, %d}" % (c, len (pool), len (canonical_units), len (compatibility_units)))
		pool += canonical_units + compatibility_units
	if len (pool) > 0xFFFF:
		sys.exit ("Decomposition data too large")
	write_table ("unicode-decomposition.inc", "Full decompositions: code point, data offset, canonical length, compatibility length (%s)" % source, items, 4)
	write_table ("unicode-decomposition-data.inc", "Full decomposition data in UTF-16 (%s)" % source, ["0x%04X" % unit for unit in pool], 12)

	# Primary composites
	compositions = []
	excluded = data.properties["Full_Composition_Exclusion"]
	for c, (is_compatibility, mapping) in data.decomposition.items ():
		if is_compatibility or c in excluded or HANGUL_SYLLABLE_FIRST <= c <= HANGUL_SYLLABLE_LAST:
			continue
		if len (mapping) != 2:
			sys.exit ("Unexpected canonical decomposition of U+%04X" % c)
		compositions.append ((mapping[0], mapping[1], c))
	items = ["{0x%04X, 0x%04X, 0x%04X}" % entry for entry in sorted (compositions)]
	write_table ("unicode-composition.inc", "Canonical compositions: first, second, composite (%s)" % source, items, 3)

#################################################################################################
# Text segmentation and width
#################################################################################################

def to_class_ranges (values):
	ranges = []
	for c in sorted (values):
		if ranges and ranges[-1][1] == c - 1 and ranges[-1][2] == values[c]:
			ranges[-1][1] = c
		else:
			ranges.append ([c, c, values[c]])
	return ranges

def generate_segmentation (data):
	source = "Unicode %s" % UNICODE_VERSION

	# Grapheme_Cluster_Break, Extended_Pictographic is stored as an additional class
	classes = {c: GRAPHEME_BREAK_CLASSES.index (value) for c, value in data.grapheme_break.items ()}
	for c in data.properties["Extended_Pictographic"]:
		if c in classes:
			sys.exit ("Extended_Pictographic U+%04X has Grapheme_Cluster_Break %s" % (c, data.grapheme_break[c]))
		classes[c] = GRAPHEME_BREAK_CLASSES.index ("Extended_Pictographic")
	items = ["{0x%04X, 0x%04X, %d}" % (first, last, value) for first, last, value in to_class_ranges (classes)]
	write_table ("unicode-grapheme-break.inc", "Grapheme_Cluster_Break and Extended_Pictographic ranges, see GraphemeBreakClass (%s)" % source, items, 4)

	# East_Asian_Width Wide and Fullwidth
	wide = {c for c, value in data.east_asian_width.items () if value in ("W", "F")}
	write_table ("unicode-east-asian-wide.inc", "East_Asian_Width Wide and Fullwidth ranges (%s)" % source, format_range_table (wide), 4)

#################################################################################################
# Codepages
#################################################################################################

def decode_byte (codec, value):
	try:
		text = bytes ([value]).decode (codec)
		return ord (text) if len (text) == 1 else None
	except UnicodeDecodeError:
		return None

def generate_single_byte_codepage (codec, prefix, description):
	decode = {}
	for value in range (0x80, 0x100):
		c = decode_byte (codec, value)
		if c is not None and c != value:
			decode[value] = c
	encode = {}
	for value in range (0x80, 0x100):
		c = decode_byte (codec, value)
		if c is not None and c >= 0x80:
			encode[c] = value
	for c in range (0x80, 0x100): # identical Latin-1 characters
		if c in encode and encode[c] == c:
			del encode[c]
	write_table ("%s-decode.inc" % prefix, "%s to Unicode (characters differing from ISO Latin 1)" % description, format_mapping_table (decode), 4)
	write_table ("%s-encode.inc" % prefix, "Unicode to %s (characters differing from ISO Latin 1)" % description, format_mapping_table (encode), 4)

CP932_TRAIL_FIRST = 0x40 # must match kShiftJISTrailFirst in codepageshiftjis.h
CP932_TRAIL_COUNT = 0xFD - CP932_TRAIL_FIRST

def generate_cp932 ():
	description = "Shift-JIS (Windows codepage 932)"

	# Single bytes 0x80..0xFF, 0 means lead byte or undefined
	single = []
	lead_bytes = []
	for value in range (0x80, 0x100):
		c = decode_byte ("cp932", value)
		single.append (c or 0)
		if c is None:
			lead_bytes.append (value)

	# Double bytes, one row of trail bytes per lead byte, 0 means undefined
	rows = []
	used_lead_bytes = []
	for lead in lead_bytes:
		row = []
		for trail in range (CP932_TRAIL_FIRST, CP932_TRAIL_FIRST + CP932_TRAIL_COUNT):
			try:
				text = bytes ([lead, trail]).decode ("cp932")
				row.append (ord (text) if len (text) == 1 else 0)
			except UnicodeDecodeError:
				row.append (0)
		if any (row):
			used_lead_bytes.append (lead)
			rows.append (row)

	# Preferred encoding of each BMP character
	encode = {}
	for c in range (0x80, 0x10000):
		if 0xD800 <= c <= 0xDFFF:
			continue
		try:
			encoded = chr (c).encode ("cp932")
		except UnicodeEncodeError:
			continue
		encode[c] = encoded[0] if len (encoded) == 1 else (encoded[0] << 8) | encoded[1]

	# Like Windows and iconv, prefer IBM extensions (0xFA..0xFC) over NEC-selected IBM extensions (0xED, 0xEE)
	for lead, row in zip (used_lead_bytes, rows):
		if lead >= 0xFA:
			for index, c in enumerate (row):
				if c and (encode.get (c, 0) >> 8) in (0xED, 0xEE):
					encode[c] = (lead << 8) | (CP932_TRAIL_FIRST + index)

	# Irreversible mappings, as in iconv
	encode.update ({0x00A5: 0x5C, 0x203E: 0x7E, 0x2014: 0x815C})

	write_table ("cp932-decode-single.inc", "%s single bytes 0x80..0xFF to Unicode, 0 for lead bytes" % description, ["0x%04X" % c for c in single], 8)
	write_table ("cp932-lead-bytes.inc", "%s lead bytes, index into double byte rows" % description, ["0x%02X" % b for b in used_lead_bytes], 16)
	write_table ("cp932-decode-double.inc", "%s double bytes to Unicode, rows of %d trail bytes starting at 0x%02X" % (description, CP932_TRAIL_COUNT, CP932_TRAIL_FIRST),
				 ["0x%04X" % c for row in rows for c in row], 12)
	write_table ("cp932-encode.inc", "Unicode to %s" % description, format_mapping_table (encode), 4)

#################################################################################################
# Main
#################################################################################################

def download_ucd (folder):
	for name in UCD_FILES:
		url = UCD_URL.format (version = UNICODE_VERSION, file = name)
		print ("Downloading %s" % url)
		urllib.request.urlretrieve (url, os.path.join (folder, os.path.basename (name)))

def main ():
	parser = argparse.ArgumentParser (description = __doc__, formatter_class = argparse.RawDescriptionHelpFormatter)
	parser.add_argument ("--ucd", help = "folder containing the UCD %s files (without subfolders)" % UNICODE_VERSION)
	args = parser.parse_args ()

	if args.ucd:
		data = UnicodeData (args.ucd)
	else:
		with tempfile.TemporaryDirectory () as folder:
			download_ucd (folder)
			data = UnicodeData (folder)

	generate_properties (data)
	generate_case_mappings (data)
	generate_normalization (data)
	generate_segmentation (data)
	generate_single_byte_codepage ("cp1252", "cp1252", "Windows Latin 1 (codepage 1252)")
	generate_single_byte_codepage ("mac_roman", "macroman", "Mac Roman")
	generate_cp932 ()

if __name__ == "__main__":
	main ()
