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

#include <ascension/presentation/absolute-length.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * @see TextLayout#TextLayout
			 * @note This interface is designed based on @c TabExpander interface of Java.
			 */
			class TabExpander {
			public:
				/// Destructor.
				virtual ~TabExpander() BOOST_NOEXCEPT {}
				/**
				 * Returns the next tab stop position given a reference position.
				 * @param ipd The position in pixels
				 * @param tabOffset The position within the underlying text that the tab occured
				 * @return The next tab stop. Should be greater than @a ipd
				 */
				virtual presentation::Pixels nextTabStop(const presentation::Pixels& ipd, Index tabOffset) const = 0;
			};

			/// Standard implementation of @c TabExpander with fixed width tabulations.
			class FixedWidthTabExpander : public TabExpander {
			public:
				/**
				 * Constructor.
				 * @param width The fixed width in pixels
				 */
				explicit FixedWidthTabExpander(const presentation::Pixels& width) BOOST_NOEXCEPT : width_(width) {}
				/// @see TabExpander#nextTabStop
				presentation::Pixels nextTabStop(const presentation::Pixels& ipd, Index tabOffset) const {
					return presentation::Pixels(ipd.value() - static_cast<long>(ipd.value()) % static_cast<long>(width_.value()) + width_.value());
				}

			private:
				const presentation::Pixels width_;
			};
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TAB_EXPANDER_HPP
