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

#include <ascension/graphics/glyph-vector.hpp>
#include <boost/optional.hpp>
#if defined(ASCENSION_SHAPING_ENGINE_CORE_TEXT)
#	include <CTRun.h>
#elif defined(ASCENSION_SHAPING_ENGINE_DIRECT_WRITE)
#	include <dwrite.h>
#elif defined(ASCENSION_SHAPING_ENGINE_HARF_BUZZ)
#	include <hb.h>
#elif defined(ASCENSION_SHAPING_ENGINE_PANGO)
#	include <pangomm.h>
#elif defined(ASCENSION_SHAPING_ENGINE_QT)
#	include <QGlyphRun>
#elif defined(ASCENSION_SHAPING_ENGINE_UNISCRIBE)
#	include <usp10.h>
#elif defined(ASCENSION_SHAPING_ENGINE_WIN32_GDI) || defined(ASCENSION_SHAPING_ENGINE_WIN32_GDIPLUS)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {
	namespace graphics {

		class RenderingContext2D;

		namespace font {
			/**
			 * Abstract class represents a minimum text run whose characters can shaped by single
			 * font and has single text reading direction.
			 * @see GlyphVector, TextLayout
			 */
			class TextRun : public GlyphVector {
			public:
				/// Destructor.
				virtual ~TextRun() BOOST_NOEXCEPT {}
				/// Returns the number of characters in this run.
				virtual Index length() const BOOST_NOEXCEPT = 0;

				/// @name Hit Test
				/// @{
				/**
				 * Returns the character encompasses the specified location.
				 * @param ipd The distance to the location from the leading edge of this text run
				 *            in inline-progression-dimension
				 * @return The character index in this run, or @c boost#none if @a ipd is outside
				 *         of this text run
				 * @see #characterHasClosestLeadingEdge
				 */
				virtual boost::optional<Index> characterEncompassesPosition(Scalar ipd) const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the character whose leading edge is closest the specified location.
				 * @param ipd The distance to the location from the leading edge of this text run
				 *            in inline-progression-dimension
				 * @return The character in this run
				 * @see #characterHasClosestLeadingEdge
				 */
				virtual Index characterHasClosestLeadingEdge(Scalar ipd) const = 0;
				/// @}

				/// @name Glyph Edges
				/// @{
				/**
				 * Returns the distance in inline-progression-dimension from the leading edge of
				 * this text run to one of the glyph of the specified character.
				 * @param character The character index in this run
				 * @return The leading edge
				 * @throw std#out_of_range @a character is outside of this text run
				 * @see #trailingEdge
				 */
				virtual Scalar leadingEdge(Index character) const = 0;
				/// Returns the measure of this text run.
				Scalar measure() const {
					return trailingEdge(length());
				}
				/**
				 * Returns the distance in inline-progression-dimension from the leading edge of
				 * this text run to trailing edge of the glyph of the specified character.
				 * @param character The character index in this run
				 * @return The trailing edge
				 * @throw std#out_of_range @a character is outside of this text run
				 * @see #leadingEdge
				 */
				virtual Scalar trailingEdge(Index character) const = 0;
				/// @}

				/// @name Box Model of CSS 3 and XSL 1.1
				/// @{
				presentation::FlowRelativeFourSides<Scalar> allocationBox() const BOOST_NOEXCEPT {
					presentation::FlowRelativeFourSides<Scalar> bounds(borderBox());
					if(const presentation::FlowRelativeFourSides<Scalar>* const marginWidths = margin()) {
						bounds.start() -= marginWidths->start();
						bounds.end() += marginWidths->end();
					}
					return bounds;
				}
				/**
				 * Returns the border.
				 * @return The border, or @c null if absent
				 * @see #margin, padding
				 */
				virtual const presentation::FlowRelativeFourSides<ComputedBorderSide>* border() const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the 'border-box' of this text run in device units.
				 * @see #border, #contentBox, #marginBox, #paddingBox
				 */
				presentation::FlowRelativeFourSides<Scalar> borderBox() const BOOST_NOEXCEPT {
					presentation::FlowRelativeFourSides<Scalar> bounds(paddingBox());
					if(const presentation::FlowRelativeFourSides<ComputedBorderSide>* const borders = border()) {
						bounds.before() -= borders->before().computedWidth();
						bounds.after() += borders->after().computedWidth();
						bounds.start() -= borders->start().computedWidth();
						bounds.end() += borders->end().computedWidth();
					}
					return bounds;
				}
				/**
				 * Returns the 'content-box' of this text run in device units.
				 * @see #borderBox, #marginBox, #paddingBox
				 */
				virtual presentation::FlowRelativeFourSides<Scalar> contentBox() const BOOST_NOEXCEPT {
					return logicalBounds();
				}
				/**
				 * Returns the margin.
				 * @return The margin widths in device units, or @c null if absent
				 * @see #border, padding
				 */
				virtual const presentation::FlowRelativeFourSides<Scalar>* margin() const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the 'margin-box' of this text run in device units.
				 * @see #borderBox, #contentBox, #paddingBox, #margin
				 */
				presentation::FlowRelativeFourSides<Scalar> marginBox() const BOOST_NOEXCEPT {
					presentation::FlowRelativeFourSides<Scalar> bounds(borderBox());
					if(const presentation::FlowRelativeFourSides<Scalar>* const marginWidths = margin()) {
						bounds.before() -= marginWidths->before();
						bounds.after() += marginWidths->after();
						bounds.start() -= marginWidths->start();
						bounds.end() += marginWidths->end();
					}
					return bounds;
				}
				/**
				 * Returns the padding.
				 * @return The padding widths in device units, or @c null if absent
				 * @see #border, #margin
				 */
				virtual const presentation::FlowRelativeFourSides<Scalar>* padding() const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the 'padding-box' of this text run in device units.
				 * @see #borderBox, #contentBox, #marginBox, #padding
				 */
				presentation::FlowRelativeFourSides<Scalar> paddingBox() const BOOST_NOEXCEPT {
					presentation::FlowRelativeFourSides<Scalar> bounds(contentBox());
					if(const presentation::FlowRelativeFourSides<Scalar>* const paddingWidths = padding()) {
						bounds.before() -= paddingWidths->before();
						bounds.after() += paddingWidths->after();
						bounds.start() -= paddingWidths->start();
						bounds.end() += paddingWidths->end();
					}
					return bounds;
				}
				/// @}

				/// @name Other Typographic Attributes
				virtual std::uint8_t characterLevel() const BOOST_NOEXCEPT = 0;
				virtual std::shared_ptr<const Font> font() const BOOST_NOEXCEPT = 0;
				/// @}

				// GlyphVector
				/// @see GlyphVector#direction
				presentation::ReadingDirection direction() const BOOST_NOEXCEPT {
					return ((characterLevel() & 0x01) == 0x00) ?
						presentation::LEFT_TO_RIGHT : presentation::RIGHT_TO_LEFT;
				}
			};

			struct ComputedTextDecoration;

			void paintTextDecoration(PaintContext& context,
				const TextRun& run, const NativePoint& origin, const ComputedTextDecoration& style);
		}
	}
}

#endif // !ASCENSION_TEXT_RUN_HPP
