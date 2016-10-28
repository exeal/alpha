# check-ascii.py
# (c) 2016 exeal

import sys

if len(sys.argv) != 2:
    print(r'usage: check-ascii <filename>')
    exit(1)

filename = sys.argv[1]
input = open(filename, 'r+b')
line_number = 1
for line in input:
    offset = 1
    for c in line:
        if c >= 0x80:
            print('{0}({1},{2}): Found non-ASCII character : 0x{3:X}'.format(filename, line_number, offset, c))
        offset += 1
    line_number += 1
