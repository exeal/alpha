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

				/// @name Geometric Definitions from XSL 1.1
				/// @{
				virtual presentation::FlowRelativeFourSides<Scalar> allocationRectangle() const BOOST_NOEXCEPT;
				virtual presentation::FlowRelativeFourSides<Scalar> borderRectangle() const BOOST_NOEXCEPT;
				virtual const presentation::FlowRelativeFourSides<ComputedBorderSide>* borders() const BOOST_NOEXCEPT = 0;
				virtual presentation::FlowRelativeFourSides<Scalar> contentRectangle() const BOOST_NOEXCEPT;
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
