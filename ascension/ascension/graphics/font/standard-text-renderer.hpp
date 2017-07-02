/**
 * @file standard-text-renderer.hpp
 * Defines @c StandardTextRenderer class.
 * @author exeal
 * @date 2016-07-05 Created.
 */

#ifndef ASCENSION_STANDARD_TEXT_RENDERER_HPP
#define ASCENSION_STANDARD_TEXT_RENDERER_HPP
#include <ascension/graphics/font/text-renderer.hpp>

namespace ascension {
	namespace presentation {
		struct ComputedStyledTextRunIterator;
		struct ComputedTextLineStyle;
		struct ComputedTextRunStyle;
		struct ComputedTextToplevelStyle;
	}

	namespace graphics {
		namespace font {
			/// Provides standard implementation of @c TextRenderer#createLineLayout method.
			class StandardTextRenderer : public TextRenderer {
			public:
				class Strategy {
				public:
					/// Destructor.
					virtual ~Strategy() BOOST_NOEXCEPT {}
					/// Returns a @c FontCollection object.
					virtual const FontCollection& fontCollection() const BOOST_NOEXCEPT = 0;
					/// Returns the viewport size of @c presentation#styles#Length#Context object which is passed to
					/// the constructor of @c TextLayout class.
					virtual Dimension lengthContextViewport() const BOOST_NOEXCEPT = 0;
					/// Returns the size of the parent content the in user units.
					virtual Dimension parentContentArea() const BOOST_NOEXCEPT = 0;
					/// Returns a @c RenderingContext2D object.
					virtual std::unique_ptr<RenderingContext2D> renderingContext() const = 0;
				};
			public:
				virtual ~StandardTextRenderer() BOOST_NOEXCEPT;
				std::unique_ptr<const TextLayout> createLineLayout(Index line) const override;
				void setStrategy(std::unique_ptr<const Strategy> newStrategy);

			protected:
				StandardTextRenderer(kernel::Document& document, const Dimension& initialSize);
				/**
				 * Builds styles for @c TextLayout object construction.
				 * @param line The line number, or @c boost#none to request an empty line (see
				 *             @c TextRenderer#createEmptyLineLayout method)
				 * @param renderingContext The rendering context to pass to @c presentation#Length#Context object
				 * @return The tuple of the computed styles. See the constructor of @c TextLayout class
				 */
				virtual std::tuple<
					const presentation::ComputedTextToplevelStyle&,
					const presentation::ComputedTextLineStyle&,
					std::unique_ptr<presentation::ComputedStyledTextRunIterator>,
					const presentation::ComputedTextRunStyle&
				> buildStylesForLineLayout(Index line, const RenderingContext2D& renderingContext) const = 0;
				/**
				 * Returns the @c Strategy object.
				 * @throw IllegalStateException No strategy is presented
				 */
				const Strategy& strategy() const {
					if(strategy_.get() == nullptr)
						throw IllegalStateException("StandardTextRenderer has no strategy.");
					return *strategy_;
				}

			private:
				std::unique_ptr<const TextLayout> createLineLayout(boost::optional<Index> line) const;
			private:
				std::unique_ptr<const Strategy> strategy_;
			};
		}
	}
}

#endif // !ASCENSION_STANDARD_TEXT_RENDERER_HPP
