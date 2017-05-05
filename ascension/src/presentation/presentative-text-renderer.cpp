/**
 * @file presentative-text-renderer.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2014
 * @date 2010-11-20 separated from ascension/layout.cpp
 * @date 2011-11-12 renamed from rendering.cpp
 * @date 2016-07-02 Separated from text-renderer.cpp.
 */

#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/presentative-text-renderer.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>

namespace ascension {
	namespace presentation {
		PresentativeTextRenderer::PresentativeTextRenderer(
				std::shared_ptr<Presentation> presentation, const graphics::Dimension& initialSize)
				: graphics::font::StandardTextRenderer(presentation->document(), initialSize), presentation_(presentation) {
//			if(presentation_.get() == nullptr)
//				throw NullPointerException("presentation");
		}

		namespace {
			inline graphics::Color alphaBlend(const graphics::Color& background, const graphics::Color& foreground) BOOST_NOEXCEPT {
				// TODO: This code is adhoc.
				const Byte a = foreground.alpha() + background.alpha() - foreground.alpha() * background.alpha() / 255;
				return graphics::Color(
					(foreground.red() * foreground.alpha() + background.red() * (255 - foreground.alpha()) * background.alpha() / 255) / a,
					(foreground.green() * foreground.alpha() + background.green() * (255 - foreground.alpha()) * background.alpha() / 255) / a,
					(foreground.blue() * foreground.alpha() + background.blue() * (255 - foreground.alpha()) * background.alpha() / 255) / a,
					a);
			}

			inline graphics::Color actualToplevelBackgroundColor(const presentation::Presentation& p) {
				const auto computed(boost::fusion::at_key<styles::BackgroundColor>(p.computedTextRunStyle().backgroundsAndBorders));
				return alphaBlend(graphics::Color::OPAQUE_WHITE, computed);
			}
		}

		/// @see TextRenderer#actualBackground
		std::shared_ptr<const graphics::Paint> PresentativeTextRenderer::actualBackground() const BOOST_NOEXCEPT {
			const auto color(actualToplevelBackgroundColor(*presentation_));
			static graphics::SolidColor background(color);
			background.graphics::SolidColor::SolidColor(color);
			return std::shared_ptr<const graphics::Paint>(&background, boost::null_deleter());
		}

		/// @see TextRenderer#actualLineBackgroundColor
		graphics::Color PresentativeTextRenderer::actualLineBackgroundColor(const graphics::font::TextLayout& layout) const BOOST_NOEXCEPT {
			boost::optional<graphics::Color> foregroundOverride, backgroundOverride;
#ifdef ASCENSION_ENABLE_TEXT_LINE_COLOR_SPECIFIER
			presentation().textLineColors(lineToPaint.lineNumber, foregroundOverride, backgroundOverride);
#endif
			const auto usedLineBackgroundColor(
				boost::get_optional_value_or(
					backgroundOverride,
					boost::fusion::at_key<styles::BackgroundColor>(layout.defaultRunStyle().backgroundsAndBorders)));
			return alphaBlend(actualToplevelBackgroundColor(*presentation_), usedLineBackgroundColor);
		}

		/// @see TextRenderer#blockFlowDirection
		BlockFlowDirection PresentativeTextRenderer::blockFlowDirection() const BOOST_NOEXCEPT {
			return boost::fusion::at_key<styles::WritingMode>(presentation_->computedTextToplevelStyle());
		}

		/// @see StandardTextRenderer#buildStylesForLineLayout
		std::tuple<
			const ComputedTextToplevelStyle&,
			const ComputedTextLineStyle&,
			std::unique_ptr<ComputedStyledTextRunIterator>,
			const ComputedTextRunStyle&
		> PresentativeTextRenderer::buildStylesForLineLayout(Index line, const graphics::RenderingContext2D& renderingContext) const {
//			const styles::Length::Context lengthContext(renderingContext, lengthContextViewport());
			return std::tuple<
				const ComputedTextToplevelStyle&,
				const ComputedTextLineStyle&,
				std::unique_ptr<ComputedStyledTextRunIterator>,
				const ComputedTextRunStyle&>(
				presentation_->computedTextToplevelStyle(),
				presentation_->computeTextLineStyle(line),
				std::move(presentation_->computeTextRunStyles(line/*, lengthContext*/)),
				presentation_->computeTextRunStyleForLine(line));	// TODO: Use std.make_tuple instead.
		}

		/// @see TextRenderer#inlineFlowDirection
		ReadingDirection PresentativeTextRenderer::inlineFlowDirection() const BOOST_NOEXCEPT {
			return boost::fusion::at_key<styles::Direction>(presentation_->computedTextLineStyle());
		}

		/// @see TextRenderer#newDefaultFont
		std::shared_ptr<const graphics::font::Font> PresentativeTextRenderer::newDefaultFont() const BOOST_NOEXCEPT {
			const auto& styles = presentation_->computedTextRunStyle();
			// TODO: Use ActualFontSpecification.
			const graphics::font::FontProperties properties(
				boost::fusion::at_key<styles::FontWeight>(styles.fonts),
				boost::fusion::at_key<styles::FontStretch>(styles.fonts),
				boost::fusion::at_key<styles::FontStyle>(styles.fonts));
			// TODO: Replace with more suitable way...
			const auto& fontFamilies = boost::fusion::at_key<styles::FontFamily>(styles.fonts);
			String matchedFamily;
			if(!fontFamilies.empty()) {
				const auto i(findMatchingFontFamily(strategy().fontCollection(), fontFamilies));
				if(i != boost::const_end(fontFamilies))
					matchedFamily = *i;
			}
			const graphics::font::FontDescription description(
				graphics::font::FontFamily(matchedFamily),
#if 0
				Points(
					styles::useFontSize(
						boost::fusion::at_key<styles::FontSize>(styles.fonts),
						systemDefaultFontSize(), systemDefaultFontSize())),	// TODO: Adhoc.
#else
				12,
#endif
				properties);
			return strategy().fontCollection().get(description);
		}

		/// @see TextRenderer#textAnchor
		graphics::font::TextAnchor PresentativeTextRenderer::textAnchor() const BOOST_NOEXCEPT {
			const graphics::font::TextAlignment alignment(boost::fusion::at_key<styles::TextAlignment>(presentation_->computedTextLineStyle()));
			switch(boost::native_value(alignment)) {
				case graphics::font::TextAlignment::START:
				case graphics::font::TextAlignment::JUSTIFY:
				case graphics::font::TextAlignment::MATCH_PARENT:
				case graphics::font::TextAlignment::START_END:
				default:
					return graphics::font::TextAnchor::START;
				case graphics::font::TextAlignment::LEFT:
					return (inlineFlowDirection() != RIGHT_TO_LEFT) ? graphics::font::TextAnchor::START : graphics::font::TextAnchor::END;
				case graphics::font::TextAlignment::RIGHT:
					return (inlineFlowDirection() != RIGHT_TO_LEFT) ? graphics::font::TextAnchor::END : graphics::font::TextAnchor::START;
				case graphics::font::TextAlignment::END:
					return graphics::font::TextAnchor::END;
				case graphics::font::TextAlignment::CENTER:
					return graphics::font::TextAnchor::MIDDLE;
			}
		}

		/// @see TextRenderer#textOrientation
		TextOrientation PresentativeTextRenderer::textOrientation() const BOOST_NOEXCEPT {
			return boost::fusion::at_key<styles::TextOrientation>(presentation_->computedTextLineStyle());
		}
	}
}
