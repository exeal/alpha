/**
 * @file newline.cpp
 * Implements @c Newline class.
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2016
 * @date 2016-07-26 Separated from kernel/document.cpp.
 */

#include <ascension/corelib/text/newline.hpp>

namespace ascension {
	namespace text {
		/// Line feed. Standard of Unix (Lf, U+000A).
		const Newline Newline::LINE_FEED(text::LINE_FEED);

		/// Carriage return. Old standard of Macintosh (Cr, U+000D).
		const Newline Newline::CARRIAGE_RETURN(text::CARRIAGE_RETURN);

		/// CR+LF. Standard of Windows (CrLf, U+000D U+000A).
		const Newline Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED((text::CARRIAGE_RETURN << 16) | text::LINE_FEED);

		/// Next line. Standard of EBCDIC-based OS (U+0085).
		const Newline Newline::NEXT_LINE(text::NEXT_LINE);

		/// Line separator (U+2028).
		const Newline Newline::LINE_SEPARATOR(text::LINE_SEPARATOR);

		/// Paragraph separator (U+2029).
		const Newline Newline::PARAGRAPH_SEPARATOR(text::PARAGRAPH_SEPARATOR);

		/// Represents any NLF as the actual newline of the line (@c kernel#Document#Line#newline()).
		const Newline Newline::USE_INTRINSIC_VALUE(0x80000000u);

		/// Represents any NLF as the value of @c kernel#DocumentInput#newline().
		const Newline Newline::USE_DOCUMENT_INPUT(0x80000001u);

		/**
		 * @internal Private constructor.
		 * @param value
		 */
		Newline::Newline(std::uint32_t value) BOOST_NOEXCEPT : value_(value) {
		}
	}
}
