/**
 * @file newline.hpp
 * @author exeal
 * @date 2003-2006 was EditDoc.h
 * @date 2006-2011 was document.hpp
 * @date 2011-05-02 separated from document.hpp
 * @date 2011-2012
 * @see kernel/document.hpp
 */

#ifndef ASCENSION_NEWLINE_HPP
#define ASCENSION_NEWLINE_HPP

#include <ascension/corelib/basic-exceptions.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/text/character.hpp>
#include <algorithm>	// std.find_first_of

namespace ascension {

	namespace text {

		/**
		 * Value represents a newline in document. @c NLF_RAW_VALUE and @c NLF_DOCUMENT_INPUT are
		 * special values indicate how to interpret newlines during any text I/O.
		 * @see newlineStringLength, newlineString, kernel#Document, ASCENSION_DEFAULT_NEWLINE,
		 *      NEWLINE_CHARACTERS
		 */
		enum Newline {
			/// Line feed. Standard of Unix (Lf, U+000A).
			NLF_LINE_FEED = 0,
			/// Carriage return. Old standard of Macintosh (Cr, U+000D).
			NLF_CARRIAGE_RETURN,
			/// CR+LF. Standard of Windows (CrLf, U+000D U+000A).
			NLF_CR_LF,
			/// Next line. Standard of EBCDIC-based OS (U+0085).
			NLF_NEXT_LINE,
			/// Line separator (U+2028).
			NLF_LINE_SEPARATOR,
			/// Paragraph separator (U+2029).
			NLF_PARAGRAPH_SEPARATOR,
			/// Represents any NLF as the actual newline of the line (@c kernel#Document#Line#newline()).
			NLF_RAW_VALUE,
			/// Represents any NLF as the value of @c kernel#IDocumentInput#newline().
			NLF_DOCUMENT_INPUT,
			NLF_COUNT
		};
		
		/**
		 * Returns the number of lines in the specified character sequence.
		 * This method is exception-neutral (does not throw if @a ForwardIterator does not).
		 * @tpatam ForwardIterator The type of @a first and @a last
		 * @param first The beginning of the character sequence
		 * @param last The end of the character sequence
		 * @return The number of lines. Zero if and only if the input sequence is empty
		 */
		template<typename ForwardIterator>
		inline Index calculateNumberOfLines(ForwardIterator first, ForwardIterator last) {
			if(first == last)
				return 0;
			Index lines = 1;
			while(true) {
				first = std::find_first_of(first, last, NEWLINE_CHARACTERS, ASCENSION_ENDOF(NEWLINE_CHARACTERS));
				if(first == last)
					break;
				++lines;
				if(*first == CARRIAGE_RETURN) {
					if(++first == last)
						break;
					else if(*first == LINE_FEED)
						++first;
				} else
					++first;
			}
			return lines;
		}

		/**
		 * Returns the number of lines in the specified text.
		 * @param text The text string
		 * @return The number of lines
		 */
		inline Index calculateNumberOfLines(const StringPiece& text) /*throw()*/ {
			return calculateNumberOfLines(text.beginning(), text.end());
		}

		/**
		 * Returns the newline at the beginning of the specified buffer.
		 * @tparam ForwardIterator The type of @a first and @a last
		 * @param first The beginning of the buffer
		 * @param last The end of the buffer
		 * @return The newline or @c NLF_RAW_VALUE if the beginning of the buffer is not line break
		 */
		template<typename ForwardIterator>
		inline Newline eatNewline(ForwardIterator first, ForwardIterator last) {
			switch(*first) {
			case LINE_FEED:
				return NLF_LINE_FEED;
			case CARRIAGE_RETURN:
				return (++first != last && *first == LINE_FEED) ? NLF_CR_LF : NLF_CARRIAGE_RETURN;
			case NEXT_LINE:
				return NLF_NEXT_LINE;
			case LINE_SEPARATOR:
				return NLF_LINE_SEPARATOR;
			case PARAGRAPH_SEPARATOR:
				return NLF_PARAGRAPH_SEPARATOR;
			default:
				return NLF_RAW_VALUE;
			}
		}

		/**
		 * Returns @c true if the given newline value is a literal.
		 * @throw UnknownValueException @a newline is invalid (undefined value)
		 */
		inline bool isLiteralNewline(Newline newline) /*throw()*/ {
#if NLF_LINE_FEED != 0 //|| NLF_COUNT != 8
#	error "Check the definition of Newline and revise this code."
#endif
			if(newline < NLF_LINE_FEED || newline >= NLF_COUNT)
				throw UnknownValueException("newline");
			return newline <= NLF_PARAGRAPH_SEPARATOR;
		}
		
		/**
		 * Returns the null-terminated string represents the specified newline.
		 * @param newline the newline
		 * @return The string
		 * @throw std#invalid_argument @a newline is not a literal value
		 * @throw UnknownValueException @a newline is undefined
		 * @see #newlineStringLength
		 */
		inline const Char* newlineString(Newline newline) {
			static const Char NEWLINE_STRINGS[] = {
				0x000du, 0, 0,
				0x000au, 0, 0,
				0x000du, 0x000au, 0,
				0x0085u, 0, 0,
				0x2028u, 0, 0,
				0x2029u, 0, 0
			};
			if(newline <= NLF_PARAGRAPH_SEPARATOR)
				return NEWLINE_STRINGS + newline * 3;
			else if(newline < NLF_COUNT)
				throw std::invalid_argument("newline");
			else
				throw UnknownValueException("newline");
		}
		
		/**
		 * Returns the length of the string represents the specified newline.
		 * @param newline the newline
		 * @return The length
		 * @throw std#invalid_argument @a newline is not a literal value
		 * @throw UnknownValueException @a newline is undefined
		 * @see #newlineString
		 */
		inline Index newlineStringLength(Newline newline) {
			switch(newline) {
			case NLF_LINE_FEED:
			case NLF_CARRIAGE_RETURN:
			case NLF_NEXT_LINE:
			case NLF_LINE_SEPARATOR:
			case NLF_PARAGRAPH_SEPARATOR:
				return 1;
			case NLF_CR_LF:
				return 2;
			default:
				if(newline < NLF_COUNT)
					throw std::invalid_argument("newline");
				else
					throw UnknownValueException("newline");
			}
		}

	}
} // namespace ascension.text

#endif // !ASCENSION_NEWLINE_HPP
