/**
 * @file font-render-context.hpp
 * @author exeal
 * @date 2010-11-06 created as font.hpp
 * @date 2010-2012 was font.hpp
 * @date 2012-08-26 separated from font.hpp
 * @date 2012-09-16 separated from text-run.hpp
 * @date 2012-2013 was glyph-vector.hpp
 * @date 2013-01-15 separated from glyph-vector.hpp
 */

#ifndef ASCENSION_FONT_RENDER_CONTEXT_HPP
#define ASCENSION_FONT_RENDER_CONTEXT_HPP

#include <ascension/graphics/affine-transform.hpp>
#include <memory>
#include <boost/operators.hpp>

namespace ascension {
	namespace graphics {
		struct RenderingHints {
			enum TextAntiAliasing {};
			enum FractionalMetrics {};
		};

		namespace font {
			/**
			 * A container for the information needed to correctly measure text.
			 * @see RenderingContext2D#fontRenderContext
			 * @note This class is designed based on @c java.awt.font.FontRenderContext class.
			 */
			class FontRenderContext : private boost::equality_comparable<FontRenderContext> {
			public:
				FontRenderContext(const geometry::AffineTransform& tx, bool isAntiAliased, bool usesFractionalMetrics);
				FontRenderContext(const geometry::AffineTransform& tx, RenderingHints::TextAntiAliasing aaHint, RenderingHints::FractionalMetrics fmHint);
				bool operator==(const FontRenderContext& other) const;
				RenderingHints::TextAntiAliasing antiAliasingHint() const BOOST_NOEXCEPT;
				RenderingHints::FractionalMetrics fractionalMetricsHint() const BOOST_NOEXCEPT;
				bool isAntiAliased() const BOOST_NOEXCEPT;
				bool isTransformed() const BOOST_NOEXCEPT;
				const geometry::AffineTransform& transform() const BOOST_NOEXCEPT;
				bool usesFractionalMetrics() const BOOST_NOEXCEPT;
			private:
				geometry::AffineTransform transform_;
				RenderingHints::TextAntiAliasing antiAliasingRenderingHint_;
				RenderingHints::FractionalMetrics fractionalMetricsHint_;
			};

			class Font;

			/// A pair of a font and render context.
			class FontAndRenderContext : private boost::equality_comparable<FontAndRenderContext> {
			public:
				/**
				 * Constructor initializes the all data members.
				 * @param font The font
				 * @param fontRenderContext The font render context
				 */
				FontAndRenderContext(std::shared_ptr<const Font> font,
					const FontRenderContext& fontRenderContext)
					: font_(font), fontRenderContext_(fontRenderContext) {}
				/// Equality operator.
				bool operator==(const FontAndRenderContext& other) const {
					return font_ == other.font_ && fontRenderContext_ == other.fontRenderContext_;
				}
				/// Returns the font.
				std::shared_ptr<const Font> font() const BOOST_NOEXCEPT {
					return font_;
				}
				/// Returns the font render context.
				const FontRenderContext& fontRenderContext() const BOOST_NOEXCEPT {
					return fontRenderContext_;
				}
			private:
				std::shared_ptr<const Font> font_;
				FontRenderContext fontRenderContext_;
			};

			std::size_t hash_value(const FontAndRenderContext& farc);	// for boost.flyweight instantiation
		}
	}
}

#endif // !ASCENSION_FONT_RENDER_CONTEXT_HPP
