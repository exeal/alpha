/**
 * @file ruler-locator.hpp
 * Defines RulerLocator interface.
 * @author exeal
 * @date 2015-03-07 Created.
 */

#ifndef ASCENSION_RULER_LOCATOR_HPP
#define ASCENSION_RULER_LOCATOR_HPP
#include <ascension/graphics/geometry/rectangle.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			class Ruler;

			struct RulerLocator {
				/**
				 * Returns the allocation-rectangle of the specified ruler, in viewer-local coordinates.
				 * @param ruler The ruler to locate
				 * @return The allocation-rectangle of @a ruler
				 * @throw std#invalid_argument @a ruler was not found
				 */
				virtual graphics::Rectangle locateRuler(const Ruler& ruler) const = 0;
			};
		}
	}
}

#endif // !ASCENSION_RULER_LOCATOR_HPP
