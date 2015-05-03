/**
 * @file font-render-context.cpp
 * Implements @c FontRenderContext and @c FontAndRenderContext classes.
 * @author exeal
 * @date 2010-11-06 created as font.hpp
 * @date 2010-2012 was font.hpp
 * @date 2012-08-26 separated from font.hpp
 * @date 2012-09-16 separated from text-run.hpp
 * @date 2012-2013 was glyph-vector.hpp
 * @date 2013-01-15 separated from glyph-vector.hpp
 * @date 2015-05-03 Separated from font-render-context.hpp.
 */

#include <ascension/graphics/font/font-render-context.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Constructs a @c FontRenderContext object from an optional @c AffineTransform and two boolean values that
			 * determine if the newly constructed object has anti-aliasing or fractional metrics.
			 * @param tx The transform which is used to scale typographical points to pixels in this
			 *           @c FontRenderContext. If @c boost#none, an identity transform is used.
			 * @param isAntiAliased Determines if the newly constructed object has anti-aliasing
			 * @param usesFractionalMetrics Determines if the newly constructed object has fractional metrics
			 */
			FontRenderContext::FontRenderContext(const boost::optional<geometry::AffineTransform> tx, bool isAntiAliased, bool usesFractionalMetrics) :
					transform_((!tx && geometry::isIdentity(boost::get(tx))) ? new geometry::AffineTransform(boost::get(tx)) : nullptr),
					antiAliasingRenderingHint_(), fractionalMetricsHint_(usesFractionalMetrics) {
			}

			/**
			 * Constructs a @c FontRenderContext object from an optional @c AffineTransform and two values that
			 * determine if the newly constructed object has anti-aliasing or fractional metrics.
			 * @param tx The transform which is used to scale typographical points to pixels in this
			 *           @c FontRenderContext
			 * @param aaHint One of the text antialiasing rendering hint values defined in
			 *               @c RenderingHints#TextAntiAliasing
			 * @param fmHint
			 */
			FontRenderContext::FontRenderContext(const boost::optional<geometry::AffineTransform>& tx,
					RenderingHints::TextAntiAliasing aaHint, const RenderingHints::FractionalMetrics& fmHint) :
					transform_((!tx && geometry::isIdentity(boost::get(tx))) ? new geometry::AffineTransform(boost::get(tx)) : nullptr),
					antiAliasingRenderingHint_(aaHint), fractionalMetricsHint_(fmHint) {
			}

			/// Copy-constructor.
			FontRenderContext::FontRenderContext(const FontRenderContext& other) :
					transform_((other.transform_.get() != nullptr) ? new geometry::AffineTransform(*other.transform_) : nullptr),
					antiAliasingRenderingHint_(other.antiAliasingRenderingHint_), fractionalMetricsHint_(other.fractionalMetricsHint_) {
			}

			/// Copy-constructor.
			FontAndRenderContext::FontAndRenderContext(const FontAndRenderContext& other) : font_(other.font_), fontRenderContext_(other.fontRenderContext_) {
			}

			/**
			 * Constructor initializes the all data members.
			 * @param font The font
			 * @param fontRenderContext The font render context
			 */
			FontAndRenderContext::FontAndRenderContext(std::shared_ptr<const Font> font,
					const FontRenderContext& fontRenderContext) : font_(font), fontRenderContext_(fontRenderContext) {
			}

			/// Destructor.
			FontAndRenderContext::~FontAndRenderContext() BOOST_NOEXCEPT {
			}
		}
	}
}
