/**
 * @file locations.hpp
 * Defines @c ascension#kernel#locations namespace.
 * @author exeal
 * @see viewer/visual-locations.hpp
 * @date 2003-2015 Was document.hpp.
 * @date 2016-05-22 Separated from document.hpp.
 */

#ifndef ASCENSION_LOCATIONS_HPP
#define ASCENSION_LOCATIONS_HPP
#include <ascension/direction.hpp>
#include <ascension/corelib/text/code-point.hpp>
#include <ascension/kernel/region.hpp>
#include <utility>	// std.pair

namespace ascension {
	namespace kernel {
		class Document;
		class DocumentChange;

		/**
		 * Provides several functions related to locations in document.
		 * Many functions in this namespace take a @c PointProxy which describes a position in the document. @c Point
		 * and @c viewer#VisualPoint classes have conversion operators into this type.
		 * @note All functions are *affected* by accessible region of the document.
		 * @see viewer#locations
		 */
		namespace locations {
			/// Character unit defines what is one character.
			enum CharacterUnit {
				UTF16_CODE_UNIT,	///< UTF-16 code unit. A surrogate pair is treated as two characters.
				UTF32_CODE_UNIT,	///< UTF-32 code unit. A surrogate pair is treated as one character.
				GRAPHEME_CLUSTER,	///< A grapheme cluster is a character.
				GLYPH_CLUSTER		///< A glyph is a character (not implemented).
			};

			struct PointProxy;

			/// @defgroup special_locations_in_document Special Locations in Document
			/// Free functions related to special locations.
			/// @note All functions are *affected* by accessible region of the document.
			/// @{
			Position beginningOfDocument(const PointProxy& p);
			Position beginningOfLine(const PointProxy& p);
			Position endOfDocument(const PointProxy& p);
			Position endOfLine(const PointProxy& p);
			bool isBeginningOfDocument(const PointProxy& p);
			bool isBeginningOfLine(const PointProxy& p);
			bool isEndOfDocument(const PointProxy& p);
			bool isEndOfLine(const PointProxy& p);
			/// @}

			/// @defgroup motions_in_document Motions in Document
			/// @note All functions are *affected* by accessible region of the document.
			/// @{
			boost::optional<Position> nextBookmark(const PointProxy& p, Direction direction, Index marks = 1);
			Position nextCharacter(const PointProxy& p, Direction direction, CharacterUnit characterUnit, Index offset = 1);
			Position nextLine(const PointProxy& p, Direction direction, Index lines = 1);
			Position nextWord(const PointProxy& p, Direction direction, Index words = 1);
			Position nextWordEnd(const PointProxy& p, Direction direction, Index words = 1);
			/// @}

			/// @defgroup regions_of_document Regions of Document
			/// Free functions related to document's region.
			/// @{
			bool isOutsideOfDocumentRegion(const PointProxy& p) BOOST_NOEXCEPT;
			Position shrinkToDocumentRegion(const PointProxy& p) BOOST_NOEXCEPT;
			Region shrinkToDocumentRegion(const Document& document, const Region& region) BOOST_NOEXCEPT;
			/// @}

			/// @defgroup accessible_regions_of_document Accessible Regions of Document
			/// Free functions related to document's accessible region.
			/// @{
			bool isOutsideOfAccessibleRegion(const PointProxy& p) BOOST_NOEXCEPT;
			Position shrinkToAccessibleRegion(const PointProxy& p) BOOST_NOEXCEPT;
			Region shrinkToAccessibleRegion(const Document& document, const Region& region) BOOST_NOEXCEPT;
			/// @}

			/// @defgroup miscellaneous_locational_functions Miscellaneous Locational Functions
			/// @{
			Index absoluteOffset(const PointProxy& p, bool fromAccessibleStart);
			Position updatePosition(const Position& position, const DocumentChange& change, Direction gravity) BOOST_NOEXCEPT;
			/// @}

			namespace detail {
				Position updatePositionForDeletion(const Position& position, const Region& region, Direction gravity) BOOST_NOEXCEPT;
				Position updatePositionForInsertion(const Position& position, const Region& region, Direction gravity) BOOST_NOEXCEPT;
			}
		}
	}
}

#endif // !ASCENSION_LOCATIONS_HPP
