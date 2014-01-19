#!/usr/local/bin/python
#
# gen-uprops.py (c) 2009-2011, 2014 exeal
# [was] props.py (c) 2008 exeal
# [was] props.pl (c) 2005-2007 exeal
#
# Generates C++ source files implement Unicode property.
#
#   usage: python gen-uprops.py <ucd.nounihan.grouped.xml> <ucd-files-directory>
#
# The input file ucd.nounihan.grouped.xml can be obtained from Unicode.org web
# site (http://www.unicode.org/Public/<unicode-version>/ucdxml/).
#
# The parameter 'ucd-files-directory' is the directory contains the input files
# include:
#
# - CaseFolding.txt
# - PropertyAliases.txt
# - PropertyValueAliases.txt
#
# These files also can be found at Unicode.org web site
# (http://www.unicode.org/Public/<unicode-version>/ucd/).
#
# This code is for Unicode version 5.1.0.
#
# For the detailed description about Ascension's Unicode property
# implementation, see the document unicode-property.pdf.

import os.path
import re
import sys
import time
import xml.sax

if len(sys.argv) != 3:
    exit('usage: python gen-uprops.py <ucd.nounihan.grouped.xml> <ucd-files-directory>')


class PropertyNames(object):
    def __init__(self, input_directory):
        print('Building property name table...')
        self._properties = {}
        comment = re.compile(r'#.*$')
        delimiters = re.compile(r'[\s;]+')
        # collect names
        f = open(os.path.join(input_directory, 'PropertyAliases.txt'))
        print('...Loaded %s.' % os.path.realpath(f.name))
        for line in f:
            s = comment.sub('', line)
            if len(s) > 0:
                names = delimiters.split(s)  # [0] short, [1] long, [2:] others
                names = tuple(filter(lambda x : x != '', names))
                if len(names) > 1:
                    self._properties[names[0]] = {'aliases' : [names[1]], 'values' : []}
                    if len(names) > 2:
                        self._properties[names[0]]['aliases'].extend(names[2:])
        f.close()
        # collect value names
        f = open(os.path.join(input_directory, 'PropertyValueAliases.txt'))
        print('...Loaded %s.' % os.path.realpath(f.name))
        for line in f:
            s = comment.sub('', line)
            if len(s) > 0:
                names = delimiters.split(s)  # [0] property short, [1] short, [2] long, [3:] others
                if names[0] == 'ccc':  # ouch, order of ccc names is different...
                    names.append(names[1])
                    del names[1]
                names = list(filter(lambda x : x != '', names))
                if len(names) > 1:
                    if names[1] == 'n/a':
                        names[1] = names[2]
                    self._properties[names[0]]['values'].append(names[1:])
        f.close()
        print('...Done.')
    def binary_value_names(self):
        names = []
        for (short_name, p) in iter(self._properties.items()):
            v = p['values']
            if len(v) == 2 and len(v[0]) == 4 and len(v[1]) == 4 and sorted((v[0][0], v[1][0])) == ['N', 'Y']:
                # p is a binary property
                if short_name == 'Gr_Link':  # Grapheme_Link is deprecated and not implemented
                    continue
                cpp_name = PropertyNames.cpp_value_name(p['aliases'][0])
                names.append((PropertyNames.fold_name(short_name), cpp_name))
                for alias in p['aliases']:
                    names.append((PropertyNames.fold_name(alias), cpp_name))
        return PropertyNames._unique_value_names(names)
    @staticmethod
    def compare_names(name1, name2):
        temp = (PropertyNames.cpp_name(name1), PropertyNames.cpp_name(name2))
        if temp[0] > temp[1]:
            return 1
        elif temp[0] == temp[1]:
            return 0
        else:
            return -1
    @staticmethod
    def cpp_name(name):
        return r'ucd::' + re.sub(r'[^A-Za-z0-9]', '', name)
    @staticmethod
    def cpp_value_name(name):
        return re.sub(r'[^A-Za-z0-9]', '_', name.upper())
    @staticmethod
    def fold_name(name):
        return re.sub(r'[_\-\s]', '', name.lower())
    def long_name(self, alias):
        self._property(alias)['aliases'][0]
        for short_name, x in enumerate(self._properties):
            aliases = x['aliases']
            if PropertyNames.compare_names(short_name, alias) == 0:
                return aliases[0]
            if len(aliases) > 1:
                for a in aliases[1:]:
                    if PropertyNames.compare_names(a, alias) == 0:
                        return aliases[0]
        raise ValueError
    def long_value_name(self, short_or_long_name, short_value_name):
        vns = self._property(short_or_long_name)['values']
        for aliases in vns:
            if short_value_name in aliases:
                return aliases[1]
        raise ValueError('\'%s\' for \'%s\' is unknown (%s are exist).' % (short_value_name, short_or_long_name, vns))
    def short_name(self, long_name):
        for i in iter(self._properties.items()):
            if i[1]['aliases'][0] == long_name:
                return i[0]
        raise ValueError
    def value_names(self, long_name):
        vns = self._property(long_name)['values']
        names = []
        for aliases in vns:
            for alias in aliases:
                if len(alias) == 1 and long_name == 'General_Category':  # see the documentation for this workaround
                    alias += '&'
                names.append((PropertyNames.fold_name(alias), PropertyNames.cpp_value_name(aliases[1])))
        return PropertyNames._unique_value_names(names)
    def _property(self, short_or_long_name):
        if short_or_long_name in self._properties:
            return self._properties[short_or_long_name]
        for i in iter(self._properties.items()):
            if i[1]['aliases'][0] == short_or_long_name:
                return i[1]
        raise ValueError
    @staticmethod
    def _unique_value_names(names):
        names.sort()
        i, c = 0, len(names)
        while i + 1 < c:
            if PropertyNames.compare_names(names[i][0], names[i + 1][0]) == 0:
                del names[i + 1]
                c -= 1
            else:
                i += 1
        return names


class CodeGenerator(object):
    def __init__(self, ucdxml_filename, ucd_input_directory, output_directory):
        self._ucdxml_filename = ucdxml_filename
        self._ucd_input_directory = ucd_input_directory
        self._output_directory = output_directory
        self._output_files = {}
        self._PROPERTY_NAMES = PropertyNames(self._ucd_input_directory)

    def main(self):
        self._open_output_file('binary-property-values-definition')
        self._open_output_file('code-table')
        self._open_output_file('inlines')
        self._open_output_file('value-names')
        self._output_data_types_definition()
        self._process_cases()
        self._process_decomposition_mappings()
        self._process_partitioned_property('General_Category')
        self._process_partitioned_property('Script')
        self._process_partitioned_property('Line_Break')
        self._process_partitioned_property('East_Asian_Width')
        self._process_blocks()
        self._process_canonical_combining_classes()
        self._process_binary_properties()
        for file in iter(self._output_files.items()):
            file[1].close()

    def _output_data_types_definition(self):
        out = self._open_output_file('data-types')
        out.write('\t}\n}\nnamespace detail {\n')
        out.write('\tstruct CharacterPropertyPartition {\n'
                  + '\t\tCodePoint beginning;\n'
                  + '\t\tint property;\n'
                  + '#if defined(ASCENSION_COMPILER_MSVC) && defined(_SECURE_SCL)\n'
                  + '\t\tbool operator<(const CharacterPropertyPartition& other) const {return beginning < other.beginning;}\n'
                  + '#endif\n'
                  + '\t};\n')
        out.write('\ttemplate<typename CharT> struct CharacterPropertyRange {\n'
                  + '\t\tCharT beginning, end;\n'
                  + '\t\tint property;\n'
                  + '#if defined(ASCENSION_COMPILER_MSVC) && defined(_SECURE_SCL)\n'
                  + '\t\tbool operator<(const CharacterPropertyRange& other) const {return beginning < other.beginning;}\n'
                  + '#endif\n'
                  + '\t};\n')
        out.write('\ttemplate<typename T, typename CharT> struct CharacterPropertyRangeComparer {\n'
                  + '\t\tbool operator()(const T& lhs, CharT rhs) const {return lhs.beginning < rhs;}\n'
                  + '\t\tbool operator()(CharT lhs, const T& rhs) const {return lhs < rhs.beginning;}\n'
                  + '#if defined(ASCENSION_COMPILER_MSVC) && defined(_SECURE_SCL)\n'
                  + '\t\tbool operator()(const T& lhs, const T& rhs) const {return lhs.beginning < rhs.beginning;}\n'
                  + '#endif\n'
                  + '\t};\n')
        out.write('\tstruct CharacterPropertyValueName {\n'
                  + '\t\tconst char* const name;\n'
                  + '\t\tconst int value;\n'
                  + '\t};\n')
        out.write('\tstruct CharacterPropertyValueNameComparer {\n'
                  + '\t\ttemplate<typename CharType>\n'
                  + '\t\tbool operator()(const CharacterPropertyValueName& lhs, const CharType* rhs) const {return text::ucd::PropertyNameComparer::compare(lhs.name, rhs) < 0;}\n'
                  + '\t\ttemplate<typename CharType>\n'
                  + '\t\tbool operator()(const CharType* lhs, const CharacterPropertyValueName& rhs) const {return text::ucd::PropertyNameComparer::compare(lhs, rhs.name) < 0;}\n'
                  + '#if defined(ASCENSION_COMPILER_MSVC) && defined(_SECURE_SCL)\n'
                  + '\t\tbool operator()(const CharacterPropertyValueName& lhs, const CharacterPropertyValueName& rhs) const {return text::ucd::PropertyNameComparer::compare(lhs.name, rhs.name) < 0;}\n'
                  + '#endif\n'
                  + '\t};\n')
        out.write('}\nnamespace text {\n\tnamespace ucd {\n')

    def _open_output_file(self, name):
        acronym = ''.join([x[0] for x in re.split(r'-', name)])
        f = open(os.path.join(self._output_directory, 'uprops-' + name), 'w')
        self._output_files[acronym] = f
        CodeGenerator._print_timestamp(f)
        return f
    @staticmethod
    def _print_timestamp(out):
        out.write(
            r'// automatically generated by tools/gen-uprops.py at $ '
            + time.ctime() + ' $\n')

    @staticmethod
    def _foreach_in_property_partitions(partitions, function, removal_value):
        result = []
        i, n = 0, len(partitions)
        while i < n:
            if partitions[i][1] != removal_value:
                next = [partitions[i + 1][0], 0x110000][i + 1 == n]
                for c in range(partitions[i][0], next):
                    function(c, partitions[i][1])
            i += 1

    def _make_property_partitions(self, short_name):
        partitions = []  # an element is (first-code-point, last-code-point, property-value)
        class ContentHandler(xml.sax.ContentHandler):
            def __init__(self):
                self._current_group_value = ''
            def startElement(self, name, attributes):
                if name in ['char', 'noncharacter', 'reserved', 'surrogate']:
                    if 'cp' in attributes:
                        first = int(attributes.getValue('cp'), 16)
                        last = first + 1
                    else:
                        first = int(attributes.getValue('first-cp'), 16)
                        last = int(attributes.getValue('last-cp'), 16) + 1
                    if short_name in attributes:
                        value = attributes.getValue(short_name)
                    else:
                        value = self._current_group_value
                    if value == '':
                        raise RuntimeError('Property %s is not defined for character U+%04x.' % (short_name, current_range[0]))
                    partitions.append((first, last, value))
                elif name == 'group' and (short_name in attributes):
                    self._current_group_value = attributes.getValue(short_name)
            def endElement(self, name):
                if name == 'group':
                    self._current_group_value = ''
        xml.sax.parse(self._ucdxml_filename, ContentHandler())
        partitions.sort()
        # combine successive partitions
        i = 0
        while i + 1 < len(partitions):
            if partitions[i][2] == partitions[i + 1][2] and partitions[i][1] == partitions[i + 1][0]:
                partitions[i] = (partitions[i][0], partitions[i + 1][1], partitions[i][2])
                del partitions[i + 1]
            else:
                i += 1
        # diagnose if sparse
        i = 0
        while i + 1 < len(partitions):
            if partitions[i][1] != partitions[i + 1][0]:
                raise RuntimeError(r'The input repartoire is not successive at U+%X.' % partitions[i][1])
            i += 1
        if partitions[-1][1] != 0x110000:
            raise RuntimeError(r'The input repartoire is not successive at U+%X.' % partitions[-1][0])
        return [(p[0], p[2]) for p in partitions]

    def _print_forname_code(self, long_name, value_names):
        out = self._output_files['i']
        out.write(
            '/// Returns the property with the given name.\n'
            + 'template<typename CharType>\n'
            + ('inline int %s::forName(const CharType* name) {\n' % PropertyNames.cpp_name(long_name))
            + '\tconst detail::CharacterPropertyValueName* const p =\n'
            + ('\t\tstd::lower_bound(NAMES_, NAMES_ + %d, name, detail::CharacterPropertyValueNameComparer());\n') % len(value_names)
            + ('\treturn (p != NAMES_ + %d && PropertyNameComparer::compare(name, p->name)) ?\n') % len(value_names)
            + '\t\tp->value : NOT_PROPERTY;\n}\n')

    def _print_partitioned_of_code(self, long_name):
        out = self._output_files['i']
        out.write('/// Returns the property of the specified character @a c.\n')
        out.write(('inline int %s::of(CodePoint c) BOOST_NOEXCEPT {' % PropertyNames.cpp_name(long_name))
                  + 'if(!isValidCodePoint(c)) return DEFAULT_VALUE;'
                  + ' const detail::CharacterPropertyPartition* const p = std::upper_bound('
                  + 'VALUES_, VALUES_ + NUMBER_, c, detail::CharacterPropertyRangeComparer<detail::CharacterPropertyPartition, CodePoint>());'
                  + ' assert(p != VALUES_); return p[-1].property;}\n')

    def _print_value_names(self, long_name):
        names = self._PROPERTY_NAMES.value_names(long_name)
        cpp_name = PropertyNames.cpp_name(long_name)
        out = self._output_files['vn']
        out.write(r'const detail::CharacterPropertyValueName %s::NAMES_[] = {' % cpp_name)
        for name in names:
            out.write(r'{"%s",%s::%s},' % (name[0], cpp_name, name[1]))
        out.write('};\n')
        self._print_forname_code(long_name, names)

    def _process_binary_properties(self):
        list(map(self._process_binary_property, [
                # general
                'White_Space', 'Noncharacter_Code_Point', 'Deprecated', 'Logical_Order_Exception', 'Variation_Selector',
                # case
                'Soft_Dotted',
                # identifiers
                'Pattern_Syntax', 'Pattern_White_Space',
                # decomposition and normalization
                # (absence)
                # shaping and rendering
                'Join_Control',
                # bidi
                'Bidi_Control',
                # numeric
                'Hex_Digit', 'ASCII_Hex_Digit',
                # cjk
                'Ideographic', 'Unified_Ideograph', 'Radical', 'IDS_Binary_Operator', 'IDS_Trinary_Operator',
                # misc
                'Quotation_Mark', 'Dash', 'Hyphen', 'STerm', 'Terminal_Punctuation', 'Diacritic', 'Extender',
                # contributory properties
                'Other_Alphabetic', 'Other_Default_Ignorable_Code_Point', 'Other_Grapheme_Extend',
                'Other_ID_Start', 'Other_ID_Continue', 'Other_Lowercase', 'Other_Math', 'Other_Uppercase']))
        names = self._PROPERTY_NAMES.binary_value_names()
        out = self._output_files['vn']
        out.write(r'const detail::CharacterPropertyValueName ucd::BinaryProperty::NAMES_[] = {')
        for name in names:
            out.write(r'{"%s",ucd::BinaryProperty::%s},' % name)
        out.write('};\n')
        self._print_forname_code('BinaryProperty', names)

    def _process_binary_property(self, long_name):
        sys.stdout.write('Generating code for property \'%s\'...' % long_name)
        partitions = self._make_property_partitions(self._PROPERTY_NAMES.short_name(long_name))
        # check if at least one character out of BMP has the property
        ps = []
        ucs4 = False
        for i, p in enumerate(partitions):
            if p[1] != 'Y':
                continue
            if i + 1 < len(partitions):
                next = partitions[i + 1][0]
            else:
                0x110000
            ps.append((p[0], next - 1))
            if p[0] >= 0x10000 or next > 0x10000:
                ucs4 = True
        element_type = ['Char', 'CodePoint'][ucs4]
        cpp_name = PropertyNames.cpp_value_name(long_name)
        member_name = r'valuesOf_%s_' % cpp_name
        self._output_files['bpvd'].write(
            'static const detail::CharacterPropertyRange<%s> %s[];\n' % (element_type, member_name))
        self._output_files['ct'].write(
            r'const detail::CharacterPropertyRange<%s> ucd::BinaryProperty::%s[] = {' % (element_type, member_name))
        for p in ps:
            self._output_files['ct'].write(r'{0x%x,0x%x},' % p)
        self._output_files['ct'].write('};\n')
        self._output_files['i'].write('/// Returns true if the specified character @a c has the property.\n')
        self._output_files['i'].write('template<> inline bool ucd::BinaryProperty::is<ucd::BinaryProperty::%s>(CodePoint c) BOOST_NOEXCEPT {' % cpp_name)
        if ucs4:
            self._output_files['i'].write('if(!isValidCodePoint(c)) return false; ')
        else:
            self._output_files['i'].write('if(c > 0xffffu) return false; ')
        self._output_files['i'].write(
            'const detail::CharacterPropertyRange<%s>* const p = std::upper_bound(%s, %s + %d, '
            % (element_type, member_name, member_name, len(ps)))
        if ucs4:
            self._output_files['i'].write('c, ')
        else:
            self._output_files['i'].write('static_cast<Char>(c), ')
        self._output_files['i'].write(
            ('detail::CharacterPropertyRangeComparer<detail::CharacterPropertyRange<%s>, %s>()); ' % (element_type, element_type))
            + ('return p != %s && c <= p[-1].end;}\n' % member_name))
        sys.stdout.write(' ([%d])\n' % len(ps))

    def _process_canonical_combining_classes(self):
        sys.stdout.write('Generating code for property \'Canonical_Combining_Class\'...')
        ps = self._make_property_partitions('ccc')
        self._output_files['ct'].write('const CodePoint ucd::CanonicalCombiningClass::CHARACTERS_[] = {')
        self._foreach_in_property_partitions(ps, lambda c, p : self._output_files['ct'].write('0x%x,' % c), '0')
        self._output_files['ct'].write('};\n')
        self._output_files['ct'].write('const std::uint8_t ucd::CanonicalCombiningClass::VALUES_[] = {')
        self._foreach_in_property_partitions(ps, lambda c, p : self._output_files['ct'].write('%s,' % p), '0')  # p is a string
        self._output_files['ct'].write('};\n')
        self._output_files['ct'].write('const std::size_t ucd::CanonicalCombiningClass::NUMBER_ = std::extent<decltype(ucd::CanonicalCombiningClass::VALUES_)>::value;')
        self._print_value_names('Canonical_Combining_Class')
        self._output_files['i'].write(
            '/// Returns the property of the specified character @a c.\n'
            + 'inline int ucd::CanonicalCombiningClass::of(CodePoint c) BOOST_NOEXCEPT {\n'
            + '\tconst CodePoint* const p = std::lower_bound(CHARACTERS_, CHARACTERS_ + NUMBER_, c);\n'
            + '\treturn (p != CHARACTERS_ + NUMBER_ && *p == c) ? VALUES_[p - CHARACTERS_] : DEFAULT_VALUE;\n'
            + '};\n')
        sys.stdout.write(' ([%d])\n' % len(ps))

    def _process_cases(self):
        print('Generating case folding code...')
        filename = os.path.realpath(os.path.join(self._ucd_input_directory, r'CaseFolding.txt'))
        input = open(filename)
        print('...Loaded %s' % filename)
        common_map, simple_map, full_map, full_folded_offsets = [], [], [], ['0']
        pattern = re.compile(r'^([\dA-F]{4,6});\s+([CSF]);\s+([^;]+)')  # yes, almost all cased characters are in BMP, but...
        for line in input:
            m = pattern.match(line)
            if m is not None:
                if m.group(2) == 'C':
                    common_map.append((m.group(1).lower(), m.group(3).lower()))
                elif m.group(2) == 'S':
                    simple_map.append((m.group(1).lower(), m.group(3).lower()))
                else:
                    folded_sequence = m.group(3).split(' ')
                    full_folded_offsets.append(str(int(full_folded_offsets[-1]) + len(folded_sequence)))
                    full_map.append((m.group(1).lower(), '0x' + ',0x'.join(folded_sequence).lower()))
        input.close()

        self._output_files['ct'].write(
            ('const std::size_t CaseFolder::NUMBER_OF_COMMON_CASED_ = %d;\n' % len(common_map))
            + ('const std::size_t CaseFolder::NUMBER_OF_SIMPLE_CASED_ = %d;\n' % len(simple_map))
            + ('const std::size_t CaseFolder::NUMBER_OF_FULL_CASED_ = %d;\n' % len(full_map)))
        self._output_files['ct'].write(
            'const CodePoint CaseFolder::COMMON_CASED_[] = {0x' + ',0x'.join([m[0] for m in common_map])
            + '};\nconst CodePoint CaseFolder::COMMON_FOLDED_[] = {0x'+ ',0x'.join([m[1] for m in common_map])
            + '};\nconst CodePoint CaseFolder::SIMPLE_CASED_[] = {0x' + ',0x'.join([m[0] for m in simple_map])
            + '};\nconst CodePoint CaseFolder::SIMPLE_FOLDED_[] = {0x' + ',0x'.join([m[1] for m in simple_map])
            + '};\nconst CodePoint CaseFolder::FULL_CASED_[] = {0x' + ',0x'.join([m[0] for m in full_map])
            + '};\nconst Char CaseFolder::FULL_FOLDED_[] = {' + ','.join([m[1] for m in full_map])
            + '};\nconst std::ptrdiff_t CaseFolder::FULL_FOLDED_OFFSETS_[] = {' + ','.join(full_folded_offsets)
            + '};\n')
            
        print('...Processed common case mapping (%d).' % len(common_map))
        print('...Processed simple case mapping (%d).' % len(simple_map))
        print('...Processed full case mapping (%d).' % len(full_map))
        print('...Done')

    def _process_decomposition_mappings(self):
        canonical_mappings, compatibility_mappings = [], []

        def code_point_to_utf_16(cp):
            cp = int(cp, 16)
            if cp < 0x10000:
                return (cp,)
            else:
                return (((cp >> 10) & 0xffff) + 0xd7c0, (cp & 0x03ff) | 0xdc00)

        def code_sequence_to_utf_16(cs):
            result = []
            for c in cs.split():
                result.extend(code_point_to_utf_16(c))
            return result

        def utf_16_to_cpp_literal(a):
            return ','.join(('static_cast<ascension::Char>(0x%x)' % code_unit) for code_unit in a)

        class ContentHandler(xml.sax.ContentHandler):
            def __init__(self, out):
                self._current_dt = ''  # Decomposition_Type
                self._current_dm = ''  # Decomposition_Mapping
                self._reset_group()
            def startElement(self, name, attributes):
                if name == 'char':
                    if 'dm' in attributes:
                        dm = attributes.getValue('dm')
                    else:
                        dm = self._current_dm
                    if dm == '#':
                        return
                    if 'dt' in attributes:
                        dt = attributes.getValue('dt')
                    else:
                        dt = self._current_dt
                    if dt == 'none' or (('na' in attributes) and attributes.getValue('na').startswith('HANGUL SYLLABLE ')):
                        return
                    if 'cp' in attributes:
                        first_cp = last_cp = int(attributes.getValue('cp'), 16)
                    else:
                        first_cp, last_cp = int(attributes.getValue('first_cp'), 16), int(attributes.getValue('last_cp'), 16)
                    mappings = (canonical_mappings, compatibility_mappings)[dt != 'can']
                    for c in range(first_cp, last_cp + 1):
                        mappings.append((c, code_sequence_to_utf_16(dm)))
                elif name == 'group':
                    if 'dt' in attributes:
                        self._current_dt = attributes.getValue('dt')
                    if 'dm' in attributes:
                        self._current_dm = attributes.getValue('dm')
            def endElement(self, name):
                if name == 'group':
                    self._reset_group()
            def _reset_group(self):
                self._current_dt = 'none'
                self._current_dm = '#'

        print('Generating \'Decomposition_Mapping\' code table...')
        xml.sax.parse(self._ucdxml_filename, ContentHandler(self._output_files['ct']))
        out = self._open_output_file('decomposition-mapping-table')
        canonical_mappings.sort()
        compatibility_mappings.sort()
        canonical_mappings_offsets, compatibility_mappings_offsets = [0], [0]
        for m in canonical_mappings:
            canonical_mappings_offsets.append(canonical_mappings_offsets[-1] + len(m[1]))
        for m in compatibility_mappings:
            compatibility_mappings_offsets.append(compatibility_mappings_offsets[-1] + len(m[1]))
        out.write('#ifndef ASCENSION_NO_UNICODE_NORMALIZATION\n'
                  + r'const CodePoint CANONICAL_MAPPING_SOURCE[] = {')
        out.write(','.join([r'0x%x' % x[0] for x in canonical_mappings]))
        out.write('};\nconst Char CANONICAL_MAPPING_DESTINATION[] = {')
        out.write(','.join([utf_16_to_cpp_literal(x[1]) for x in canonical_mappings]))
        out.write('};\nconst std::ptrdiff_t CANONICAL_MAPPINGS_OFFSETS[] = {' + ','.join(str(x) for x in canonical_mappings_offsets))
        out.write('};\n#ifndef ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING\n'
                  + r'const CodePoint COMPATIBILITY_MAPPING_SOURCE[] = {')
        out.write(','.join([r'0x%x' % x[0] for x in compatibility_mappings]))
        out.write('};\nconst Char COMPATIBILITY_MAPPING_DESTINATION[] = {')
        out.write(','.join([utf_16_to_cpp_literal(x[1]) for x in compatibility_mappings]))
        out.write('};\nconst std::ptrdiff_t COMPATIBILITY_MAPPINGS_OFFSETS[] = {' + ','.join(str(x) for x in compatibility_mappings_offsets))
        out.write('};\n#endif // !ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING\n'
                  + '#endif // !ASCENSION_NO_UNICODE_NORMALIZATION\n')
        print('...Processed canonical mappings (%d).' % len(canonical_mappings))
        print('...Processed compatibility mappings (%d).' % len(compatibility_mappings))
        print('...Done,')

    def _process_blocks(self):
        class ContentHandler(xml.sax.ContentHandler):
            def __init__(self, outer):
                self._outer = outer
                sys.stdout.write('Generating code for property \'Block\'...')
                self._blocks = []
            def startElement(self, name, attributes):
                if name == 'block':
                    self._blocks.append((int(attributes.getValue('first-cp'), 16),
                                         int(attributes.getValue('last-cp'), 16) + 1,
                                         attributes.getValue('name')))
            def endElement(self, name):
                if name != 'blocks':
                    return
                self._blocks.sort()
                code_table = self._outer._output_files['ct']
                value_names = self._outer._output_files['vn']
                code_table.write(r'const detail::CharacterPropertyPartition ucd::Block::VALUES_[] = {')
                for i, block in enumerate(self._blocks):
                    cpp_name = PropertyNames.cpp_value_name(block[2])
                    self._outer._output_files['bd'].write('%s, ///< %s.\n' % (cpp_name, block[2]))
                    if i != 0 and self._blocks[i - 1][1] != block[0]:
                        code_table.write('{0x%x,ucd::Block::NO_BLOCK},' % self._blocks[i - 1][0])
                    code_table.write(r'{0x%x,ucd::Block::%s},' % (block[0], cpp_name))
                code_table.write('};\n')
                code_table.write('const std::size_t ucd::Block::NUMBER_ = std::extent<decltype(ucd::Block::VALUES_)>::value;\n')
                print(' ([%d])' % len(self._blocks))
        self._open_output_file(r'blocks-definition')
        xml.sax.parse(self._ucdxml_filename, ContentHandler(self))
        self._print_partitioned_of_code('Block')
        self._print_value_names('Block')

    def _process_partitioned_property(self, long_name):
        sys.stdout.write('Generating code for property \'%s\'...' % long_name)
        cpp_name = PropertyNames.cpp_name(long_name)
        out_ct = self._output_files['ct']
        out_i = self._output_files['i']
        out_ct.write(r'const detail::CharacterPropertyPartition %s::VALUES_[] = {' % cpp_name)
        short_name = self._PROPERTY_NAMES.short_name(long_name)
        ps = self._make_property_partitions(short_name)
        for p in ps:
            out_ct.write(r'{0x%x,%s::%s},'
                         % (p[0], cpp_name, PropertyNames.cpp_value_name(self._PROPERTY_NAMES.long_value_name(short_name, p[1]))))
        out_ct.write('};\n')
        out_ct.write('const std::size_t %s::NUMBER_ = std::extent<decltype(%s::VALUES_)>::value;\n' % (cpp_name, cpp_name))
        self._print_partitioned_of_code(long_name)
        self._print_value_names(long_name)
        print(' ([%d])' % len(ps))


def main():
    CodeGenerator(sys.argv[1], sys.argv[2], r'../src/generated/').main()
    print('Done.')

if __name__ == '__main__':
    main()

