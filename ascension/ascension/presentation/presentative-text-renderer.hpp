/**
 * @file presentative-text-renderer.hpp
 * Defines @c PresentativeTextRenderer class.
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2013
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 * @date 2016-07-02 Separated from text-renderer.hpp.
 */

#ifndef ASCENSION_PRESENTATIVE_TEXT_RENDERER_HPP
#define ASCENSION_PRESENTATIVE_TEXT_RENDERER_HPP
#include <ascension/graphics/font/standard-text-renderer.hpp>

namespace ascension {
	namespace presentation {
		class PresentativeTextRenderer : public graphics::font::StandardTextRenderer {
		public:
			PresentativeTextRenderer(std::shared_ptr<Presentation> presentation, const graphics::Dimension& initialSize);
			BlockFlowDirection blockFlowDirection() const override BOOST_NOEXCEPT;
			ReadingDirection inlineFlowDirection() const override BOOST_NOEXCEPT;
			graphics::font::TextAnchor textAnchor() const override BOOST_NOEXCEPT;
			TextOrientation textOrientation() const override BOOST_NOEXCEPT;

		protected:
			graphics::Color actualLineBackgroundColor(const graphics::font::TextLayout& layout) const override BOOST_NOEXCEPT;
			std::tuple<
				const ComputedTextToplevelStyle&,
				const ComputedTextLineStyle&,
				std::unique_ptr<ComputedStyledTextRunIterator>,
				const ComputedTextRunStyle&
			> buildStylesForLineLayout(Index line, const graphics::RenderingContext2D& renderingContext) const override;
			std::shared_ptr<const graphics::font::Font> newDefaultFont() const override BOOST_NOEXCEPT;

		private:
			const std::shared_ptr<Presentation> presentation_;
		};
	}
}

#endif // !ASCENSION_PRESENTATIVE_TEXT_RENDERER_HPP
