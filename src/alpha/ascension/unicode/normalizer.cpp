/**
 * @file normalizer.cpp
 * @author exeal
 * @date 2007
 */

#include "stdafx.h"
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
#include "../unicode-property.hpp"
using namespace ascension;
using namespace ascension::unicode;
using namespace std;


/**
 * @class ascension::unicode::Normalizer ../unicode.hpp
 * @c Nomalizer supports the standard normalization forms described in
 * <a href="http://www.unicode.org/reports/tr15/">UAX #15: Unicode Normalization Forms</a> revision 27.
 *
 * This class has an interface for C++ standard bidirectional iterator and returns normalized text
 * incrementally. The following illustrates this:
 *
 * @code
 * String text(L"C\x0301\x0327");
 * Normalizer n(text, Normalizer::FORM_NFD);
 * // print all normalized characters of the sequence.
 * while(!n.isLast()) {
 *   std::cout << std::dec << (n.tell() - text.data()) << " : "
 *             << std::hex << *n << std::endl;
 *   ++n;
 * }
 * @endcode
 *
 * @c Normalizer is boundary safe. @c #isFirst and @c #isLast check the iterator is at the boundary.
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
		S_BASE = 0xAC00, L_BASE = 0x1100, V_BASE = 0x1161, T_BASE = 0x11A7,
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

	basic_string<CodePoint> composeHangul(UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> i) {
		// from The Unicode Standard 5.0 pp.1356~1357
		if(i.isLast())
			return basic_string<CodePoint>();
		basic_stringbuf<CodePoint> result;
		CodePoint last = *i;

		while(!(++i).isLast()) {
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

#include "../code-table/uprops-decomposition-mapping-table"

	inline void splice(Char* at, Char*& last, size_t eraseLength, const Char* replacement, size_t replacementLength) throw() {
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
		Char* last = destination + surrogates::encode(c, destination);
		length_t len;
		CodePoint current;
		Char decomposedHangul[4];
		const CodePoint* src;
		for(UTF16To32Iterator<Char*, utf16boundary::DONT_CHECK> i(destination); i.tell() < last; ) {
			current = *i;
			if(current < 0x010000 && (0 != (len = decomposeHangul(static_cast<Char>(current & 0xFFFF), decomposedHangul)))) {
				splice(i.tell(), last, 1, decomposedHangul, len);
				continue;
			}
			src = lower_bound(CANONICAL_MAPPING_SRC, CANONICAL_MAPPING_SRC + NUMBER_OF_CANONICAL_MAPPINGS, current);
			if(*src == current) {
				splice(i.tell(), last, (current < 0x010000) ? 1 : 2,
					CANONICAL_MAPPING_DEST[src - CANONICAL_MAPPING_SRC],
					wcslen(CANONICAL_MAPPING_DEST[src - CANONICAL_MAPPING_SRC]));
				continue;
			}
#ifndef ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
			else if(compatibility) {
				src = lower_bound(COMPATIBILITY_MAPPING_SRC, COMPATIBILITY_MAPPING_SRC + NUMBER_OF_COMPATIBILITY_MAPPINGS, current);
				if(*src == current) {
					splice(i.tell(), last, (current < 0x010000) ? 1 : 2,
						COMPATIBILITY_MAPPING_DEST[src - COMPATIBILITY_MAPPING_SRC],
						wcslen(COMPATIBILITY_MAPPING_DEST[src - COMPATIBILITY_MAPPING_SRC]));
					continue;
				}
			}
#endif /* !ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING */
			++i;
		}
		return last - destination;
	}

	/***/
	basic_string<CodePoint> compose(const basic_string<CodePoint>& s) {
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
		Char buffer[32];
		length_t len;
		int ccc, previous = CanonicalCombiningClass::NOT_REORDERED;
		for(UTF16To32Iterator<CharacterSequence, utf16boundary::USE_BOUNDARY_ITERATORS> i(first, last); !i.isLast(); ++i) {
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
		for(auto_ptr<CharacterIterator> i(first.clone()); i->getOffset() < last.getOffset(); i->next()) {
			len = internalDecompose(i->current(), form == Normalizer::FORM_KD || form == Normalizer::FORM_KC, room);
			for(UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> j(room, room + len); !j.isLast(); ++j)
				buffer.sputc(*j);
		}
		// reorder combining marks
		basic_string<CodePoint> decomposed(buffer.str());
		reorderCombiningMarks(decomposed);
		// compose if needed
		return (form == Normalizer::FORM_C || form == Normalizer::FORM_KC) ? compose(decomposed) : decomposed;
	}

	/**
	 * Compares the two strings for canonical equivalence. The both strings must be FCD.
	 * @param s1 the first string
	 * @param s2 the second string
	 * @return the result
	 */
	int internalCompare(const String& s1, const String& s2, CaseSensitivity caseSensitivity) {
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
	nextClosure(FORWARD, true);
}

/// Copy-constructor.
Normalizer::Normalizer(const Normalizer& rhs) : form_(rhs.form_),
		current_(rhs.current_->clone()), normalizedBuffer_(rhs.normalizedBuffer_), indexInBuffer_(rhs.indexInBuffer_) {
}

/// Destructor.
Normalizer::~Normalizer() throw() {
}

/// Assignment operator.
Normalizer& Normalizer::operator=(const Normalizer& rhs) {
	normalizedBuffer_ = rhs.normalizedBuffer_;
	current_ = rhs.current_->clone();
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
	if(direction == FORWARD) {
		if(!initialize) {
			do {
				current_->next();
			} while(current_->getOffset() < nextOffset_);
		}
		if(current_->isLast()) {
			// reached the end of the source character sequence
			indexInBuffer_ = 0;
			return;
		}
		// locate the next starter
		next = current_->clone();
		for(next->next(); !next->isLast(); next->next()) {
			if(CanonicalCombiningClass::of(next->current()) == CanonicalCombiningClass::NOT_REORDERED)
				break;
		}
		nextOffset_ = next->getOffset();
	} else {
		next = current_->clone();
		nextOffset_ = current_->getOffset();
		current_->previous();
		// locate the previous starter
		while(!current_->isFirst()) {
			if(CanonicalCombiningClass::of(current_->current()) == CanonicalCombiningClass::NOT_REORDERED)
				break;
		}
	}
	normalizedBuffer_ = internalNormalize(*current_, *next, form_);
	indexInBuffer_ = (direction == FORWARD) ? 0 : normalizedBuffer_.length() - 1;
}

/**
 * Normalizes the specified text according to the normalization form.
 * @param text the text to normalize
 * @param form the normalization form
 */
String Normalizer::normalize(const CharacterIterator& text, Form form) {
	// TODO: there is more efficient implementation.
	Normalizer n(text, form);
	StringBuffer buffer(ios_base::out);
	CodePoint c;
	Char surrogate[2];
	for(Normalizer n(text, form); !n.isLast(); ++n) {
		c = *n;
		if(c < 0x010000)
			buffer.sputc(static_cast<Char>(c & 0xFFFF));
		else {
			surrogates::encode(c, surrogate);
			buffer.sputn(surrogate, 2);
		}
	}
	return buffer.str();
}

#endif /* !ASCENSION_NO_UNICODE_NORMALIZATION */
