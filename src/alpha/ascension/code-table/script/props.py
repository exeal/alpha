#!/usr/local/bin/python

# props.py (c) 2008 exeal
# (was props.pl (c) 2005-2007 exeal)
#
# This script generates C++ source files
# - uprops-implementation
# - uprops-table
# - uprops-binary-property-table-definition
# for ascension.text.ucd.* Unicode property classes. This uses the folowing public files
# obtained from Unicode.org:
# - UnicodeData.txt
# - Blocks.txt
# - Scripts.txt
# - PropList.txt
# - CaseFolding.txt
# - EastAsianWidth.txt
# - LineBreak.txt

import sys
import time
import os.path
import re

category_map = {
	'Lu' : 1, 'Ll' : 2, 'Lt' : 3, 'Lm' : 4, 'Lo' : 5,
	'Mn' : 6, 'Mc' : 7, 'Me' : 8,
	'Nd' : 9, 'Nl' : 10, 'No' : 11,
	'Pc' : 12, 'Pd' : 13, 'Ps' : 14, 'Pe' : 15, 'Pi' : 16, 'Pf' : 17, 'Po' : 18,
	'Sm' : 19, 'Sc' : 20, 'Sk' : 21, 'So' : 22,
	'Zs' : 23, 'Zl' : 24, 'Zp' : 25,
	'Cc' : 26, 'Cf' : 27, 'Cs' : 28, 'Co' : 29, 'Cn' : 30
}

east_asian_width_map = {
	'F' : 318, 'H' : 319, 'W' : 320, 'Na' : 321, 'A' : 322, 'N' : 323
}

line_break_map = {
	# normative
	'BK' : 324, 'CR' : 325, 'LF' : 326, 'CM' : 327, 'SG' : 329, 'GL' : 332, 'CB' : 338, 'SP' : 333, 'ZW' : 331,
	'NL' : 328, 'WJ' : 330, 'JL' : 355, 'JV' : 356, 'JT' : 357, 'H2' : 352, 'H3' : 353,
	# informative
	'XX' : 359, 'OP' : 343, 'CL' : 339, 'QU' : 344, 'NS' : 342, 'EX' : 340, 'SY' : 349,
	'IS' : 345, 'PR' : 348, 'PO' : 347, 'NU' : 346, 'AL' : 351, 'ID' : 354, 'IN' : 341, 'HY' : 337,
	'BB' : 336, 'BA' : 335, 'SA' : 358, 'AI' : 350, 'B2' : 334
}


if(len(sys.argv) != 2):
	exit('usage: python props.py [input-file-directory]')
input_directory = sys.argv[1]
output_directory = '..'

header = '// automatically generated by props.py at $ ' + time.ctime() + ' $'

# converts a list of code points to a C++ wide string literal
def cp2str(cps):
	result = ''
	for cp in cps:
		cp = int(cp, 16)
		if cp < 0x010000:
			result += r'\x%X' % cp
		else:
			result += r'\x%X\x%X' % (((cp >> 10) & 0xFFFF) + 0xD7C0, (cp & 0x03FF) | 0xDC00)
	return 'L"' + result + '"'

# generate General_Category code
def process_general_categories():
	input = open(os.path.join(input_directory, 'UnicodeData.txt'))
	print 'generating General_Category table...'
	output_table.write('const PropertyRange GeneralCategory::ranges_[] = {\n')

	first, last = 0, -1
	continued_gc = 'Cc'
	pattern = re.compile(r'^([\dA-Fa-f]+)\;(.+?)\;(\w\w)')
	range_pattern = re.compile(r'^\<.+?First\>$')
	code_point_pattern = re.compile(r'^([\dA-Fa-f]+)')
	while True:
		line = input.readline()
		if line == '':
			break
		m = pattern.match(line)
		if m == None:
			continue
		cp, gc = int(m.group(1), 16), m.group(3)
		is_range = range_pattern.match(m.group(2)) != None

		if cp != last + 1 or gc != continued_gc:	# not continued
			output_table.write('{0x%X,0x%X,%d},' % (first, last, category_map[continued_gc]))
			first = cp
			continued_gc = gc
			if not is_range:
				last = first
			else:
				line = input.readline()
				m = code_point_pattern.match(line)
				last = int(m.group(1), 16)
		else:	# continued
			if not is_range:
				last += 1
			else:
				line = input.readline()
				m = code_point_pattern.match(line)
				last = int(m.group(1), 16)
	output_table.write('{0x%X,0x%X,%d}};\n' % (first, last, category_map[continued_gc]))
	output_table.write('const size_t GeneralCategory::count_ = MANAH_COUNTOF(GeneralCategory::ranges_);\n')
	input.close()

# generate Block code
def process_code_blocks():
	input = open(os.path.join(input_directory, 'Blocks.txt'))
	print 'generating Block table...'
	output_table.write('const PropertyRange CodeBlock::ranges_[] = {\n')

	pattern = re.compile(r'^([\dA-Fa-f]+)\.\.([\dA-Fa-f]+)')
	block_number = 40;	# CodeBlock::BASIC_LATIN
	for line in input:
		m = pattern.match(line)
		if m == None:
			continue
		output_table.write('{0x%X,0x%X,%d},' % (int(m.group(1), 16), int(m.group(2), 16), block_number))
		block_number += 1
	input.close()
	output_table.write('};\n')
	output_table.write('const size_t CodeBlock::count_ = MANAH_COUNTOF(CodeBlock::ranges_);\n')

# generate two arrays table for Canonical_Combining_Class
def process_canonical_combining_classes():
	input = open(os.path.join(input_directory, 'UnicodeData.txt'))
	print 'generating CanonicalCombining_Class table...'

	cp, ccc = [], [];
	pattern = re.compile(r'^([0-9A-F]+);[^;]+;[^;]+;(\d+)')
	for line in input:
		m = pattern.match(line)
		if m == None:
			continue
		klass = int(m.group(2))
		if klass != 0:
			cp.append(int(m.group(1), 16))
			ccc.append(klass)

	output_table.write('#ifndef ASCENSION_NO_UNICODE_NORMALIZATION\n')
	output_table.write('const Char CanonicalCombiningClass::SRC_UCS2[] = {')
	i, j = 0, 0
	while cp[i] < 0x10000:
		output_table.write('0x%X,' % cp[i])
		i += 1
	output_table.write('};\nconst uchar CanonicalCombiningClass::DEST_UCS2[] = {')
	number_of_ucs2 = i
	while j < number_of_ucs2:
		output_table.write('%d,' % ccc[j])
		j += 1
	output_table.write('};\nconst CodePoint CanonicalCombiningClass::SRC_UCS4[] = {')
	while i < len(cp):
		output_table.write('0x%X,' % cp[i])
		i += 1
	output_table.write('};\nconst uchar CanonicalCombiningClass::DEST_UCS4[] = {')
	while j < len(ccc):
		output_table.write('%d,' % ccc[j])
		j += 1
	output_table.write('};\nconst size_t CanonicalCombiningClass::UCS2_COUNT = %d;\n' % number_of_ucs2)
	output_table.write('const size_t CanonicalCombiningClass::UCS4_COUNT = %d;\n' % (len(ccc) - number_of_ucs2))
	output_table.write('#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */\n')
	input.close()

# generate Script code
def process_scripts():
	input = open(os.path.join(input_directory, 'Scripts.txt'))
	print 'generating Script table...'
	print >> output_table, 'const PropertyRange Script::ranges_[] = {'

	singleton_pattern = re.compile(r'^([\dA-Fa-f]+)\s+\;')
	range_pattern = re.compile(r'^([\dA-Fa-f]+)\.\.([\dA-Fa-f]+)\s+\;')
	end_pattern = re.compile(r'^\# Total')
	ranges = []
	script_number = 195	# Script::COMMON
	first, last = 0, -1
	for line in input:
		m = singleton_pattern.match(line)
		if m != None:	# singleton
			cp = int(m.group(1), 16)
			if cp == last + 1:
				last = last + 1
			else:
				ranges.append({'first' : first, 'last' : last, 'script' : script_number})
				first = last = cp
		else:
			m = range_pattern.match(line)
			if m != None:	# range
				begin, end = int(m.group(1), 16), int(m.group(2), 16)
				if begin == last + 1:
					last = end
				else:
					ranges.append({'first' : first, 'last' : last, 'script' : script_number})
					first, last = begin, end
			else:
				m = end_pattern.match(line)
				if m != None:	# end of section
					ranges.append({'first' : first, 'last' : last, 'script' : script_number})
					script_number = script_number + 1
	for v in sorted(ranges, lambda x, y: cmp(x['first'], y['first'])):
		output_table.write('{0x%X,0x%X,%d},' % (v['first'], v['last'], v['script']))
	print >> output_table, '};'
	print >> output_table, 'const size_t Script::count_ = MANAH_COUNTOF(Script::ranges_);'
	input.close()

# generate binary properties code
def process_binary_properties():
	input = open(os.path.join(input_directory, 'PropList.txt'))
	print 'generating binary properties table...'

	whole_pattern = re.compile(r'^[\da-fA-F\.\s]+\;\s+(\w+)')
	singleton_pattern = re.compile(r'^([\dA-Fa-f]+)\s+')
	range_pattern = re.compile(r'^([\dA-Fa-f]+)\.\.([\dA-Fa-f]+)\s+')
	end_pattern = re.compile(r'^\# Total')
	ranges = []
	read_points, continued_points, ucs4 = 0, 0, False
	property_name = ''
	for line in input:
		if read_points == 0:
			m = whole_pattern.match(line)
			if m != None:
				property_name = m.group(1)
		m = singleton_pattern.match(line)
		if m != None:	# singleton
			cp = int(m.group(1), 16)
			ranges.extend([cp, cp])
			read_points += 1
			if cp > 0xFFFF:
				ucs4 = True
		else:
			m = range_pattern.match(line)
			if m != None:	# range
				first, last = int(m.group(1), 16), int(m.group(2), 16)
				ranges.extend([first, last])
				read_points += last - first + 1
				continued_points += last - first + 1
				if last > 0xFFFF:
					ucs4 = True
			else:
				m = end_pattern.match(line)
				if m != None:	# end of section
					if continued_points * 3 / 2 < read_points:	# simple array
						if ucs4:
							bp_table_definition.write('static const CodePoint ')
						else:
							bp_table_definition.write('static const Char ')
						bp_table_definition.write('tableOfBp__' + property_name + '_[];\n')
						if ucs4:
							output_table.write('const CodePoint ')
						else:
							output_table.write('const Char ')
						output_table.write('BinaryProperty::tableOfBp__' + property_name + '_[] = {')
						i = 0
						while i < len(ranges):
							output_table.write('0x%04X,' % ranges[i])
							i += 2
						output_impl.write('template<> inline bool BinaryProperty::is<BinaryProperty::' + property_name.upper() + '>(CodePoint cp) {')
						if ucs4:
							output_impl.write('return std::binary_search(tableOfBp__' + property_name
								+ '_,tableOfBp__' + property_name + '_+' + str(len(ranges) / 2) + ',cp);}\n')
						else:
							output_impl.write('return (cp > 0xFFFF) ? false : std::binary_search(tableOfBp__' + property_name
								+ '_,tableOfBp__' + property_name + '_+' + str(len(ranges) / 2) + ',static_cast<Char>(cp));}\n')
					else:	# range based array
						if ucs4:
							bp_table_definition.write('static const internal::CodeRange<CodePoint> ')
						else:
							bp_table_definition.write('static const internal::CodeRange<Char> ')
						bp_table_definition.write('tableOfBp__' + property_name + '_[];\n')
						if ucs4:
							output_table.write('const CodeRange<CodePoint> ')
						else:
							output_table.write('const CodeRange<Char> ')
						output_table.write('BinaryProperty::tableOfBp__' + property_name + '_[] = {')
						i = 0
						while i < len(ranges):
							output_table.write('{0x%04X,0x%04X},' % (ranges[i], ranges[i + 1]))
							i += 2
						output_impl.write('template<> inline bool BinaryProperty::is<BinaryProperty::' + property_name.upper() + '>(CodePoint cp) {')
						output_impl.write('return findInRange(tableOfBp__%s_,tableOfBp__%s_+%d,cp)!=0;}\n' % (property_name, property_name, len(ranges)))
					print >> output_table, '};'
					ranges = []
					read_points = continued_points = 0
					ucs4 = False
	input.close()

# generate case-folding code
def process_case_folding():
	input = open(os.path.join(input_directory, 'CaseFolding.txt'))
	print 'generating case-folding table...'

	common_cased, common_folded, simple_cased, simple_folded, full_cased, full_folded = [], [], [], [], [], []
	pattern = re.compile(r'^([\dA-F]{4});\s+([CSF]);\s+([^;]+)')
	for line in input:
		m = pattern.match(line)
		if m == None:
			continue
		if m.group(2) == 'C':
			common_cased.append(m.group(1))
			common_folded.append(m.group(3))
		elif m.group(2) == 'S':
			simple_cased.append(m.group(1))
			simple_folded.append(m.group(3))
		else:
			full_cased.append(m.group(1))
			full_folded.append(r'L"\x' + m.group(3).replace(' ', r'\x') + '"')
	input.close()

	output_table.write(
		'const Char CaseFolder::COMMON_CASED[] = {\n0x' + ',0x'.join(common_cased)
		+ '};\nconst Char CaseFolder::COMMON_FOLDED[] = {\n0x' + ',0x'.join(common_folded)
		+ '};\nconst Char CaseFolder::SIMPLE_CASED[] = {\n0x' + ',0x'.join(simple_cased)
		+ '};\nconst Char CaseFolder::SIMPLE_FOLDED[] = {\n0x' + ',0x'.join(simple_folded)
		+ '};\nconst Char CaseFolder::FULL_CASED[] = {\n0x' + ',0x'.join(full_cased)
		+ '};\nconst Char* CaseFolder::FULL_FOLDED[] = {' + ','.join(full_folded)
		+ '};\nconst size_t CaseFolder::NUMBER_OF_COMMON_CASED = ' + str(len(common_cased))
		+ ';\nconst size_t CaseFolder::NUMBER_OF_SIMPLE_CASED = ' + str(len(simple_cased))
		+ ';\nconst size_t CaseFolder::NUMBER_OF_FULL_CASED = ' + str(len(full_cased)) + ';\n')

# generate decomposition mapping tables
def process_decomposition_mappings():
	input = open(os.path.join(input_directory, 'UnicodeData.txt'))
	output = open(os.path.join(output_directory, 'uprops-decomposition-mapping-table'), 'w')
	print 'generating decomposition mapping table...'
	print >> output, header

	canonical_src, canonical_dest, compatibility_src, compatibility_dest = [], [], [], []
	pattern = re.compile(r'^([\dA-F]{4,6});[^;]+;[^;]+;[^;]+;[^;]+;(\<\w+\> )?([^;]+);')
	for line in input:
		m = pattern.match(line)
		if m == None:
			continue
		if m.group(2) == None:
			canonical_src.append(int(m.group(1), 16))
			canonical_dest.append(cp2str(m.group(3).split(' ')))
		else:
			compatibility_src.append(int(m.group(1), 16))
			compatibility_dest.append(cp2str(m.group(3).split(' ')))

	output.write('const CodePoint CANONICAL_MAPPING_SRC[] = {')
	for c in canonical_src:
		output.write('0x%X,' % c)
	output.write('};\nconst Char* CANONICAL_MAPPING_DEST[] = {')
	for c in canonical_dest:
		output.write(c + ',')
	output.write('};\nconst size_t NUMBER_OF_CANONICAL_MAPPINGS = ' + str(len(canonical_src))
		+ ';\n#ifndef ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING\nconst CodePoint COMPATIBILITY_MAPPING_SRC[] = {')
	for c in compatibility_src:
		output.write('0x%X,' % c)
	output.write('};\nconst Char* COMPATIBILITY_MAPPING_DEST[] = {')
	for c in compatibility_dest:
		output.write(c + ',')
	output.write('};\nconst size_t NUMBER_OF_COMPATIBILITY_MAPPINGS = ' + str(len(compatibility_src)) + ';\n#endif\n')

	input.close();
	output.close()

# generate East_Asian_Width code
def process_east_asian_widths():
	input = open(os.path.join(input_directory, 'EastAsianWidth.txt'))
	first, last, ea = 0, -1, 'ZZ'

	print 'generating East_Asian_Width table...'
	print >> output_table, 'const PropertyRange EastAsianWidth::ranges_[] = {'
	singleton_pattern = re.compile(r'^([\dA-F]{4,6});([A-Z]{1,2})')
	range_pattern = re.compile(r'^([\dA-F]{4,6})\.\.([\dA-F]{4,6});([A-Z]{2})')
	for line in input:
		is_range = False
		m = singleton_pattern.match(line)
		if m == None:
			is_range = True
			m = range_pattern.match(line)
		if m != None:
			cp, prop = int(m.group(1), 16), m.group([2, 3][is_range])
			if prop != ea or cp != last + 1:
				if last != -1:
					output_table.write('{0x%X,0x%X,%d},' % (first, last, east_asian_width_map[ea]))
				first, ea = cp, prop
			if is_range:
				last = int(m.group(2), 16)
			else:
				last = cp

	output_table.write('{0x%X,0x%X,%d}};' % (first, last, east_asian_width_map[ea]))
	output_table.write('\nconst size_t EastAsianWidth::count_ = MANAH_COUNTOF(EastAsianWidth::ranges_);\n')
	input.close()

# generate Line_Break code
def process_line_breaks():
	input = open(os.path.join(input_directory, 'LineBreak.txt'))
	first, last, lb = 0, -1, 'ZZ'

	print 'generating Line_Break table...'
	print >> output_table, 'const PropertyRange LineBreak::ranges_[] = {'
	singleton_pattern = re.compile(r'^([\dA-F]{4,6});([A-Z]{2})')
	range_pattern = re.compile(r'^([\dA-F]{4,6})\.\.([\dA-F]{4,6});([A-Z]{2})')
	for line in input:
		is_range = False
		m = singleton_pattern.match(line)
		if m == None:
			is_range = True
			m = range_pattern.match(line)
		if m != None:
			cp, prop = int(m.group(1), 16), m.group([2, 3][is_range])
			if prop != lb or cp != last + 1:
				if last != -1:
					output_table.write('{0x%X,0x%X,%d},' % (first, last, line_break_map[lb]))
				first, lb = cp, prop
			if is_range:
				last = int(m.group(2), 16)
			else:
				last = cp

	output_table.write('{0x%X,0x%X,%d}};' % (first, last, line_break_map[lb]))
	output_table.write('\nconst size_t LineBreak::count_ = MANAH_COUNTOF(LineBreak::ranges_);\n')
	input.close()


# open output files
output_table = open(os.path.join(output_directory, 'uprops-table'), 'w')
print >> output_table, header
bp_table_definition = open(os.path.join(output_directory, 'uprops-binary-property-table-definition'), 'w')
print >> bp_table_definition, header
output_impl = open(os.path.join(output_directory, 'uprops-implementation'), 'w')
print >> output_impl, header

# version diagnostics
print >> output_table, '#if ASCENSION_UNICODE_VERSION != 0x0500'
print >> output_table, '#error These codes are based on old version of Unicode.'
print >> output_table, '#endif'

# process all!
process_general_categories()
process_code_blocks()
process_canonical_combining_classes()
process_scripts()
process_binary_properties()
process_case_folding()
process_decomposition_mappings()
process_east_asian_widths()
process_line_breaks()
print 'done.'

output_table.close()
bp_table_definition.close()
output_impl.close()
