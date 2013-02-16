/**
 * @file font-metrics.hpp
 * @author exeal
 * @date 2010-11-06 created as font.hpp
 * @date 2010-2013 was font.hpp
 * @date 2013-01-14 separated from font.hpp
 */

#ifndef ASCENSION_FONT_METRICS_HPP
#define ASCENSION_FONT_METRICS_HPP
#include <ascension/corelib/basic-types.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Abstract class provides physical font metrics information.
			 * @tparam T The numeric type represents the unit
			 * @see Font#metrics
			 */
			template<typename T>
			class FontMetrics {
			public:
				/// The unit type.
				typedef T Unit;
				/// Returns the ascent of the text.
				virtual Unit ascent() const BOOST_NOEXCEPT = 0;
				/// Returns the average width (advance) of a character.
				virtual Unit averageCharacterWidth() const BOOST_NOEXCEPT = 0;
				/// Returns the cell height.
				Unit cellHeight() const BOOST_NOEXCEPT {return ascent() + descent();}
				/// Returns the descent of the text.
				virtual Unit descent() const BOOST_NOEXCEPT = 0;
				/// Returns the em height.
				Unit emHeight() const BOOST_NOEXCEPT {return cellHeight() - internalLeading();}
				/// Returns the external leading.
				virtual Unit externalLeading() const BOOST_NOEXCEPT = 0;
				/// Returns the internal leading.
				virtual Unit internalLeading() const BOOST_NOEXCEPT = 0;
				/// Returns the gap of the lines (external leading).
				Unit lineGap() const BOOST_NOEXCEPT {return externalLeading();}
				/// Returns the pitch of lines.
				Unit linePitch() const BOOST_NOEXCEPT {return cellHeight() + lineGap();}
				/// Returns the units-per-em.
				virtual std::uint16_t unitsPerEm() const BOOST_NOEXCEPT;
				/// Returns the x-height of the font.
				virtual Unit xHeight() const BOOST_NOEXCEPT = 0;
			};
		}
	}
}

#endif // ASCENSION_FONT_METRICS_HPP