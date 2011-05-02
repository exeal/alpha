/**
 * @file case-folder.hpp
 * @author exeal
 * @date 2005-2011
 * @date 2011-04-25 deparated from unicode.hpp
 */

#ifndef ASCENSION_CASE_FOLDER_HPP
#define ASCENSION_CASE_FOLDER_HPP

#include <ascension/corelib/character-iterator.hpp>
#include <ascension/corelib/text/unicode-utf.hpp>
#include <algorithm>	// std.lower_bound
#include <sstream>		// std.basic_stringbuf

#if ASCENSION_UNICODE_VERSION > 0x0510
#error These class definitions and implementations are based on old version of Unicode.
#endif

namespace ascension {
	namespace text {

		/**
		 * @c CaseFolder folds cases of characters and strings. This behavior is based on Default
		 * Case Algorithm of Unicode, and locale-independent and context-insensitive.
		 * @see Collator, Normalizer, searcher#LiteralPattern
		 */
		class CaseFolder {
			ASCENSION_NONCOPYABLE_TAG(CaseFolder);
		public:
			static const length_t MAXIMUM_EXPANSION_CHARACTERS;
			static int compare(const CharacterIterator& s1,
				const CharacterIterator& s2, bool excludeTurkishI = false);

			/**
			 * Compares the two character sequences case-insensitively.
			 * @param s1 The character sequence
			 * @param s2 The the other
			 * @param excludeTurkishI Set @c true to perform "Turkish I mapping"
			 * @retval &lt;0 The first character sequence is less than the second
			 * @retval 0 The both sequences are same
			 * @retval &gt;0 The first character sequence is greater than the second
			 */
			static int compare(const String& s1, const String& s2, bool excludeTurkishI = false) {
				return compare(StringCharacterIterator(s1),
					StringCharacterIterator(s2), excludeTurkishI);
			}

			/**
			 * Folds the case of the specified character. This method performs "simple case
			 * folding."
			 * @param c The code point of the character to fold
			 * @param excludeTurkishI Set @c true to perform "Turkish I mapping"
			 * @return The case-folded character
			 */
			static CodePoint fold(CodePoint c, bool excludeTurkishI = false) /*throw()*/ {
				CodePoint result;
				// Turkish I
				if(excludeTurkishI && c != (result = foldTurkishI(c)))
					return result;
				// common mapping
				if(c != (result = foldCommon(c)))
					return result;
				// simple mapping
				const CodePoint* const p = std::lower_bound(
					SIMPLE_CASED_, SIMPLE_CASED_ + NUMBER_OF_SIMPLE_CASED_, c);
				return (*p == c) ? SIMPLE_FOLDED_[p - SIMPLE_CASED_] : c;
			}

			template<typename CharacterSequence> static String fold(
				CharacterSequence first, CharacterSequence last, bool excludeTurkishI = false);

			/**
			 * Folds case of the specified character sequence. This method performs "full case
			 * folding."
			 * @param text The character sequence
			 * @param excludeTurkishI Set @c true to perform "Turkish I mapping"
			 * @return The folded string
			 */
			static String fold(const String& text, bool excludeTurkishI = false) {
				return fold(text.data(), text.data() + text.length(), excludeTurkishI);
			}

		private:
			static CodePoint foldCommon(CodePoint c) /*throw()*/ {
				const CodePoint* const p = std::lower_bound(
					COMMON_CASED_, COMMON_CASED_ + NUMBER_OF_COMMON_CASED_, c);
				return (*p == c) ? COMMON_FOLDED_[p - COMMON_CASED_] : c;
			}
			template<typename OutputIterator>
			static std::size_t foldFull(CodePoint c, bool excludeTurkishI, OutputIterator out) {
				ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<OutputIterator>::value == 4);
				CodePoint first;
				if(!excludeTurkishI || c == (first = foldTurkishI(c)))
					first = foldCommon(c);
				if(first == c && c < 0x010000ul) {
					const CodePoint* const p = std::lower_bound(
						FULL_CASED_, FULL_CASED_ + NUMBER_OF_FULL_CASED_, c);
					if(*p == c) {
						const ptrdiff_t* const offset = &FULL_FOLDED_OFFSETS_[p - FULL_CASED_];
						for(ptrdiff_t i = 0; i < offset[1] - offset[0]; ++i, ++out)
							*out = FULL_FOLDED_[*offset + i];
						return offset[1] - offset[0];
					}
				}
				return (*out = first), ++out, 1;
			}
			static CodePoint foldTurkishI(CodePoint c) /*throw()*/ {
				if(c == 0x0049u)
					c = 0x0131u;
				else if(c == 0x0130u)
					c = 0x0069u;
				return c;
			}
		private:
			static const CodePoint COMMON_CASED_[],
				COMMON_FOLDED_[], SIMPLE_CASED_[], SIMPLE_FOLDED_[], FULL_CASED_[];
			static const Char FULL_FOLDED_[];
			static const std::ptrdiff_t FULL_FOLDED_OFFSETS_[];
			static const std::size_t
				NUMBER_OF_COMMON_CASED_, NUMBER_OF_SIMPLE_CASED_, NUMBER_OF_FULL_CASED_;
		};

		// inline implementations /////////////////////////////////////////////////////////////////

		/**
		 * Folds case of the specified character sequence. This method performs "full case folding."
		 * @a CharacterSequence must represents a UTF-16 character sequence.
		 * @tparam CharacterSequence
		 * @param first the start of the character sequence
		 * @param last the end of the character sequence
		 * @param excludeTurkishI set true to perform "Turkish I mapping"
		 * @return the folded string
		 */
		template<typename CharacterSequence>
		inline String CaseFolder::fold(CharacterSequence first,
				CharacterSequence last, bool excludeTurkishI /* = false */) {
			ASCENSION_STATIC_ASSERT(CodeUnitSizeOf<CharacterSequence>::value == 2);
			using namespace std;
			std::basic_stringbuf<Char> s(ios_base::out);
			CodePoint c, f;
			Char buffer[2];
			for(UTF16To32Iterator<CharacterSequence> i(first, last); i.hasNext(); ++i) {
				c = *i;
				if(!excludeTurkishI || c == (f = foldTurkishI(*i)))
					f = foldCommon(c);
				if(f != c || c >= 0x010000u) {
					if(surrogates::encode(f, buffer) < 2)
						s.sputc(buffer[0]);
					else
						s.sputn(buffer, 2);
				} else {
					const CodePoint* const p = lower_bound(
						FULL_CASED_, FULL_CASED_ + NUMBER_OF_FULL_CASED_, c);
					if(*p == c) {
						const std::ptrdiff_t* const offset = &FULL_FOLDED_OFFSETS_[p - FULL_CASED_];
						s.sputn(FULL_FOLDED_ + offset[0], offset[1] - offset[0]);
					}
					else
						s.sputc(static_cast<Char>(c & 0xffffu));
				}
			}
			return s.str();
		}

	}
} // namespace ascension.text

#endif // !ASCENSION_CASE_FOLDER_HPP
