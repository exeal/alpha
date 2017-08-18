/**
 * @file visual-locations.hpp
 * Defines @c ascension#viewer#locations namespace.
 * @author exeal
 * @see kernel/locations.hpp
 * @date 2003-2008 was point.hpp
 * @date 2008-xx-xx Separated from point.hpp.
 * @date 2011-10-02 Separated from caret.hpp.
 * @date 2016-05-22 Separated from visual-point.hpp.
 */

#ifndef ASCENSION_VISUAL_LOCATIONS_HPP
#define ASCENSION_VISUAL_LOCATIONS_HPP
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
#	include <ascension/viewer/visual-destination-proxy.hpp>
#else
#	include <ascension/viewer/text-hit.hpp>
#endif

namespace ascension {
	namespace viewer {
		class TextArea;

		/**
		 * Provides several functions related to locations in text area.
		 * Many functions in this namespace take a @c std#pair&lt;const TextArea&amp;, const kernel#Position&amp;&gt;
		 * which describes a position in the text area. @c viewer#VisualPoint class has conversion operators into this
		 * type.
		 * @note All functions are *affected* by accessible region of the document.
		 * @see kernel#locations
		 */
		namespace locations {
			/**
			 * Describes a position in the @c TextArea.
			 * @see kernel#locations#PointProxy
			 */
			typedef std::pair<const TextArea&, TextHit> PointProxy;

			/// @defgroup special_locations_in_text_area Special Locations in Text Area
			/// Free functions related to special locations in @c TextArea.
			/// @note All functions are *affected* by accessible region of the document.
			/// @{
			TextHit beginningOfVisualLine(const PointProxy& p);
			TextHit contextualBeginningOfLine(const PointProxy& p);
			TextHit contextualBeginningOfVisualLine(const PointProxy& p);
			TextHit contextualEndOfLine(const PointProxy& p);
			TextHit contextualEndOfVisualLine(const PointProxy& p);
			TextHit endOfVisualLine(const PointProxy& p);
			TextHit firstPrintableCharacterOfLine(const PointProxy& p);
			TextHit firstPrintableCharacterOfVisualLine(const PointProxy& p);
			bool isEndOfVisualLine(const PointProxy& p);
			bool isFirstPrintableCharacterOfLine(const PointProxy& p);
			bool isFirstPrintableCharacterOfVisualLine(const PointProxy& p);
			bool isLastPrintableCharacterOfLine(const PointProxy& p);
			bool isLastPrintableCharacterOfVisualLine(const PointProxy& p);
			bool isBeginningOfVisualLine(const PointProxy& p);
			TextHit lastPrintableCharacterOfLine(const PointProxy& p);
			TextHit lastPrintableCharacterOfVisualLine(const PointProxy& p);
			/// @}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/// @defgroup motions_in_text_area Motions in Text Area
			/// @note All functions are *affected* by accessible region of the document.
			/// @{
			viewer::VisualDestinationProxy backwardPage(const PointProxy& p, Index pages = 1);
			viewer::VisualDestinationProxy backwardVisualLine(const PointProxy& p, Index lines = 1);
			viewer::VisualDestinationProxy forwardPage(const PointProxy& p, Index pages = 1);
			viewer::VisualDestinationProxy forwardVisualLine(const PointProxy& p, Index lines = 1);
			viewer::VisualDestinationProxy leftCharacter(
				const PointProxy& p, CharacterUnit unit, Index characters = 1);
			boost::optional<TextHit> leftWord(const PointProxy& p, Index words = 1);
			boost::optional<TextHit> leftWordEnd(const PointProxy& p, Index words = 1);
			viewer::VisualDestinationProxy rightCharacter(
				const VisualPoint& p, CharacterUnit unit, Index characters = 1);
			boost::optional<Position> rightWord(const PointProxy& p, Index words = 1);
			boost::optional<Position> rightWordEnd(const PointProxy& p, Index words = 1);
			/// @}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/// @defgroup miscellaneous_visual_locational_functions Miscellaneous Visual Locational Functions
			/// @{
			TextHit updateTextHit(const TextHit& position, const kernel::DocumentChange& change, Direction gravity) BOOST_NOEXCEPT;
			/// @}
		}
	}
}

#endif // !ASCENSION_VISUAL_LOCATIONS_HPP
