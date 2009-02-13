/**
 * @file normalizer.cpp
 * @author exeal
 * @date 2007-2009
 */

#include <ascension/unicode-property.hpp>
using namespace ascension;
using namespace ascension::text;
using namespace std;
using text::ucd::CanonicalCombiningClass;


// CaseFolder ///////////////////////////////////////////////////////////////

/// The maximum number of characters folded from one character.
const length_t CaseFolder::MAXIMUM_EXPANSION_CHARACTERS = 3;

/**
 * Compares the two character sequences case-insensitively.
 * @param s1 the character sequence
 * @param s2 the the other
 * @param excludeTurkishI set true to perform "Turkish I mapping"
 * @retval &lt;0 the first character sequence is less than the second
 * @retval 0 the both sequences are same
 * @retval &gt;0 the first character sequence is greater than the second
 */
int CaseFolder::compare(const CharacterIterator& s1, const CharacterIterator& s2, bool excludeTurkishI /* = false */) {
	auto_ptr<CharacterIterator> i1(s1.clone()), i2(s2.clone());
	CodePoint c1 = CharacterIterator::DONE, c2 = CharacterIterator::DONE;
	CodePoint folded1[MAXIMUM_EXPANSION_CHARACTERS], folded2[MAXIMUM_EXPANSION_CHARACTERS];
	const CodePoint* p1 = folded1 - 1;
	const CodePoint* p2 = folded2 - 1;
	const CodePoint* last1 = p1;
	const CodePoint* last2 = p2;

	while(true) {
		if(c1 == CharacterIterator::DONE) {
			if(p1 >= folded1 && p1 < last1)
				c1 = *p1++;
			else if(CharacterIterator::DONE != (c1 = i1->current())) {
				i1->next();
				p1 = folded1 - 1;
			}
		}
		if(c2 == CharacterIterator::DONE) {
			if(p2 >= folded2 && p2 < last2)
				c2 = *p2++;
			else if(CharacterIterator::DONE != (c2 = i2->current())) {
				i2->next();
				p2 = folded2 - 1;
			}
		}

		if(c1 == c2) {
			if(c1 == CharacterIterator::DONE)
				return 0;
			c1 = c2 = CharacterIterator::DONE;
			continue;
		} else if(c1 == CharacterIterator::DONE)
			return -1;
		else if(c2 == CharacterIterator::DONE)
			return +1;

		// fold c1
		if(p1 == folded1 - 1) {
			p1 = folded1;
			last1 = p1 + foldFull(c1, excludeTurkishI, folded1);
			c1 = CharacterIterator::DONE;
			continue;
		}
		// fold c2
		if(p2 == folded2 - 1) {
			p2 = folded2;
			last2 = p2 + foldFull(c2, excludeTurkishI, folded2);
			c2 = CharacterIterator::DONE;
			continue;
		}

		return static_cast<int>(c1) - static_cast<int>(c2);
	}
}

inline size_t CaseFolder::foldFull(CodePoint c, bool excludeTurkishI, CodePoint* dest) /*throw()*/ {
	if(!excludeTurkishI || c == (*dest = foldTurkishI(c)))
		*dest = foldCommon(c);
	if(*dest != c || c >= 0x010000ul)
		return 1;
	else {
		const CodePoint* const p = lower_bound(FULL_CASED_, FULL_CASED_ + NUMBER_OF_FULL_CASED_, c);
		if(*p != c)
			return 1;
		const size_t len = wcslen(FULL_FOLDED_[p - FULL_CASED_]);
		for(size_t i = 0; i < len; ++i)
			dest[i] = FULL_FOLDED_[p - FULL_CASED_][i];
		return len;
	}
}


#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
#include <ascension/unicode-property.hpp>

// Normalizer ///////////////////////////////////////////////////////////////

/**
 * @class ascension::text::Normalizer ../unicode.hpp
 * @c Nomalizer supports the standard normalization forms described in
 * <a href="http://www.unicode.org/reports/tr15/">UAX #15: Unicode Normalization Forms</a> revision 27.
 * One can use this to normalize text or compare two strings for canonical equivalence for sorting
 * or searching text.
 *
 * This class has an interface for C++ standard bidirectional iterator and returns normalized text
 * incrementally. The following illustrates this:
 *
 * @code
 * String text(L"C\x0301\x0327");
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
	 * @param c the character to decompose
	 * @param[out] destination the destination buffer. the size must be greater than 2
	 * @return the number of the characters written to @a destination. either 0, 2, or 3
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

	basic_string<CodePoint> composeHangul(UTF16To32Iterator<const Char*> i) {
		// from The Unicode Standard 5.0 pp.1356~1357
		if(!i.hasNext())
			return basic_string<CodePoint>();
		basic_stringbuf<CodePoint> result;
		CodePoint last = *i;

		while((++i).hasNext()) {
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
	 * @param[in,out] s the decomposed character sequence
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

#include "../generated/uprops-decomposition-mapping-table"

	inline void splice(Char* at, Char*& last, size_t eraseLength, const Char* replacement, size_t replacementLength) {
		last += replacementLength - eraseLength;
		if(eraseLength < replacementLength) {
			for(Char* p = last - 1; p >= at + replacementLength; --p)
				*p = p[eraseLength - replacementLength];
		}
		wmemcpy(at, replacement, replacementLength);
	}

	/**
	 * Decomposes the given character but does not reorder combining marks.
	 * @param c the code point of the character to decompose
	 * @param compatibility set true to perform compatibility decomposition
	 * @param[out] destination the destination buffer
	 * @return the length of the decomposition
	 */
	length_t internalDecompose(CodePoint c, bool compatibility, Char* destination) {
		Char* last = destination + (surrogates::encode(c, destination) < 2 ? 1 : 2);
		length_t len;
		CodePoint current;
		Char decomposedHangul[4];
		const CodePoint* src;
		for(UTF16To32IteratorUnsafe<Char*> i(destination); i.tell() < last; ) {
			current = *i;
			if(current < 0x010000ul && (0 != (len = decomposeHangul(static_cast<Char>(current & 0xffffu), decomposedHangul)))) {
				splice(i.tell(), last, 1, decomposedHangul, len);
				continue;
			}
			src = lower_bound(CANONICAL_MAPPING_SOURCE, MANAH_ENDOF(CANONICAL_MAPPING_SOURCE), current);
			if(*src == current) {
				splice(i.tell(), last, (current < 0x010000ul) ? 1 : 2,
					CANONICAL_MAPPING_DESTINATION[src - CANONICAL_MAPPING_SOURCE],
					wcslen(CANONICAL_MAPPING_DESTINATION[src - CANONICAL_MAPPING_SOURCE]));
				continue;
			}
#ifndef ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
			else if(compatibility) {
				src = lower_bound(COMPATIBILITY_MAPPING_SOURCE, MANAH_ENDOF(COMPATIBILITY_MAPPING_SOURCE), current);
				if(*src == current) {
					splice(i.tell(), last, (current < 0x010000ul) ? 1 : 2,
						COMPATIBILITY_MAPPING_DESTINATION[src - COMPATIBILITY_MAPPING_SOURCE],
						wcslen(COMPATIBILITY_MAPPING_DESTINATION[src - COMPATIBILITY_MAPPING_SOURCE]));
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
	 * @param s the source UTF-32 string to compose
	 * @return the precomposed string
	 */
	basic_string<CodePoint> internalCompose(const basic_string<CodePoint>& s) {
		// TODO: not implemented.
		return s;
	}

	/**
	 * Returns true if the specified character sequence is in FCD.
	 * @param first the start of the character sequence
	 * @param last the end of the character sequence
	 * @return true if the sequence is in FCD.
	 */
	template<typename CharacterSequence> inline bool isFCD(CharacterSequence first, CharacterSequence last) {
		MANAH_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::result == 2);
		Char buffer[32];
		length_t len;
		int ccc, previous = CanonicalCombiningClass::NOT_REORDERED;
		for(UTF16To32Iterator<CharacterSequence> i(first, last); i.hasNext(); ++i) {
			len = internalDecompose(*i, false, buffer);
			ccc = CanonicalCombiningClass::of(surrogates::decodeFirst(buffer, buffer + len));
			if(ccc != CanonicalCombiningClass::NOT_REORDERED && ccc < previous)
				return false;
			previous = CanonicalCombiningClass::of(surrogates::decodeLast(buffer, buffer + len));
		}
		return true;
	}

	/**
	 * Normalizes the given character sequence.
	 * @param first the start of the character sequence
	 * @param last the end of the character sequence
	 * @param form the normalization form
	 * @return the normalized sequence
	 */
	basic_string<CodePoint> internalNormalize(const CharacterIterator& first, const CharacterIterator& last, Normalizer::Form form) {
		Char room[128];
		length_t len;	// length of room
		// decompose
		basic_stringbuf<CodePoint> buffer(ios_base::out);
		for(auto_ptr<CharacterIterator> i(first.clone()); i->offset() < last.offset(); i->next()) {
			len = internalDecompose(i->current(), form == Normalizer::FORM_KD || form == Normalizer::FORM_KC, room);
			for(UTF16To32Iterator<const Char*> j(room, room + len); j.hasNext(); ++j)
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
	 * @param s1 the first string
	 * @param s2 the second string
	 * @return the result
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
 * @param text the text to be normalized
 * @param form the normalization form
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
 * @param s1 the string
 * @param s2 the other string
 * @param caseSensitivity for caseless match
 * @retval &lt;0 @a s1 is less than @a s2
 * @retval 0 the two strings are canonical equivalent
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
 * @param text the text to normalize
 * @param form the normalization form
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
			surrogates::encode(c, surrogate);
			buffer.sputn(surrogate, 2);
		}
	}
	return buffer.str();
}

#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
