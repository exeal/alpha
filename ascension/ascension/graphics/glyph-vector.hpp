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

#include <ascension/graphics/font-render-context.hpp>
#include <ascension/graphics/geometry.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <cstdint>
#include <memory>
#include <boost/flyweight.hpp>
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

			/**
			 * Abstract class represents a vector of glyph codes with geometric information. All
			 * geometric coordinates are in user space units. 
			 * @see Font#createGlyphVector, TextRun, TextLayout
			 */
			class GlyphVector {
			public:
				/// Destructor.
				virtual ~GlyphVector() BOOST_NOEXCEPT {}
				virtual presentation::ReadingDirection direction() const BOOST_NOEXCEPT = 0;

				/// @name Glyph Codes
				/// @{
				/**
				 * Returns the glyphcode of the specified glyph.
				 * @param index The index in this vector
				 * @return The glyphcode
				 * @throw std#out_of_range @a index &gt; @c #numberOfGlyphs()
				 * @see #glyphCodes
				 */
				virtual GlyphCode glyphCode(std::size_t index) const = 0;
//				virtual std::vector<GlyphCode>&& glyphCodes(const Range<std::size_t>& range) const = 0;
				/// Returns the number of glyphs in this vector.
				virtual std::size_t numberOfGlyphs() const BOOST_NOEXCEPT = 0;
				/// @}

				/// @name Attributes
				/// @{
				/// Returns the @c Font associated with this vector.
				virtual std::shared_ptr<const Font> font() const = 0;
				/// Returns the @c FontRenderContext with this vector.
				virtual const FontRenderContext& fontRenderContext() const = 0;
				/**
				 * Returns the character index of the specified glyph, which is the index of the
				 * first logical character represented by the glyph.
				 * @param index The index in this vector
				 * @return The character index
				 * @throw std#out_of_range @a index &gt; @c #numberOfGlyphs()
				 * @see #glyphCharacterIndices
				 */
				virtual Index glyphCharacterIndex(std::size_t index) const = 0;
//				virtual std::vector<Index>&& glyphCharacterIndices(const Range<std::size_t>& range) const = 0;
				/// @}

				/// @name Glyph Position
				/// @{
				/**
				 * Returns the position of the specified glyph.
				 * @param index The glyph index in this vector
				 * @return The position of the specified glyph relative to the origin of this
				 *         vector, in user units. If @a index equals @c #numberOfGlyphs(), this
				 *         method returns the position of the end of the last glyph
				 * @throw std#out_of_range @a index &gt; @c #numberOfGlyphs()
				 * @see #setGlyphPosition
				 */
				virtual presentation::AbstractTwoAxes<double> glyphPosition(std::size_t index) const = 0;	// TODO: useless???
//				virtual std::vector<presentation::AbstractTwoAxes<double>>&& glyphPositions(const Range<std::size_t>& range) const = 0;
				/**
				 * Sets the position of the specified glyph within this vector.
				 * @param index The glyph index in this vector
				 * @param position The position relative to the origin of this vector in user units
				 * @throw std#out_of_range @a index &gt; @c #numberOfGlyphs()
				 * @see #glyphPosition
				 */
				virtual void setGlyphPosition(std::size_t index, const presentation::AbstractTwoAxes<double>& position) = 0;
				/// @}

				/// @name Logical, Visual and Pixel Bounds
				/// @{
				/**
				 * Returns the logical bounds of the specified glyphs within this vector in user units.
				 * @param index The glyph index in this vector
				 * @return The logical bounds of the glyph
				 * @throw std#out_of_range @a index &gt; @c #numberOfGlyphs()
				 * @see #glyphPixelBounds, #glyphVisualBounds, #logicalBounds
				 */
				virtual presentation::FlowRelativeFourSides<double> glyphLogicalBounds(std::size_t index) const = 0;
//				virtual NativeRectangle glyphPixelBounds(std::size_t index, const FontRenderContext& frc, const NativePoint& at) const = 0;
				/**
				 * Returns the visual bounds of the specified glyphs within this vector in user units.
				 * @param index The glyph index in this vector
				 * @return The visual bounds of the glyph
				 * @throw std#out_of_range @a index &gt; @c #numberOfGlyphs()
				 * @see #glyphLogicalBounds, #glyphPixelBounds, #visualBounds
				 */
				virtual presentation::FlowRelativeFourSides<double> glyphVisualBounds(const Range<std::size_t>& range) const = 0;
				/**
				 * Returns the logical bounds of this vector in user units.
				 * @see #glyphLogicalBounds, #pixelBounds, #visualBounds
				 */
				virtual presentation::FlowRelativeFourSides<double> logicalBounds() const = 0;
//				virtual NativeRectangle pixelBounds(const FontRenderContext& frc, const NativePoint& at) const = 0;
				/**
				 * Returns the visual bounds of this vector in user units.
				 * @see #glyphVisualBounds, #logicalBounds, #pixelBounds
				 */
				virtual presentation::FlowRelativeFourSides<double> visualBounds() const = 0;
				/// @}

				/// @name Glyph Metrics
				/// @{
//				virtual GlyphMetrics&& glyphMetrics(std::size_t index) const = 0;
//				virtual NativeShape&& glyphOutline(std::size_t index) const = 0;
//				virtual NativeShape&& outline() const = 0;
				/// @}

				/// @name Glyph Transform
				/// @{
//				virtual NativeAffineTransform&& glyphTransform(std::size_t index) const = 0;
//				virtual void setGlyphTransform(std::size_t index, const NativeAffineTransform& tx) = 0;
				/// @}

				/// @name Painting
				/// @{
				/**
				 * @see #strokeGlyphs
				 */
				virtual void fillGlyphs(PaintContext& context, const NativePoint& origin,
					boost::optional<Range<std::size_t>> range = boost::none) const = 0;
				/**
				 * @see #fillGlyphs
				 */
				virtual void strokeGlyphs(PaintContext& context, const NativePoint& origin,
					boost::optional<Range<std::size_t>> range = boost::none) const = 0;
				/// @}

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
			};

		}
	}
}

#endif // !ASCENSION_GLYPH_VECTOR_HPP
