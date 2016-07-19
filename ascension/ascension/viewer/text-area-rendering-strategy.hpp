/**
 * @file text-area-rendering-strategy.hpp
 * Defines @c TextAreaRenderingStrategy class.
 * @author exeal
 * @date 2016-07-18 Created.
 */

#ifndef ASCENSION_TEXT_AREA_RENDERING_STRATEGY_HPP
#define ASCENSION_TEXT_AREA_RENDERING_STRATEGY_HPP
#include <ascension/graphics/font/standard-text-renderer.hpp>

namespace ascension {
	namespace viewer {
		class TextArea;

		/// Default implementation of @c graphics#font#StandardTextRenderer#Strategy interface by using @c TextArea.
		class TextAreaRenderingStrategy : public graphics::font::StandardTextRenderer::Strategy {
		public:
			explicit TextAreaRenderingStrategy(TextArea& textArea);

		private:
			const graphics::font::FontCollection& fontCollection() const override BOOST_NOEXCEPT;
			graphics::Dimension lengthContextViewport() const override BOOST_NOEXCEPT;
			graphics::Dimension parentContentArea() const override BOOST_NOEXCEPT;
			std::unique_ptr<graphics::RenderingContext2D> renderingContext() const override;
		private:
			TextArea& textArea_;
			const graphics::font::FontCollection fontCollection_;
		};
	}
}

#endif // !ASCENSION_TEXT_AREA_RENDERING_STRATEGY_HPP
