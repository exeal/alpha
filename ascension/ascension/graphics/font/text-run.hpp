/**
 * @file text-run.hpp
 * @see text-layout.hpp
 * @author exeal
 * @date 2010-11-06 created as font.hpp
 * @date 2010-2012 was font.hpp
 * @date 2012-08-26 separated from font.hpp
 */

#ifndef ASCENSION_TEXT_RUN_HPP
#define ASCENSION_TEXT_RUN_HPP
#include <ascension/graphics/font/glyph-vector.hpp>
#include <ascension/graphics/font/text-hit.hpp>
#include <ascension/presentation/flow-relative-directions-dimensions.hpp>
#include <boost/geometry/algorithms/distance.hpp>
#include <boost/optional.hpp>
#if ASCENSION_SELECTS_SHAPING_ENGINE(CORE_TEXT)
#	include <CTRun.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
#	include <dwrite.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(HARF_BUZZ)
#	include <hb.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
#	include <pangomm.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(QT)
#	include <QGlyphRun>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE)
#	include <usp10.h>
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace graphics {
		class RenderingContext2D;

		namespace font {
			struct ActualBorderSide;

			/**
			 * Abstract class represents a minimum text run whose characters can shaped by single font and has single
			 * text reading direction.
			 * @see GlyphVector, TextLayout
			 */
			class TextRun : public GlyphVector {
			public:
				/// Destructor.
				virtual ~TextRun() BOOST_NOEXCEPT {}
				/// Returns the character range this text run represents.
				virtual StringPiece characterRange() const BOOST_NOEXCEPT = 0;

				/// @name Hit Test
				/// @{
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				/**
				 * Returns the character encompasses the specified location.
				 * @param ipd The distance to the location from the leading edge of this text run in
				 *        inline-progression-dimension, in user units
				 * @return The character index in this run, or @c boost#none if @a ipd is outside of this text run
				 * @see #characterHasClosestLeadingEdge
				 */
				virtual boost::optional<Index> characterEncompassesPosition(Scalar ipd) const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the character whose leading edge is closest the specified location.
				 * @param ipd The distance to the location from the leading edge of this text run in
				 *        inline-progression-dimension, in user units
				 * @return The character in this run
				 * @see #characterHasClosestLeadingEdge
				 */
				virtual Index characterHasClosestLeadingEdge(Scalar ipd) const = 0;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				/**
				 * Returns a @c TextHit corresponding to the specified position. Position outside the bounds of the
				 * glyph content of the @c TextRun map to hits on the leading edge of the first logical character, or
				 * the trailing edge of the last logical character, as appropriate, regardless of the position of that
				 * character in the run.
				 * @param position The logical position, the distance from the line-left edge of the glyph content (not
				 *                 the allocation box) of this text run, in user units
				 * @param bounds The bounds of the @c TextRun. If @c boost#none, the inline-progression-dimension of
				 *               this text run is used
				 * @param[out] outOfBounds @c true if @a position is out of @a bounds. Can be @c null if not needed
				 * @return A hit describing the character and edge (leading or trailing) under the specified position
				 * @see TextLayout#hitTestCharacter
				 */
				virtual TextHit<> hitTestCharacter(Scalar position,
					const boost::optional<NumericRange<Scalar>>& bounds,
					bool* outOfBounds = nullptr) const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the logical position of the specified character in this text run. This is the distance from
				 * the line-left edge of the glyph content (not the allocation box) of this text run to the specified
				 * character.
				 * @param hit The hit to check. This must be a valid hit on the @c TextRun
				 * @return The logical character position in user units
				 * @throw IndexOutOfBounds @a hit is not valid for the @c TextRun
				 * @see GlyphVector#glyphPosition, TextLayout#hitToPoint
				 */
				virtual Scalar hitToLogicalPosition(const TextHit<>& hit) const = 0;
				/// @}

				/// @name Box Model of CSS 3 and XSL 1.1
				/// @{
				/**
				 * Returns the border.
				 * @return The border, or @c null if absent
				 * @see #margin, #padding
				 */
				virtual const presentation::FlowRelativeFourSides<ActualBorderSide>* border() const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the margin.
				 * @return The margin widths in user units, or @c null if absent
				 * @see #border, #padding
				 */
				virtual const presentation::FlowRelativeFourSides<Scalar>* margin() const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the padding.
				 * @return The padding widths in user units, or @c null if absent
				 * @see #border, #margin
				 */
				virtual const presentation::FlowRelativeFourSides<Scalar>* padding() const BOOST_NOEXCEPT = 0;
				/// @}

				/// @name Other Typographic Attributes
				/// @{
				virtual std::uint8_t characterLevel() const BOOST_NOEXCEPT = 0;
//				virtual std::shared_ptr<const Font> font() const BOOST_NOEXCEPT = 0;
				/// @}

				// GlyphVector
				/// @see GlyphVector#direction
				presentation::ReadingDirection direction() const BOOST_NOEXCEPT {
					return ((characterLevel() & 0x01) == 0x00) ?
						presentation::LEFT_TO_RIGHT : presentation::RIGHT_TO_LEFT;
				}
			};

			/// @name Free Functions to Compute Box of Text Run
			/// @{
			presentation::FlowRelativeFourSides<Scalar> contentBox(const TextRun& textRun) BOOST_NOEXCEPT;
			/**
			 * Returns the 'padding-box' of the specified text run in user units.
			 * @see #borderBox, #contentBox, #marginBox, TextRun#padding
			 */
			inline presentation::FlowRelativeFourSides<Scalar> paddingBox(const TextRun& textRun) BOOST_NOEXCEPT {
				presentation::FlowRelativeFourSides<Scalar> bounds(contentBox(textRun));
				if(const presentation::FlowRelativeFourSides<Scalar>* const paddingWidths = textRun.padding()) {
					bounds.before() -= paddingWidths->before();
					bounds.after() += paddingWidths->after();
					bounds.start() -= paddingWidths->start();
					bounds.end() += paddingWidths->end();
				}
				return bounds;
			}
			presentation::FlowRelativeFourSides<Scalar> borderBox(const TextRun& textRun) BOOST_NOEXCEPT;
			/**
			 * Returns the 'margin-box' of the specified text run in user units.
			 * @see #borderBox, #contentBox, #paddingBox, TextRun#margin
			 */
			inline presentation::FlowRelativeFourSides<Scalar> marginBox(const TextRun& textRun) BOOST_NOEXCEPT {
				presentation::FlowRelativeFourSides<Scalar> bounds(borderBox(textRun));
				if(const presentation::FlowRelativeFourSides<Scalar>* const marginWidths = textRun.margin()) {
					bounds.before() -= marginWidths->before();
					bounds.after() += marginWidths->after();
					bounds.start() -= marginWidths->start();
					bounds.end() += marginWidths->end();
				}
				return bounds;
			}
			/**
			 * Returns the 'allocation-rectangle' of the specified text run in user units.
			 */
			inline presentation::FlowRelativeFourSides<Scalar> allocationBox(const TextRun& textRun) BOOST_NOEXCEPT {
				presentation::FlowRelativeFourSides<Scalar> bounds(borderBox(textRun));
				if(const presentation::FlowRelativeFourSides<Scalar>* const marginWidths = textRun.margin()) {
					bounds.start() -= marginWidths->start();
					bounds.end() += marginWidths->end();
				}
				return bounds;
			}
			/**
			 * Returns the measure of the 'content-box' of the specified text run in user units.
			 * @see #allocationMeasure
			 */
			inline Scalar measure(const TextRun& textRun) {
				return textRun.hitToLogicalPosition(TextHit<>::leading(textRun.characterRange().length()));
			}
			Scalar allocationMeasure(const TextRun& textRun);
			/// @}
		}
	}
}

#endif // !ASCENSION_TEXT_RUN_HPP
