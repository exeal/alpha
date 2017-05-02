/**
 * @file widget-themed-text-renderer.hpp
 * Defines @c WidgetThemedTextRenderer class.
 * @author exeal
 * @date 2016-07-03 Created.
 */

#ifndef ASCENSION_WIDGET_THEMED_TEXT_RENDERER_HPP
#define ASCENSION_WIDGET_THEMED_TEXT_RENDERER_HPP
#include <ascension/graphics/font/standard-text-renderer.hpp>

namespace ascension {
	namespace viewer {
		class TextViewer;

		/// Default implementation of @c graphics#font#TextRenderer abstract class used by @c TextArea.
		class WidgetThemedTextRenderer : public graphics::font::StandardTextRenderer {
		public:
			WidgetThemedTextRenderer(TextViewer& textViewer, const graphics::Dimension& initialSize);
			// graphics.font.TextRenderer
			presentation::BlockFlowDirection blockFlowDirection() const BOOST_NOEXCEPT override;
			presentation::ReadingDirection inlineFlowDirection() const BOOST_NOEXCEPT override;
			graphics::font::TextAnchor textAnchor() const BOOST_NOEXCEPT override;
			presentation::TextOrientation textOrientation() const BOOST_NOEXCEPT override;

		protected:
			// graphics.font.TextRenderer
			graphics::Color actualLineBackgroundColor(const graphics::font::TextLayout& layout) const BOOST_NOEXCEPT override;
			std::shared_ptr<const graphics::font::Font> newDefaultFont() const BOOST_NOEXCEPT override;
			// graphics.font.TextRenderer
			virtual std::shared_ptr<const graphics::Paint> actualBackground() const BOOST_NOEXCEPT override;
			// graphics.font.StandardTextRenderer
			virtual std::tuple<
				const presentation::ComputedTextToplevelStyle&,
				const presentation::ComputedTextLineStyle&,
				std::unique_ptr<presentation::ComputedStyledTextRunIterator>,
				const presentation::ComputedTextRunStyle&
			> buildStylesForLineLayout(Index line, const graphics::RenderingContext2D& renderingContext) const override;

		private:
			TextViewer& textViewer_;
		};
	}
}

#endif // !ASCENSION_WIDGET_THEMED_TEXT_RENDERER_HPP
