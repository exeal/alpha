/**
 * @file glyph-vector.hpp
 * @author exeal
 * @date 2010-11-06 created as font.hpp
 * @date 2010-2012 was font.hpp
 * @date 2012-08-26 separated from font.hpp
 * @date 2012-09-16 separated from text-run.hpp
 */

#ifndef ASCENSION_GLYPH_VECTOR_HPP
#define ASCENSION_GLYPH_VECTOR_HPP

#include <ascension/graphics/geometry.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <cstdint>
#include <memory>
#include <boost/optional.hpp>

namespace ascension {
	namespace graphics {

		class PaintContext;

		namespace font {
			/// 16-bit glyph index value.
			typedef std::uint16_t GlyphCode;

			/**
			 * Represents information for a single glyph.
			 * @see GlyphVector#metrics
			 */
			class GlyphMetrics {
			public:
				virtual Scalar advanceX() const = 0;
				virtual Scalar advanceY() const = 0;
				virtual NativeSize bounds() const = 0;
				virtual Scalar leftTopSideBearing() const = 0;
				virtual Scalar rightBottomSideBearing() const = 0;
			};

			class Font;

			/**
			 * Abstract class represents a vector of glyph codes with geometric information.
			 * @see Font#createGlyphVector, TextRun, TextLayout
			 */
			class GlyphVector {
			public:
				/// Destructor.
				virtual ~GlyphVector() BOOST_NOEXCEPT {}
				virtual presentation::ReadingDirection direction() const BOOST_NOEXCEPT = 0;
				/**
				 * @see #strokeGlyphs
				 */
				virtual void fillGlyphs(PaintContext& context, const NativePoint& origin,
					boost::optional<Range<std::size_t>> range = boost::none) const = 0;
				/// Returns the font associated with this vector.
				virtual std::shared_ptr<const Font> font() const = 0;
//				virtual GlyphCode glyphCode(std::size_t index) const = 0;
//				virtual GlyphMetrics&& glyphMetrics(std::size_t index) const = 0;
				/**
				 * Returns the logical bounds of the specified glyphs within this vector.
				 * @param range The range of the glyphs
				 * @return The logical bounds
				 * @throw std#out_of_range @a range is invalid
				 * @see #glyphVisualBounds, #logicalBounds
				 */
				virtual presentation::FlowRelativeFourSides<Scalar> glyphLogicalBounds(const Range<std::size_t>& range) const;
				/**
				 * Returns the position of the specified glyph.
				 * @param index The glyph index in this vector
				 * @return The position of the specified glyph relative to the leading edge of this
				 *         vector. If this equals @c size(), this method returns the position of
				 *         the trailing edge of the last glyph
				 * @throw std#out_of_range @a index &gt; @c size()
				 */
//				virtual Scalar glyphPosition(std::size_t index) const = 0;	// TODO: useless???
				/**
				 * Returns the visual bounds of the specified glyphs within this vector.
				 * @param range The range of the glyphs
				 * @return The visual bounds
				 * @throw std#out_of_range @a range is invalid
				 * @see #glyphLogicalBounds, #visualBounds
				 */
				virtual presentation::FlowRelativeFourSides<Scalar> glyphVisualBounds(const Range<std::size_t>& range) const = 0;
				/**
				 * Returns the logical bounds of this vector.
				 * @see #glyphLogicalBounds, #visualBounds
				 */
				virtual presentation::FlowRelativeFourSides<Scalar> logicalBounds() const {
					return glyphLogicalBounds(makeRange<size_t>(0, numberOfGlyphs()));
				}
				template<typename T>
				NativeRectangle mapLogicalToPhysical(
						const presentation::FlowRelativeFourSides<T>& logical,
						presentation::BlockFlowDirection blockFlowDirection,
						presentation::TextOrientation textOrientation) const {
					PhysicalFourSides<Scalar> physical;
					presentation::mapFlowRelativeToPhysical(presentation::WritingMode(
						direction(), blockFlowDirection, textOrientation), logical, physical);
					return geometry::make<NativeRectangle>(physical);
				}
				/// Returns the number of glyphs in this vector.
				virtual std::size_t numberOfGlyphs() const BOOST_NOEXCEPT = 0;
//				virtual std::shared_ptr<Shape> outline(std::size_t index) const = 0;
				/**
				 * @see #fillGlyphs
				 */
				virtual void strokeGlyphs(PaintContext& context, const NativePoint& origin,
					boost::optional<Range<std::size_t>> range = boost::none) const = 0;
				/**
				 * Returns the visual bounds of this vector.
				 * @return The visual bounds
				 * @see #glyphVisualBounds, #logicalBounds
				 */
				virtual presentation::FlowRelativeFourSides<Scalar> visualBounds() const {
					return glyphVisualBounds(makeRange<size_t>(0, numberOfGlyphs()));
				}
			};

		}
	}
}

#endif // !ASCENSION_GLYPH_VECTOR_HPP
