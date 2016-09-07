/**
 * @file locations.hpp
 * Defines @c ascension#kernel#locations namespace.
 * @author exeal
 * @see viewer/locations.hpp
 * @date 2003-2015 Was document.hpp.
 * @date 2016-05-22 Separated from document.hpp.
 */

#ifndef ASCENSION_LOCATIONS_HPP
#define ASCENSION_LOCATIONS_HPP
#include <ascension/direction.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <ascension/kernel/position.hpp>
#include <utility>	// std.pair

namespace ascension {
	namespace kernel {
		class Document;

		/**
		 * Provides several functions related to locations in document.
		 * Many functions in this namespace take a @c std#pair&lt;const Document&amp;, const Position&amp;&gt; which
		 * describes a position in the document. @c Point and @c viewer#VisualPoint classes have conversion operators
		 * into this type.
		 * @note All functions are *affected* by accessible region of the document.
		 * @see viewer#locations
		 */
		namespace locations {
			/// Character unit defines what is one character.
			enum CharacterUnit {
				UTF16_CODE_UNIT,	///< UTF-16 code unit.
				UTF32_CODE_UNIT,	///< UTF-32 code unit. A surrogate pair is treated as one character.
				GRAPHEME_CLUSTER,	///< A grapheme cluster is a character.
				GLYPH_CLUSTER		///< A glyph is a character (not implemented).
			};

			/// Describes a position in the document.
			typedef std::pair<const Document&, Position> PointProxy;

			Position beginningOfDocument(const PointProxy& p);
			Position beginningOfLine(const PointProxy& p);
			CodePoint characterAt(const PointProxy& p, bool useLineFeed = false);
			Position endOfDocument(const PointProxy& p);
			Position endOfLine(const PointProxy& p);
			bool isBeginningOfDocument(const PointProxy& p);
			bool isBeginningOfLine(const PointProxy& p);
			bool isEndOfDocument(const PointProxy& p);
			bool isEndOfLine(const PointProxy& p);
			boost::optional<Position> nextBookmark(const PointProxy& p, Direction direction, Index marks = 1);
			Position nextCharacter(const PointProxy& p,
				Direction direction, CharacterUnit characterUnit, Index offset = 1);
			Position nextLine(const PointProxy& p, Direction direction, Index lines = 1);
			Position nextWord(const PointProxy& p, Direction direction, Index words = 1);
			Position nextWordEnd(const PointProxy& p, Direction direction, Index words = 1);
		}
	}
}

#endif // !ASCENSION_LOCATIONS_HPP
