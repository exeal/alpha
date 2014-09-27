/**
 * @file text-style.hpp
 * @author exeal
 * @see presentation.hpp, graphics/text-alignment.hpp, graphics/text-layout-styles.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014
 */

#ifndef ASCENSION_TEXT_STYLE_HPP
#define ASCENSION_TEXT_STYLE_HPP

//#include <ascension/corelib/string-piece.hpp>
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/directions.hpp>
#include <ascension/presentation/styles/auxiliary.hpp>
#include <ascension/presentation/styles/background.hpp>
#include <ascension/presentation/styles/box.hpp>
#include <ascension/presentation/styles/color.hpp>
#include <ascension/presentation/styles/fonts.hpp>
#include <ascension/presentation/styles/inline.hpp>
#include <ascension/presentation/styles/text.hpp>
#include <ascension/presentation/styles/text-decor.hpp>
#include <ascension/presentation/styles/writing-modes.hpp>
#include <memory>
#include <boost/optional.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	/// Defines presentative stuffs.
	namespace presentation {
#if 0
		/**
		 * @overload
		 * @tparam InheritedOrNotForCurrentColor The template parameter for @a current
		 * @tparam InheritedOrNotForParentColor The template parameter for @a parent
		 * @param current The declared color property of the current element
		 * @param parent The declared color property of the parent element
		 * @param ancestor The top-level style provides the color property
		 * @return A computed color value
		 * @ingroup css3_color
		 */
		template<typename InheritedOrNotForCurrentColor, typename InheritedOrNotForParentColor>
		inline graphics::Color computeColor(const ColorProperty<InheritedOrNotForCurrentColor>* current,
				const ColorProperty<InheritedOrNotForParentColor>* parent, const TextToplevelStyle& ancestor) {
			const ColorProperty<sp::Inherited>* ancestorColor = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorColor = &ancestor.defaultLineStyle->defaultRunStyle->color;
			return computeColor(current, parent, (ancestorColor != nullptr) ? *ancestorColor : ColorProperty<sp::Inherited>());
		}

		/**
		 * @overload
		 * @param current The declared color property of the current element
		 * @param parent The declared color property of the parent element
		 * @param ancestor The top-level style provides the background property
		 * @return A computed background value as @c Paint
		 * @ingroup css3_background
		 */
		inline std::unique_ptr<graphics::Paint> computeBackground(
				const Background* current, const Background* parent, const TextToplevelStyle& ancestor) {
			const Background* ancestorBackground = nullptr;
			if(ancestor.defaultLineStyle && ancestor.defaultLineStyle->defaultRunStyle)
				ancestorBackground = &ancestor.defaultLineStyle->defaultRunStyle->background;
			Background inheritedBackground;
			inheritedBackground.color.inherit();
			return computeBackground(current, parent, (ancestorBackground != nullptr) ? *ancestorBackground : inheritedBackground);
		}
#endif
	}
} // namespace ascension.presentation

#endif // !ASCENSION_TEXT_STYLE_HPP
