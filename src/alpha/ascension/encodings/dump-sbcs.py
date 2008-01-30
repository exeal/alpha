#!/usr/local/bin/python

# dump-sbcs.py (c) 2008 exeal
# Converts UTR #22 based XML file into 16x16 native-to-UCS mapping array.
# Do not use with non-SBCS encoding file.

import sys
import xml.sax
import xml.sax.handler

if len(sys.argv) != 2:
	exit('usage: python dump-dbcs.py <input-file-name>')

table = {}
substitution = 0x1A
class MyContentHandler(xml.sax.ContentHandler):
	def startElement(self, name, attributes):
		if name == 'a':
			table[int(attributes.getValue('b'), 16)] = int(attributes.getValue('u'), 16)
		elif name == 'characterMapping':
			print '# Reading', attributes.getValue('id'), '...'
		elif name == 'assignments':
			substitution = '0x' + attributes.getValue('sub')

sax_reader = xml.sax.make_parser()
sax_reader.setFeature(xml.sax.handler.feature_validation, False)
sax_reader.setFeature(xml.sax.handler.feature_external_ges, False)
sax_reader.setContentHandler(MyContentHandler())
sax_reader.parse(sys.argv[1])

# dump mapping table
has_surrogates = False
iso_646_compatible = True
iso_646_c0_compatible, iso_646_c1_compatible = True, True
has_euro_sign = False;
for c in range(0x00, 0x100):
	if not c in table:
		table[c] = 0xFFFD
	sys.stdout.write('0x%04X,' % table[c])
	if c != table[c]:
		if c < 0x80:
			iso_646_compatible = False
		if c < 0x20:
			iso_646_c0_compatible = False
		if c > 0x80 and c < 0xA0:
			iso_646_c1_compatible = False
	if table[c] > 0xFFFF:
		has_surrogates = True
	if table[c] == 0x20AC:
		has_euro_sign = True
	if ((c + 1) % 8) == 0:
		print ''
	else:
		sys.stdout.write(' ')

yes_no = ['no', 'yes']
print '# Substitution byte : 0x%X' % substitution
print '# Has surrogates [\u10000-\u10FFFF]? :', yes_no[has_surrogates]
print '# ISO 646 compatible [0x00-0x7F]? :', yes_no[iso_646_compatible]
print '# ISO 646 C0 compatible [0x00-0x1F]? :', yes_no[iso_646_c0_compatible]
print '# ISO 646 C1 compatible [0x80-0x9F]? :', yes_no[iso_646_c1_compatible]

ibm_pc_7bit = [
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
	0x0018, 0x0019, 0x001C, 0x001B, 0x007F, 0x001D, 0x001E, 0x001F,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x001A
]
ibm_pc_compatible = not iso_646_compatible
c = 0
while c < 0x80 and ibm_pc_compatible:
	if table[c] != ibm_pc_7bit[c]:
		ibm_pc_compatible = False
	c = c + 1
print '# IBM PC compatible [0x00-0x7F]? :', yes_no[ibm_pc_compatible]
print '# Has Euro sign [\u20AC]? :', yes_no[has_euro_sign]
