#!/usr/local/bin/python

# dump-sbcs.py (c) 2008-2010 exeal
#
# Converts UTR #22 based XML file into 16x16 native-to-UCS mapping array and
# prints the result to the standard output.
# Do not use with non-SBCS encoding file.

import sys
import xml.sax
import xml.sax.handler

if len(sys.argv) != 2:
	exit('usage: python dump-dbcs.py <input-file-name>')

table = {}
substitution = 0x1a
class MyContentHandler(xml.sax.ContentHandler):
	def startElement(self, name, attributes):
		if name == 'a':
			table[int(attributes.getValue('b'), 16)] = int(attributes.getValue('u'), 16)
		elif name == 'characterMapping':
			print('# Reading', attributes.getValue('id'), '...')
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
		table[c] = 0xfffd
	sys.stdout.write('0x%04x,' % table[c])
	if c != table[c]:
		if c < 0x80:
			iso_646_compatible = False
		if c < 0x20:
			iso_646_c0_compatible = False
		if c > 0x80 and c < 0xa0:
			iso_646_c1_compatible = False
	if table[c] > 0xffff:
		has_surrogates = True
	if table[c] == 0x20ac:
		has_euro_sign = True
	if ((c + 1) % 8) == 0:
		print('')
	else:
		sys.stdout.write(' ')

yes_no = ['no', 'yes']
print('# Substitution byte : 0x%x' % substitution)
print('# Has surrogates [\u10000-\u10FFFF]? :', yes_no[has_surrogates])
print('# ISO 646 compatible [0x00-0x7F]? :', yes_no[iso_646_compatible])
print('# ISO 646 C0 compatible [0x00-0x1F]? :', yes_no[iso_646_c0_compatible])
print('# ISO 646 C1 compatible [0x80-0x9F]? :', yes_no[iso_646_c1_compatible])

ibm_pc_7bit = [
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
	0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
	0x0018, 0x0019, 0x001c, 0x001b, 0x007f, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
	0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
	0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
	0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
	0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
	0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
	0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x001a
]
ibm_pc_compatible = not iso_646_compatible
c = 0
while c < 0x80 and ibm_pc_compatible:
	if table[c] != ibm_pc_7bit[c]:
		ibm_pc_compatible = False
	c = c + 1
print('# IBM PC compatible [0x00-0x7F]? :', yes_no[ibm_pc_compatible])
print('# Has Euro sign [\u20AC]? :', yes_no[has_euro_sign])
