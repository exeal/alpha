/**
 * @file glyph-metrics.hpp
 * @author exeal
 * @date 2010-11-06 created as font.hpp
 * @date 2010-2012 was font.hpp
 * @date 2012-08-26 separated from font.hpp
 * @date 2012-09-16 separated from text-run.hpp
 * @date 2012-2013 was glyph-vector.hpp
 * @date 2013-03-28 separated from glyph-vector.hpp
 */

#ifndef ASCENSION_GLYPH_METRICS_HPP
#define ASCENSION_GLYPH_METRICS_HPP

#include <ascension/graphics/geometry.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Represents information for a single glyph. All coordinates used in this class are in user units.
			 * @see GlyphVector#glyphMetrics
			 */
			class GlyphMetrics {
			public:
				enum Type {};

				/**
				 * Constructs a @c GlyphMetrics object for the horizontal glyph.
				 * @param advance The advance width of the glyph
				 * @param bounds The black box bounds of the glyph
				 * @param type The type of the glyph
				 */
				GlyphMetrics(Scalar advance, const Rectangle& bounds, Type type) : horizontal_(true),
					advances_(geometry::_dx = advance, geometry::_dy = static_cast<Scalar>(0)), bounds_(bounds), type_(type) {}
				/**
				 * Constructs a @c GlyphMetrics object.
				 * @param horizontal If @c true, metrics are for a horizontal baseline, otherwise they are for a vertical baseline
				 * @param advances The x- and y-component of the glyph's advance
				 * @param bounds The visual bounds of the glyph
				 * @param type The type of the glyph
				 */
				GlyphMetrics(bool horizontal, const Dimension& advances, const Rectangle& bounds, Type type) :
					horizontal_(horizontal), advances_(advances), bounds_(bounds), type_(type) {}

				/// Returns the advance of the glyph along the baseline (either horizontal or vertical).
				Scalar advance() const BOOST_NOEXCEPT {
					return horizontal_ ? advanceX() : advanceY();
				}
				/// Returns the x-component of the advance of the glyph.
				Scalar advanceX() const BOOST_NOEXCEPT {
					return geometry::dx(advances_);
				}
				/// Returns the y-component of the advance of the glyph.
				Scalar advanceY() const BOOST_NOEXCEPT {
					return geometry::dy(advances_);
				}
				/// Returns the bounding box of the glyph outline.
				const Rectangle& bounds() const BOOST_NOEXCEPT {
					return bounds_;
				}
				/// Returns the left (top) side bearing of the glyph.
				Scalar leftTopSideBearing() const BOOST_NOEXCEPT {
					return horizontal_ ? geometry::left(bounds()) : geometry::top(bounds());
				}
				/// Returns the left (top) side bearing of the glyph.
				Scalar rightBottomSideBearing() const BOOST_NOEXCEPT {
					return horizontal_ ?
						(advanceX() - geometry::right(bounds())) : (advanceY() - geometry::bottom(bounds()));
				}
				/// Returns the type of the glyph.
				Type type() const BOOST_NOEXCEPT {
					return type_;
				}
			private:
				const bool horizontal_;
				const Dimension advances_;
				const Rectangle bounds_;
				const Type type_;
			};
		}
	}
}

#endif // !ASCENSION_GLYPH_METRICS_HPP
