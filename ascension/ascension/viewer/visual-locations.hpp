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
#include <ascension/viewer/visual-destination-proxy.hpp>

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
			typedef std::pair<const TextArea&, kernel::Position> PointProxy;

			bool isEndOfVisualLine(const PointProxy& p);
			bool isFirstPrintableCharacterOfLine(const PointProxy& p);
			bool isFirstPrintableCharacterOfVisualLine(const PointProxy& p);
			bool isLastPrintableCharacterOfLine(const PointProxy& p);
			bool isLastPrintableCharacterOfVisualLine(const PointProxy& p);
			bool isBeginningOfVisualLine(const PointProxy& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy backwardPage(const PointProxy& p, Index pages = 1);
			viewer::VisualDestinationProxy backwardVisualLine(const PointProxy& p, Index lines = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			kernel::Position beginningOfVisualLine(const PointProxy& p);
			kernel::Position contextualBeginningOfLine(const PointProxy& p);
			kernel::Position contextualBeginningOfVisualLine(const PointProxy& p);
			kernel::Position contextualEndOfLine(const PointProxy& p);
			kernel::Position contextualEndOfVisualLine(const PointProxy& p);
			kernel::Position endOfVisualLine(const PointProxy& p);
			kernel::Position firstPrintableCharacterOfLine(const PointProxy& p);
			kernel::Position firstPrintableCharacterOfVisualLine(const PointProxy& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy forwardPage(const PointProxy& p, Index pages = 1);
			viewer::VisualDestinationProxy forwardVisualLine(const PointProxy& p, Index lines = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			kernel::Position lastPrintableCharacterOfLine(const PointProxy& p);
			kernel::Position lastPrintableCharacterOfVisualLine(const PointProxy& p);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy leftCharacter(
				const PointProxy& p, CharacterUnit unit, Index characters = 1);
			boost::optional<kernel::Position> leftWord(const PointProxy& p, Index words = 1);
			boost::optional<kernel::Position> leftWordEnd(const PointProxy& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			viewer::VisualDestinationProxy rightCharacter(
				const VisualPoint& p, CharacterUnit unit, Index characters = 1);
			boost::optional<Position> rightWord(const PointProxy& p, Index words = 1);
			boost::optional<Position> rightWordEnd(const PointProxy& p, Index words = 1);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
		}
	}
}

#endif // !ASCENSION_VISUAL_LOCATIONS_HPP
