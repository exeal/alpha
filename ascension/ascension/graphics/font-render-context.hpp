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
#include <boost/flyweight.hpp>
#include <boost/operators.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			class FontRenderContext : private boost::equality_comparable<FontRenderContext> {
			public:
				bool operator==(const FontRenderContext& other) const;
				const NativeAffineTransform& transform() const BOOST_NOEXCEPT;
			private:
				NativeAffineTransform transform_;
//				??? antiAliasingRenderingHint_;
//				??? fractionalMetricsHint_;
			};

			std::size_t hash_value(const FontRenderContext& frc);	// for boost.flyweight instantiation

			class Font;

			class FontAndRenderContext {
			public:
				/**
				 * Constructor initializes the all data members.
				 * @param font The font
				 * @param fontRenderContext The font render context
				 */
				FontAndRenderContext(std::shared_ptr<const Font> font,
					const FontRenderContext& fontRenderContext)
					: font_(font), fontRenderContext_(fontRenderContext) {}
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
				boost::flyweight<FontRenderContext> fontRenderContext_;
			};
		}
	}
}

#endif // !ASCENSION_FONT_RENDER_CONTEXT_HPP
