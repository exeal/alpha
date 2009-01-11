#!/usr/local/bin/python

# gen-ivs-otft.py (c) 2008 exeal

import sys
import re

if len(sys.argv) != 2:
	exit('usage: python gen-ivs-otft.py <IVS_OTFT.txt>')

input = open(sys.argv[1])
output = open('ivs-otft.ipp', 'w')
pattern = re.compile(r"^([0-9A-Fa-f]{4,6})\s([0-9A-Fa-f]{4,6})\s\d+\s'(.{4})'")
try:
	for line in input:
		m = pattern.search(line)
		if m == None:
			continue
		base = m.group(1)
		vs = int(m.group(2), 16)
		if vs < 0xE0100 or vs > 0xE01EF:
			input.close()
			output.close()
			exit('error: invalid code point for variation selector %s' % m.group(2))
		tag = m.group(3)
		output.write('\t{(0x%s << 8) | %d, %s_TAG},\n' % (base, vs - 0xE0100, tag.upper()))
except:
	input.close()
	output.close()
	exit('error: I/O error occured during reading the input file.')
input.close()
output.close()
