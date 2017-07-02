/**
 * @file standard-text-renderer.cpp
 * Implements @c StandardTextRenderer class.
 * @author exeal
 * @date 2016-07-05 Created.
 */

#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/standard-text-renderer.hpp>	// LineLayoutVector.document
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/styles/length.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Protected constructor.
			 * @param document The document
			 * @param initialSize
			 */
			StandardTextRenderer::StandardTextRenderer(kernel::Document& document, const Dimension& initialSize) : TextRenderer(document, initialSize) {
			}

			/// Destructor.
			StandardTextRenderer::~StandardTextRenderer() BOOST_NOEXCEPT {
			}

			/// @see graphics#font#TextRenderer#createEmptyLineLayout
			std::unique_ptr<const TextLayout> StandardTextRenderer::createEmptyLineLayout() const {
				return createLineLayout(boost::none);
			}

			/// @see graphics#font#TextRenderer#createLineLayout
			std::unique_ptr<const TextLayout> StandardTextRenderer::createLineLayout(Index line) const {
				return createLineLayout(boost::make_optional(line));
			}

			/// @internal
			std::unique_ptr<const TextLayout> StandardTextRenderer::createLineLayout(boost::optional<Index> line) const {
				static const String emptyString;
//				const std::unique_ptr<const RenderingContext2D> renderingContext(widgetapi::createRenderingContext(textArea_.textViewer()));
				const std::unique_ptr<const RenderingContext2D> renderingContext(strategy().renderingContext());
				auto styles(buildStylesForLineLayout(line, *renderingContext));
				return std::unique_ptr<const TextLayout>(
					new TextLayout(
						(line == boost::none) ? emptyString : layouts().document().lineString(boost::get(line)),
						std::get<0>(styles), std::get<1>(styles), std::move(std::get<2>(styles)), std::get<3>(styles),
						presentation::styles::Length::Context(*renderingContext, strategy().lengthContextViewport()),
						strategy().parentContentArea(), strategy().fontCollection(), renderingContext->fontRenderContext()));
			}

			/**
			 * Sets the new @c Strategy object.
			 * @param newStrategy The new strategy object to set
			 */
			void StandardTextRenderer::setStrategy(std::unique_ptr<const Strategy> newStrategy) {
				strategy_ = std::move(newStrategy);
			}
		}
	}
}
