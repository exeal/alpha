/**
 * @file tab-expander.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010, 2014
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012 was text-layout.hpp
 * @date 2012-08-17 separated from text-layout.hpp
 * @date 2014-10-13 Separated from text-layout-styles.hpp
 */

#ifndef ASCENSION_TAB_EXPANDER_HPP
#define ASCENSION_TAB_EXPANDER_HPP

#include <ascension/graphics/geometry/common.hpp>
#include <cmath>
#include <type_traits>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Interface of object which implements tab expansion.
			 * @tparam Length
			 * @see TextLayout#TextLayout
			 * @note This interface is designed based on @c TabExpander interface of Java.
			 */
			template<typename Length = Scalar>
			class TabExpander {
			public:
				/// Destructor.
				virtual ~TabExpander() BOOST_NOEXCEPT {}
				/**
				 * Returns the next tab stop position given a reference position. Values are expressed in @c Length.
				 * @param ipd The position in @c Length
				 * @param tabOffset The position within the underlying text that the tab occured
				 * @return The next tab stop. Should be greater than @a ipd
				 */
				virtual Length nextTabStop(Length ipd, Index tabOffset) const = 0;
			};

			/**
			 * Standard implementation of @c TabExpander with fixed width tabulations.
			 * @tparam Length See the description of @c TabExpander
			 */
			template<typename Length>
			class FixedWidthTabExpander : public TabExpander<Length> {
			public:
				/**
				 * Constructor.
				 * @param width The fixed width in @c Length
				 * @throw std#invalid_argument @a width is zero
				 */
				explicit FixedWidthTabExpander(Length width) : width_(width) {
					if(width == 0)
						throw std::invalid_argument("width");
				}
				/// @see TabExpander#nextTabStop
				Length nextTabStop(Length ipd, Index tabOffset) const override {
					return impl(ipd, tabOffset);
				}

			private:
				template<typename T>
				Length impl(T ipd, Index tabOffset, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr) const BOOST_NOEXCEPT {
					return ipd - ipd % width_ + width_;
				}
				template<typename T>
				Length impl(T ipd, Index tabOffset, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr) const BOOST_NOEXCEPT {
					return ipd - std::fmod(ipd, width_) + width_;
				}
			private:
				const Length width_;
			};
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TAB_EXPANDER_HPP
