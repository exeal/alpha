/**
 * @file normalizer.cpp
 * @author exeal
 * @date 2007-2014
 */

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_*
#ifndef ASCENSION_NO_UNICODE_NORMALIZATION
#include <ascension/corelib/text/character-property.hpp>
#include <ascension/corelib/text/normalizer.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>
#include <ascension/corelib/text/utf-iterator.hpp>
#include <boost/optional.hpp>
#include <boost/range/algorithm/lower_bound.hpp>
#include <algorithm>	// std.find_if, std.lower_bound, std.sort


namespace ascension {
	namespace text {
		/**
		 * @class ascension::text::Normalizer normalizer.hpp
		 * @c Nomalizer supports the standard normalization forms described in
		 * <a href="http://www.unicode.org/reports/tr15/">UAX #15: Unicode Normalization Forms</a> revision 27. One can
		 * use this to normalize text or compare two strings for canonical equivalence for sorting or searching text.
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
		 * @c Normalizer is boundary safe. @c #hasNext and @c #hasPreviouscheck itself is at the boundary. If you
		 * increment or decrement the iterator over that boundary, @c std#out_of_range exception will be thrown.
		 *
		 * The @c operator* returns the code point of normalized character, not the character unit value.
		 *
		 * This class supports also "Fast C or D" form described in
		 * <a href="http://www.unicode.org/notes/tn5/">UTN #5: Canonical Equivalence in Application</a> for efficient
		 * processing on unnormalized text.
		 *
		 * Notice that an instance of this class does not duplicate the input string. If you dispose the input after an
		 * instance is initialized, the result is undefined.
		 *
		 * This class also has static functions to provide fundamental processes for normalization.
		 *
		 * This class is not available if configuration symbol @c ASCENSION_NO_UNICODE_NORMALIZATION is defind. And the
		 * features about compatibility mapping are not available if configuration symbol
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
			Index decomposeHangul(Char c, Char* destination) {
				// from The Unicode Standard 5.0 pp.1356
				const Char s = c - S_BASE;
				if(c < S_BASE || s >= S_COUNT)
					return 0;
				*(destination++) = L_BASE + s / N_COUNT;				// L
				*(destination++) = V_BASE + (s % N_COUNT) / T_COUNT;	// V
				const Char t = T_BASE + s % T_COUNT;
				return (t != T_BASE) ? ((*destination = t), 3) : 2;
			}
		
			std::basic_string<CodePoint> composeHangul(
					utf::CharacterDecodeIterator<const Char*> i, utf::CharacterDecodeIterator<const Char*> e) {
				// from The Unicode Standard 5.0 pp.1356~1357
				if(i == e)
					return std::basic_string<CodePoint>();
				std::basic_stringbuf<CodePoint> result;
				CodePoint last = *i;
		
				while(++i != e) {
					const CodePoint c = *i;
		
					// 1. check to see if two current characters are L and V
					if(last >= L_BASE && c >= V_BASE) {
						const CodePoint lIndex = last - L_BASE, vIndex = c - V_BASE;
						if(lIndex < L_COUNT && vIndex < V_COUNT) {
							// make syllable of form LV
							last = S_BASE + (lIndex * V_COUNT + vIndex) * T_COUNT;
							result.pubseekoff(-1, std::ios_base::cur); result.sputc(last);	// reset last
							continue;	// discard c
						}
					}
		
					// 2. check to see if two current characters are LV and T
					if(last >= S_BASE && c >= T_BASE) {
						const CodePoint sIndex = last - S_BASE, tIndex = c - T_BASE;
						if(sIndex < S_COUNT && tIndex < T_COUNT && (sIndex % T_COUNT) == 0) {
							// make syllable of form LVT
							last += tIndex;
							result.pubseekoff(-1, std::ios_base::cur); result.sputc(last);	// reset last
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
					return ucd::CanonicalCombiningClass::of(c1) < ucd::CanonicalCombiningClass::of(c2);
				}
			};

			/**
			 * Reorders the combining marks in the given character sequence according to "3.11 Canonical Ordering
			 * Behavior" of Unicode standard 5.0.
			 * @param[in,out] s The decomposed character sequence
			 */
			inline void reorderCombiningMarks(std::basic_string<CodePoint>& s) {
				std::basic_string<CodePoint>::iterator current(std::begin(s)), next;
				const std::basic_string<CodePoint>::iterator last(end(s));
				while(current != last) {
					next = std::find_if(current, last, [](CodePoint c) {
						return ucd::CanonicalCombiningClass::of(c) == 0;
					});
					if(next != last)
						++next;
					std::sort(current, next, LessCCC());
					current = next;
				}
			}

#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
#	include "../../generated/uprops-decomposition-mapping-table"
#endif

			inline void splice(Char* at, Char*& last, std::size_t eraseLength, const Char* replacement, std::size_t replacementLength) {
				last += replacementLength - eraseLength;
				if(eraseLength < replacementLength) {
					for(Char* p = last - 1; p >= at + replacementLength; --p)
						*p = p[eraseLength - replacementLength];
				}
				std::copy(replacement, replacement + replacementLength, at);
			}

			/**
			 * Decomposes the given character but does not reorder combining marks.
			 * @param c The code point of the character to decompose
			 * @param compatibility Set @c true to perform compatibility decomposition
			 * @param[out] destination The destination buffer
			 * @return The length of the decomposition
			 */
			Index internalDecompose(CodePoint c, bool compatibility, Char* destination) {
				Char* last = destination + (utf::checkedEncode(c, destination) < 2 ? 1 : 2);
				Index len;
				CodePoint current;
				Char decomposedHangul[4];
				const CodePoint* src;
				for(utf::CharacterDecodeIterator<Char*> i(destination, last); i.tell() < i.last(); ) {
					current = *i;
					if(current < 0x010000ul && (0 != (len = decomposeHangul(static_cast<Char>(current & 0xffffu), decomposedHangul)))) {
						splice(i.tell(), last, 1, decomposedHangul, len);
						continue;
					}
					src = boost::lower_bound(CANONICAL_MAPPING_SOURCE, current);
					if(*src == current) {
						splice(i.tell(), last, (current < 0x010000ul) ? 1 : 2,
							&CANONICAL_MAPPING_DESTINATION[src - CANONICAL_MAPPING_SOURCE],
							CANONICAL_MAPPINGS_OFFSETS[src - CANONICAL_MAPPING_SOURCE + 1]
								- CANONICAL_MAPPINGS_OFFSETS[src - CANONICAL_MAPPING_SOURCE]);
						continue;
					}
#ifndef ASCENSION_NO_UNICODE_COMPATIBILITY_MAPPING
					else if(compatibility) {
						src = boost::lower_bound(COMPATIBILITY_MAPPING_SOURCE, current);
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
			std::basic_string<CodePoint> internalCompose(const std::basic_string<CodePoint>& s) {
				// TODO: not implemented.
				return s;
			}

			/**
			 * Returns @c true if the specified character sequence is in FCD.
			 * @tparam SinglePassReadableRange 16-bit character sequence
			 * @param characterSequence The character sequence
			 * @return true if the sequence is in FCD.
			 */
			template<typename SinglePassReadableRange>
			inline bool isFCD(const SinglePassReadableRange& characterSequence) {
				typedef boost::range_iterator<const SinglePassReadableRange>::type Iterator;
				static_assert(CodeUnitSizeOf<Iterator>::value == 2, "SinglePassReadableRange should represent 16-bit character sequence.");
				Char buffer[32];
				Index len;
				int ccc, previous = ucd::CanonicalCombiningClass::NOT_REORDERED;
				for(utf::CharacterDecodeIterator<Iterator> i(boost::begin(characterSequence), boost::end(characterSequence)); i.tell() < boost::end(characterSequence); ++i) {
					len = internalDecompose(*i, false, buffer);
					ccc = ucd::CanonicalCombiningClass::of(utf::decodeFirst(buffer, buffer + len));
					if(ccc != ucd::CanonicalCombiningClass::NOT_REORDERED && ccc < previous)
						return false;
					previous = ucd::CanonicalCombiningClass::of(utf::decodeLast(buffer, buffer + len));
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
			std::basic_string<CodePoint> internalNormalize(const detail::CharacterIterator& first, const detail::CharacterIterator& last, Normalizer::Form form) {
				Char room[128];
				Index len;	// length of room
				// decompose
				std::basic_stringbuf<CodePoint> buffer(std::ios_base::out);
				for(detail::CharacterIterator i(first); i.offset() < last.offset(); ++i) {
					len = internalDecompose(*i, form == Normalizer::FORM_KD || form == Normalizer::FORM_KC, room);
					for(utf::CharacterDecodeIterator<const Char*> j(room, room + len); j.tell() < room + len; ++j)
						buffer.sputc(*j);
				}
				// reorder combining marks
				std::basic_string<CodePoint> decomposed(buffer.str());
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
		
		/// Copy-constructor.
		Normalizer::Normalizer(const Normalizer& other) : form_(other.form_),
				characterIterator_(other.characterIterator_), normalizedBuffer_(other.normalizedBuffer_),
				indexInBuffer_(other.indexInBuffer_), nextOffset_(other.nextOffset_) {
		}
		
		/// Move-constructor.
		Normalizer::Normalizer(Normalizer&& other) BOOST_NOEXCEPT : form_(other.form_), characterIterator_(std::move(other.characterIterator_)),
				normalizedBuffer_(std::move(other.normalizedBuffer_)), indexInBuffer_(other.indexInBuffer_), nextOffset_(other.nextOffset_) {
		}

		/// Copy-assignment operator.
		Normalizer& Normalizer::operator=(const Normalizer& other) {
			normalizedBuffer_ = other.normalizedBuffer_;
			characterIterator_ = other.characterIterator_;
			form_ = other.form_;
			indexInBuffer_ = other.indexInBuffer_;
			nextOffset_ = other.nextOffset_;
			return *this;
		}

		/// Move-assignment operator.
		Normalizer& Normalizer::operator=(Normalizer&& other) BOOST_NOEXCEPT {
			form_ = other.form_;
			characterIterator_ = std::move(other.characterIterator_);
			normalizedBuffer_ = std::move(other.normalizedBuffer_);
			indexInBuffer_ = other.indexInBuffer_;
			nextOffset_ = other.nextOffset_;
			return *this;
		}

		/// Normalizes the next or previous closure for the following iteration.
		void Normalizer::nextClosure(Direction direction, bool initialize) {
			detail::CharacterIterator next;
			if(direction == Direction::FORWARD) {
				if(!initialize) {
					do {
						++characterIterator_;
					} while(characterIterator_.offset() < nextOffset_);
				}
				if(!characterIterator_.hasNext()) {
					// reached the end of the source character sequence
					indexInBuffer_ = 0;
					return;
				}
				// locate the next starter
				next = characterIterator_;
				for(++next; next.hasNext(); ++next) {
					if(ucd::CanonicalCombiningClass::of(*next) == ucd::CanonicalCombiningClass::NOT_REORDERED)
						break;
				}
				nextOffset_ = next.offset();
			} else {
				next = characterIterator_;
				nextOffset_ = characterIterator_.offset();
				--characterIterator_;
				// locate the previous starter
				while(characterIterator_.hasPrevious()) {
					if(ucd::CanonicalCombiningClass::of(*characterIterator_) == ucd::CanonicalCombiningClass::NOT_REORDERED)
						break;
				}
			}
			normalizedBuffer_ = internalNormalize(characterIterator_, next, form_);
			indexInBuffer_ = (direction == Direction::FORWARD) ? 0 : normalizedBuffer_.length() - 1;
		}


		// free functions /////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Compares the two strings for canonical equivalence.
		 * @param s1 The string
		 * @param s2 The other string
		 * @param caseSensitivity For caseless match
		 * @retval &lt;0 @a s1 is less than @a s2
		 * @retval 0 The two strings are canonical equivalent
		 * @retval &gt;0 @a s1 is greater than @a s2
		 */
		int compareForCanonicalEquivalence(const String& s1, const String& s2, CaseSensitivity caseSensitivity) {
			boost::optional<String> nfd1, nfd2;
			if(caseSensitivity == CASE_INSENSITIVE_EXCLUDING_TURKISH_I || !isFCD(s1))
				nfd1 = normalize(StringCharacterIterator(s1), Normalizer::FORM_D);
			if(caseSensitivity == CASE_INSENSITIVE_EXCLUDING_TURKISH_I || !isFCD(s2))
				nfd2 = normalize(StringCharacterIterator(s2), Normalizer::FORM_D);
			return internalCompare(boost::get_optional_value_or(nfd1, s1), boost::get_optional_value_or(nfd2, s2), caseSensitivity);
		}
	}
}

#endif // !ASCENSION_NO_UNICODE_NORMALIZATION
