/**
 * @file normalizer.cpp
 * @author exeal
 * @date 2007-2011
 */

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_*
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
#include <ascension/corelib/text/normalizer.hpp>
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/utf-iterator.hpp>
#include <algorithm>	// std.find_if, std.lower_bound, std.sort
using namespace ascension;
using namespace ascension::text;
using namespace std;
using text::ucd::CanonicalCombiningClass;


// Normalizer /////////////////////////////////////////////////////////////////////////////////////

/**
 * @class ascension::text::Normalizer normalizer.hpp
 * @c Nomalizer supports the standard normalization forms described in
 * <a href="http://www.unicode.org/reports/tr15/">UAX #15: Unicode Normalization Forms</a> revision 27.
 * One can use this to normalize text or compare two strings for canonical equivalence for sorting
 * or searching text.
 *
 * This class has an interface for C++ standard bidirectional iterator and returns normalized text
 * incrementally. The following illustrates this:
 *
 * @code
 * String text(L"C\x0301\x0327");  // as if String is std.wstring...
 * Normalizer n(StringCharacterIterator(text), Normalizer::FORM_NFD);
 * // print all normalized characters of the sequence.
 * while(n.hasNext()) {
 *   std::cout << std::dec << (n.tell() - text.data()) << " : "
 *             << std::hex << *n << std::endl;
 *   ++n;
 * }
 * @endcode
 *
 * @c Normalizer is boundary safe. @c #hasNext and @c #hasPreviouscheck itself is at the boundary.
 * If you increment or decrement the iterator over that boundary, @c std#out_of_range exception
 * will be thrown.
 *
 * The @c operator* returns the code point of normalized character, not the character unit value.
 *
 * This class supports also "Fast C or D" form described in
 * <a href="http://www.unicode.org/notes/tn5/">UTN #5: Canonical Equivalence in Application</a> for
 * efficient processing on unnormalized text.
 *
 * Notice that an instance of this class does not duplicate the input string. If you dispose the input
 * after an instance is initialized, the result is undefined.
 *
 * This class also has static functions to provide fundamental processes for normalization.
 *
 * This class is not available if configuration symbol @c ASCENSION_NO_UNICODE_NORMALIZATION is defind.
 * And the features about compatibility mapping are not available if configuration symbol
 * @c ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING is defind. See the description of config.hpp.
 *
 * @note This class is not derivable.
 */

// internal helpers
namespace {
	// Based on 3.12 Combining Jamo Behavior and UAX #15 X16 Hangul of Unicode 5.0.
	const Char
		S_BASE = 0xac00, L_BASE = 0x1100, V_BASE = 0x1161, T_BASE = 0x11a7,
		L_COUNT = 19, V_COUNT = 21, T_COUNT = 28, N_COUNT = V_COUNT * T_COUNT, S_COUNT = L_COUNT * N_COUNT;

	/**
	 * Decomposes a Hangul character.
	 * @param c The character to decompose
	 * @param[out] destination The destination buffer. the size must be greater than 2
	 * @return The number of the characters written to @a destination. either 0, 2, or 3
	 */
	length_t decomposeHangul(Char c, Char* destination) {
		// from The Unicode Standard 5.0 pp.1356
		const Char s = c - S_BASE;
		if(c < S_BASE || s >= S_COUNT)
			return 0;
		*(destination++) = L_BASE + s / N_COUNT;				// L
		*(destination++) = V_BASE + (s % N_COUNT) / T_COUNT;	// V
		const Char t = T_BASE + s % T_COUNT;
		return (t != T_BASE) ? ((*destination = t), 3) : 2;
	}

	basic_string<CodePoint> composeHangul(
			utf::CharacterDecodeIterator<const Char*> i, utf::CharacterDecodeIterator<const Char*> e) {
		// from The Unicode Standard 5.0 pp.1356~1357
		if(i == e)
			return basic_string<CodePoint>();
		basic_stringbuf<CodePoint> result;
		CodePoint last = *i;

		while(++i != e) {
			const CodePoint c = *i;

			// 1. check to see if two current characters are L and V
			if(last >= L_BASE && c >= V_BASE) {
				const CodePoint lIndex = last - L_BASE, vIndex = c - V_BASE;
				if(lIndex < L_COUNT && vIndex < V_COUNT) {
					// make syllable of form LV
					last = S_BASE + (lIndex * V_COUNT + vIndex) * T_COUNT;
					result.pubseekoff(-1, ios_base::cur); result.sputc(last);	// reset last
					continue;	// discard c
				}
			}

			// 2. check to see if two current characters are LV and T
			if(last >= S_BASE && c >= T_BASE) {
				const CodePoint sIndex = last - S_BASE, tIndex = c - T_BASE;
				if(sIndex < S_COUNT && tIndex < T_COUNT && (sIndex % T_COUNT) == 0) {
					// make syllable of form LVT
					last += tIndex;
					result.pubseekoff(-1, ios_base::cur); result.sputc(last);	// reset last
					continue;	// discard c
				}
			}

			// if neither case was true, just add the character
			last = c;
			result.sputc(c);
		}
		return result.str();
	}

	struct LessCCC {
		bool operator()(CodePoint c1, CodePoint c2) {
			return CanonicalCombiningClass::of(c1) < CanonicalCombiningClass::of(c2);
		}
	};

	/**
	 * Reorders the combining marks in the given character sequence according to "3.11 Canonical
	 * Ordering Behavior" of Unicode standard 5.0.
	 * @param[in,out] s The decomposed character sequence
	 */
	inline void reorderCombiningMarks(basic_string<CodePoint>& s) {
		basic_string<CodePoint>::iterator current(s.begin()), next;
		const basic_string<CodePoint>::iterator last(s.end());
		while(current != last) {
			next = find_if(current, last, not1(ptr_fun(CanonicalCombiningClass::of)));
			if(next != last)
				++next;
			sort(current, next, LessCCC());
			current = next;
		}
	}

#include "../../generated/uprops-decomposition-mapping-table"

	inline void splice(Char* at, Char*& last, size_t eraseLength, const Char* replacement, size_t replacementLength) {
		last += replacementLength - eraseLength;
		if(eraseLength < replacementLength) {
			for(Char* p = last - 1; p >= at + replacementLength; --p)
				*p = p[eraseLength - replacementLength];
		}
		copy(replacement, replacement + replacementLength, at);
	}

	/**
	 * Decomposes the given character but does not reorder combining marks.
	 * @param c The code point of the character to decompose
	 * @param compatibility Set @c true to perform compatibility decomposition
	 * @param[out] destination The destination buffer
	 * @return The length of the decomposition
	 */
	length_t internalDecompose(CodePoint c, bool compatibility, Char* destination) {
		Char* last = destination + (utf16::checkedEncode(c, destination) < 2 ? 1 : 2);
		length_t len;
		CodePoint current;
		Char decomposedHangul[4];
		const CodePoint* src;
		for(utf::CharacterDecodeIteratorUnsafe<Char*> i(destination); i.tell() < last; ) {
			current = *i;
			if(current < 0x010000ul && (0 != (len = decomposeHangul(static_cast<Char>(current & 0xffffu), decomposedHangul)))) {
				splice(i.tell(), last, 1, decomposedHangul, len);
				continue;
			}
			src = lower_bound(CANONICAL_MAPPING_SOURCE, ASCENSION_ENDOF(CANONICAL_MAPPING_SOURCE), current);
			if(*src == current) {
				splice(i.tell(), last, (current < 0x010000ul) ? 1 : 2,
					&CANONICAL_MAPPING_DESTINATION[src - CANONICAL_MAPPING_SOURCE],
					CANONICAL_MAPPINGS_OFFSETS[src - CANONICAL_MAPPING_SOURCE + 1]
						- CANONICAL_MAPPINGS_OFFSETS[src - CANONICAL_MAPPING_SOURCE]);
				continue;
			}
#ifndef ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
			else if(compatibility) {
				src = lower_bound(COMPATIBILITY_MAPPING_SOURCE, ASCENSION_ENDOF(COMPATIBILITY_MAPPING_SOURCE), current);
				if(*src == current) {
					splice(i.tell(), last, (current < 0x010000ul) ? 1 : 2,
						&COMPATIBILITY_MAPPING_DESTINATION[src - COMPATIBILITY_MAPPING_SOURCE],
						COMPATIBILITY_MAPPINGS_OFFSETS[src - COMPATIBILITY_MAPPING_SOURCE + 1]
							- COMPATIBILITY_MAPPINGS_OFFSETS[src - COMPATIBILITY_MAPPING_SOURCE]);
					continue;
				}
			}
#endif // !ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
			++i;
		}
		return last - destination;
	}

	/**
	 * Composes the string.
	 * @param s The source UTF-32 string to compose
	 * @return The precomposed string
	 */
	basic_string<CodePoint> internalCompose(const basic_string<CodePoint>& s) {
		// TODO: not implemented.
		return s;
	}

	/**
	 * Returns @c true if the specified character sequence is in FCD.
	 * @param first The start of the character sequence
	 * @param last The end of the character sequence
	 * @return true if the sequence is in FCD.
	 */
	template<typename CharacterSequence> inline bool isFCD(CharacterSequence first, CharacterSequence last) {
		ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::value == 2);
		Char buffer[32];
		length_t len;
		int ccc, previous = CanonicalCombiningClass::NOT_REORDERED;
		for(utf::CharacterDecodeIterator<CharacterSequence> i(first, last); i.tell() < last; ++i) {
			len = internalDecompose(*i, false, buffer);
			ccc = CanonicalCombiningClass::of(utf16::decodeFirst(buffer, buffer + len));
			if(ccc != CanonicalCombiningClass::NOT_REORDERED && ccc < previous)
				return false;
			previous = CanonicalCombiningClass::of(utf16::decodeLast(buffer, buffer + len));
		}
		return true;
	}

	/**
	 * Normalizes the given character sequence.
	 * @param first The start of the character sequence
	 * @param last The end of the character sequence
	 * @param form The normalization form
	 * @return The normalized sequence
	 */
	basic_string<CodePoint> internalNormalize(const CharacterIterator& first, const CharacterIterator& last, Normalizer::Form form) {
		Char room[128];
		length_t len;	// length of room
		// decompose
		basic_stringbuf<CodePoint> buffer(ios_base::out);
		for(auto_ptr<CharacterIterator> i(first.clone()); i->offset() < last.offset(); i->next()) {
			len = internalDecompose(i->current(), form == Normalizer::FORM_KD || form == Normalizer::FORM_KC, room);
			for(utf::CharacterDecodeIterator<const Char*> j(room, room + len); j.tell() < room + len; ++j)
				buffer.sputc(*j);
		}
		// reorder combining marks
		basic_string<CodePoint> decomposed(buffer.str());
		reorderCombiningMarks(decomposed);
		// compose if needed
		return (form == Normalizer::FORM_C || form == Normalizer::FORM_KC) ? internalCompose(decomposed) : decomposed;
	}

	/**
	 * Compares the two strings for canonical equivalence. The both strings must be FCD.
	 * @param s1 The first string
	 * @param s2 The second string
	 * @return The result
	 */
	int internalCompare(const String& s1, const String& s2, CaseSensitivity caseSensitivity) {
#if 0
		// this implementation is based on unormcmp.cpp of ICU 3.4 (IBM)
		enum {RAW, CASE_FOLDING, DECOMPSITION};
		UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS>
			i1(s1.data(), s1.data() + s1.length()), i2(s2.data(), s2.data() + s2.length());
		struct Data {
			CodePoint c;
			int phase;
			struct Stack {
				const Char* current;
				const Char* first;
				const Char* last;
			} stack[2];
			Char caseFolded[ASCENSION_INTERNAL_CASE_FOLDING_MAX_EXPANSION];
			Data() : c(INVALID_CODE_POINT), phase(RAW) {}
		} data1, data2;

		while(true) {
			if(data1.c == INVALID_CODE_POINT) {
				// get the next character from s1
			}
			if(data2.c == INVALID_CODE_POINT) {
				// get the next character from s1
			}

			if(data1.c == data2.c) {
				if(data1.c == INVALID_CODE_POINT)
					return 0;
				data1.c = data2.c = INVALID_CODE_POINT;
				continue;
			} else if(data1.c == INVALID_CODE_POINT)	// s1 < s2
				return -1;
			else if(data2.c == INVALID_CODE_POINT)	// s1 > s2
				return +1;

			if(data1.phase == RAW && caseSensitivity != CASE_SENSITIVE) {
				data1.c = INVALID_CODE_POINT;
				continue;
			}

			if(data2.phase == RAW && caseSensitivity != CASE_SENSITIVE) {
				data2.c = INVALID_CODE_POINT;
				continue;
			}

			if(data1.phase != DECOMPOSITION) {
				data1.c = INVALID_CODE_POINT;
				continue;
			}

			if(data2.phase != DECOMPOSITION) {
				data2.c = INVALID_CODE_POINT;
				continue;
			}

			return static_cast<int>(data1.c) - static_cast<int>(data2.c);
		}
#endif
		// TODO: not implemented.
		return s1.compare(s2);
	}
} // namespace @0

/// Default constructor.
Normalizer::Normalizer() {
}

/**
 * Constructor.
 * @param text The text to be normalized
 * @param form The normalization form
 */
Normalizer::Normalizer(const CharacterIterator& text, Form form) : form_(form), current_(text.clone()) {
	nextClosure(Direction::FORWARD, true);
}

/// Copy-constructor.
Normalizer::Normalizer(const Normalizer& rhs) : form_(rhs.form_),
		current_(rhs.current_->clone()), normalizedBuffer_(rhs.normalizedBuffer_), indexInBuffer_(rhs.indexInBuffer_) {
}

/// Destructor.
Normalizer::~Normalizer() /*throw()*/ {
}

/// Assignment operator.
Normalizer& Normalizer::operator=(const Normalizer& rhs) {
	normalizedBuffer_ = rhs.normalizedBuffer_;
	current_.reset(rhs.current_->clone().release());
	form_ = rhs.form_;
	indexInBuffer_ = rhs.indexInBuffer_;
	return *this;
}

/**
 * Compares the two strings for canonical equivalence.
 * @param s1 The string
 * @param s2 The other string
 * @param caseSensitivity For caseless match
 * @retval &lt;0 @a s1 is less than @a s2
 * @retval 0 The two strings are canonical equivalent
 * @retval &gt;0 @a s1 is greater than @a s2
 */
int Normalizer::compare(const String& s1, const String& s2, CaseSensitivity caseSensitivity) {
	auto_ptr<String>
		nfd1((caseSensitivity == CASE_INSENSITIVE_EXCLUDING_TURKISH_I || !isFCD(s1.begin(), s1.end())) ? new String : 0),
		nfd2((caseSensitivity == CASE_INSENSITIVE_EXCLUDING_TURKISH_I || !isFCD(s2.begin(), s2.end())) ? new String : 0);
	if(nfd1.get() != 0)
		nfd1->assign(normalize(StringCharacterIterator(s1), FORM_D));
	if(nfd2.get() != 0)
		nfd2->assign(normalize(StringCharacterIterator(s2), FORM_D));
	return internalCompare((nfd1.get() != 0) ? *nfd1 : s1, (nfd2.get() != 0) ? *nfd2 : s2, caseSensitivity);
}

/// Normalizes the next or previous closure for the following iteration.
void Normalizer::nextClosure(Direction direction, bool initialize) {
	auto_ptr<CharacterIterator> next;
	if(direction == Direction::FORWARD) {
		if(!initialize) {
			do {
				current_->next();
			} while(current_->offset() < nextOffset_);
		}
		if(!current_->hasNext()) {
			// reached the end of the source character sequence
			indexInBuffer_ = 0;
			return;
		}
		// locate the next starter
		next.reset(current_->clone().release());
		for(next->next(); next->hasNext(); next->next()) {
			if(CanonicalCombiningClass::of(next->current()) == CanonicalCombiningClass::NOT_REORDERED)
				break;
		}
		nextOffset_ = next->offset();
	} else {
		next.reset(current_->clone().release());
		nextOffset_ = current_->offset();
		current_->previous();
		// locate the previous starter
		while(current_->hasPrevious()) {
			if(CanonicalCombiningClass::of(current_->current()) == CanonicalCombiningClass::NOT_REORDERED)
				break;
		}
	}
	normalizedBuffer_ = internalNormalize(*current_, *next, form_);
	indexInBuffer_ = (direction == Direction::FORWARD) ? 0 : normalizedBuffer_.length() - 1;
}

/**
 * Normalizes the specified text according to the normalization form.
 * @param text The text to normalize
 * @param form The normalization form
 */
String Normalizer::normalize(const CharacterIterator& text, Form form) {
	// TODO: there is more efficient implementation.
	Normalizer n(text, form);
	basic_stringbuf<Char> buffer(ios_base::out);
	CodePoint c;
	Char surrogate[2];
	for(Normalizer n(text, form); n.hasNext(); n.next()) {
		c = n.current();
		if(c < 0x010000ul)
			buffer.sputc(static_cast<Char>(c & 0xffffu));
		else {
			assert(isScalarValue(c));
			utf16::uncheckedEncode(c, surrogate);
			buffer.sputn(surrogate, 2);
		}
	}
	return buffer.str();
}

#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
