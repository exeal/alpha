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
#include <boost/functional/hash.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace graphics {
		struct RenderingHints {
			enum TextAntiAliasing {};
			typedef boost::optional<bool> FractionalMetrics;
		};

		namespace font {
			/**
			 * A container for the information needed to correctly measure text.
			 * @see RenderingContext2D#fontRenderContext
			 * @note This class is designed based on @c java.awt.font.FontRenderContext class.
			 */
			class FontRenderContext : private boost::equality_comparable<FontRenderContext> {
			public:
				/**
				 * Constructs a @c FontRenderContext object from an optional @c AffineTransform and two boolean values
				 * that determine if the newly constructed object has anti-aliasing or fractional metrics.
				 * @param tx The transform which is used to scale typographical points to pixels in this
				 *           @c FontRenderContext. If @c boost#none, an identity transform is used.
				 * @param isAntiAliased Determines if the newly constructed object has anti-aliasing
				 * @param usesFractionalMetrics Determines if the newly constructed object has fractional metrics
				 */
				FontRenderContext(const boost::optional<geometry::AffineTransform> tx, bool isAntiAliased, bool usesFractionalMetrics) :
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
				FontRenderContext(const boost::optional<geometry::AffineTransform>& tx,
						RenderingHints::TextAntiAliasing aaHint, const RenderingHints::FractionalMetrics& fmHint) :
						transform_((!tx && geometry::isIdentity(boost::get(tx))) ? new geometry::AffineTransform(boost::get(tx)) : nullptr),
						antiAliasingRenderingHint_(aaHint), fractionalMetricsHint_(fmHint) {
				}
				/// Copy-constructor.
				FontRenderContext(const FontRenderContext& other) :
						transform_((other.transform_.get() != nullptr) ? new geometry::AffineTransform(*other.transform_) : nullptr),
						antiAliasingRenderingHint_(other.antiAliasingRenderingHint_), fractionalMetricsHint_(other.fractionalMetricsHint_) {
				}
				/// Returns @c true if @a other has the same transform, antialiasing, and fractional metrics values as
				/// this.
				bool operator==(const FontRenderContext& other) const {
					if(antiAliasingRenderingHint_ != other.antiAliasingRenderingHint_ || fractionalMetricsHint_ != other.fractionalMetricsHint_)
						return false;
					assert(transform_.get() == nullptr || !geometry::isIdentity(*transform_));
					assert(other.transform_.get() == nullptr || !geometry::isIdentity(*other.transform_));
					if(transform_.get() == nullptr)
						return other.transform_.get() == nullptr;
					return other.transform_.get() != nullptr && geometry::equals(*transform_, *other.transform_);
				}
				/// Returns the text anti-aliasing rendering mode hint used in this @c FontRenderContext.
				RenderingHints::TextAntiAliasing antiAliasingHint() const BOOST_NOEXCEPT {
					return antiAliasingRenderingHint_;
				}
				/// Returns the text fractional metrics renderinf mode hint used in this @c FontRenderContext.
				const RenderingHints::FractionalMetrics& fractionalMetricsHint() const BOOST_NOEXCEPT {
					return fractionalMetricsHint_;
				}
				/// Returns a boolean which indicates whether or not some form of antialiasing is specified by this
				/// @c FontRenderContext.
				bool isAntiAliased() const BOOST_NOEXCEPT {
					return true;	// TODO: Not implemented.
				}
				/// Indicates whether or not this @c FontRenderContext object measures text in a transformed render
				/// context.
				bool isTransformed() const BOOST_NOEXCEPT {
					return transform_.get() != nullptr/* && !geometry::isIdentity(*transform_)*/;
				}
				/// Returns the transform that is used to scale typographical points to pixels in this
				/// @c FontRenderContext.
				geometry::AffineTransform&& transform() const BOOST_NOEXCEPT {
					return (transform_.get() != nullptr) ? geometry::AffineTransform(*transform_) : geometry::makeIdentityTransform();
				}
				/// Returns a boolean which whether text fractional metrics mode is used in this @c FontRenderContext.
				bool usesFractionalMetrics() const BOOST_NOEXCEPT {
					return boost::get_optional_value_or(fractionalMetricsHint_, false);
				}

			private:
				friend std::size_t hash_value(const FontRenderContext& frc);
				const std::unique_ptr<const geometry::AffineTransform> transform_;
				const RenderingHints::TextAntiAliasing antiAliasingRenderingHint_;
				const RenderingHints::FractionalMetrics fractionalMetricsHint_;
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

			inline std::size_t hash_value(const FontRenderContext& frc) {	// for boost.flyweight instantiation
				std::size_t v = geometry::hash_value(frc.transform());
				boost::hash_combine(v, frc.antiAliasingHint());
				const RenderingHints::FractionalMetrics& fm = frc.fractionalMetricsHint();
				boost::hash_combine(v, (fm != boost::none) ? (boost::get(fm) ? 1 : 0) : 2);
				return v;
			}
			inline std::size_t hash_value(const FontAndRenderContext& farc) {	// for boost.flyweight instantiation
				std::size_t v = boost::hash_value(farc.font().get());
				boost::hash_combine(v, farc.fontRenderContext());
				return v;
			}
		}
	}
}

#endif // !ASCENSION_FONT_RENDER_CONTEXT_HPP
