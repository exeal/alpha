/**
 * @file text-layout-styles.cpp
 * @author exeal
 * @date 2003-2006 (was TextLayout.cpp)
 * @date 2006-2011
 * @date 2010-11-20 renamed from ascension/layout.cpp
 * @date 2015-05-10 Separated from text-layout.cpp.
 */

#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-run.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Returns @c true if the given @c TextLayout is vertical.
			 * @param layout The @c TextLayout object
			 * @return @c true if @a layout is vertical; @c false otherwise
			 */
			bool isVertical(const TextLayout& layout) BOOST_NOEXCEPT {
				return presentation::isVertical(boost::fusion::at_key<presentation::styles::WritingMode>(layout.parentStyle()));
			}

			/**
			 * Returns the writing modes of the given @c TextLayout.
			 * @param layout The @c TextLayout object
			 * @return The writing modes of @a layout
			 */
			presentation::WritingMode writingMode(const TextLayout& layout) BOOST_NOEXCEPT {
				return presentation::WritingMode(
					boost::fusion::at_key<presentation::styles::Direction>(layout.style()),
					boost::fusion::at_key<presentation::styles::WritingMode>(layout.parentStyle()),
					boost::fusion::at_key<presentation::styles::TextOrientation>(layout.style()));
			}

			namespace detail {
				bool isNegativeVertical(const TextLayout& layout) {
					return
						presentation::isVertical(boost::fusion::at_key<presentation::styles::WritingMode>(layout.parentStyle()))
							&& presentation::detail::isNegativeVertical(writingMode(layout));
				}
			}

			/**
			 * Constructor.
			 * @param textString The text string to display
			 * @param toplevelStyle The computed text toplevel style
			 * @param lineStyle The computed text line style
			 * @param textRunStyles The computed text runs styles
			 * @param defaultRunStyle
			 * @param lengthContext
			 * @param percentageResolver
			 * @param fontCollection The font collection
			 * @param fontRenderContext Information about a graphics device which is needed to measure the text correctly
			 */
			TextLayout::TextLayout(const String& textString, 
					const presentation::ComputedTextToplevelStyle& toplevelStyle,
					const presentation::ComputedTextLineStyle& lineStyle,
					std::unique_ptr<presentation::ComputedStyledTextRunIterator> textRunStyles,
					const presentation::ComputedTextRunStyle& defaultRunStyle,
					const presentation::styles::Length::Context& lengthContext,
					const Dimension& parentContentArea,
					const FontCollection& fontCollection, const FontRenderContext& fontRenderContext)
					: textString_(textString), styles_(toplevelStyle, lineStyle, defaultRunStyle), numberOfLines_(0) {
				initialize(std::move(textRunStyles), lengthContext, parentContentArea, fontCollection, fontRenderContext);
			}

			/// Returns the base bidirectional embedding level of this @c TextLayout.
			std::uint8_t TextLayout::characterLevel() const BOOST_NOEXCEPT {
				const auto direction(boost::fusion::at_key<presentation::styles::Direction>(style()));
				return (direction == presentation::RIGHT_TO_LEFT) ? 1 : 0;
			}

			/// Returns the "Computed Value" of @c presentation#TextRunStyle of this layout.
			const presentation::ComputedTextRunStyle& TextLayout::defaultRunStyle() const BOOST_NOEXCEPT {
				return styles_.forRun;
			}

			/// Returns the "Computed Value" of @c presentation#TextToplevelStyle of this layout.
			const presentation::ComputedTextToplevelStyle& TextLayout::parentStyle() const BOOST_NOEXCEPT {
				return styles_.forToplevel;
			}

			/// Returns the "Computed Value" of @c presentation#TextLineStyle of this layout.
			const presentation::ComputedTextLineStyle& TextLayout::style() const BOOST_NOEXCEPT {
				return styles_.forLine;
			}

			/// Returns the baseline of the current line.
			DominantBaseline TextLayout::LineMetricsIterator::baseline() const {
				return boost::fusion::at_key<presentation::styles::DominantBaseline>(layout_->style());
			}

			/**
			 */
			Point TextLayout::LineMetricsIterator::baselineOffsetInPhysicalCoordinates() const {
				if(layout_ == nullptr)
					throw NoSuchElementException();
				switch(boost::fusion::at_key<presentation::styles::WritingMode>(layout_->parentStyle())) {
					case presentation::HORIZONTAL_TB:
						return geometry::make<Point>((geometry::_x = 0.0f, geometry::_y = baselineOffset()));
					case presentation::VERTICAL_RL:
			 			return geometry::make<Point>((geometry::_x = -baselineOffset(), geometry::_y = 0.0f));
					case presentation::VERTICAL_LR:
			 			return geometry::make<Point>((geometry::_x = +baselineOffset(), geometry::_y = 0.0f));
					default:
						ASCENSION_ASSERT_NOT_REACHED();
				}
			}
		}
	}
}
