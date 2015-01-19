/**
 * @file actual-text-styles.cpp
 * @author exeal
 * @date 2015-01-10 Created.
 */

#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			namespace {
				/***/
				PhysicalFourSides<ActualBorderSide> makeActualBorderSides(
						const presentation::ComputedTextRunStyle& computed, const presentation::WritingMode& writingMode, const RenderingContext2D& context) {
					presentation::FlowRelativeFourSides<ActualBorderSide> abstractSides;
					const auto& computedColors = boost::fusion::at_key<presentation::FlowRelativeFourSides<presentation::styles::BorderColor>>(computed);
					const auto& computedStyles = boost::fusion::at_key<presentation::FlowRelativeFourSides<presentation::styles::BorderStyle>>(computed);
					const auto& computedWidths = boost::fusion::at_key<presentation::FlowRelativeFourSides<presentation::styles::BorderWidth>>(computed);
					for(std::size_t i = 0; i < std::extent<decltype(abstractSides)>::value; ++i) {
						boost::fusion::at_key<presentation::styles::BorderColor>(abstractSides[i]) = computedColors[i];
						boost::fusion::at_key<presentation::styles::BorderStyle>(abstractSides[i]) = computedStyles[i];
						boost::fusion::at_key<presentation::styles::BorderWidth>(abstractSides[i]) = computedWidths[i].value();	// TODO: Convert pixels into user units.
					}

					return presentation::mapFlowRelativeToPhysical(writingMode, abstractSides);
				}
			}

			/**
			 * Creates @c ActualTextRunStyleCore object.
			 * @param computed The "Computed Value" of the text run styles
			 * @param writingMode The writing modes to map flow-relative bounds into physical
			 * @param context The rendering context to map between pixels and user units
			 */
			ActualTextRunStyleCore::ActualTextRunStyleCore(
					const presentation::ComputedTextRunStyle& computed, const presentation::WritingMode& writingMode, const RenderingContext2D& context) :
				color(boost::fusion::at_key<presentation::styles::Color>(computed)),
				backgroundColor(boost::fusion::at_key<presentation::styles::BackgroundColor>(computed)),
				borders(makeActualBorderSides(computed, writingMode, context)),
				textDecoration(
					boost::fusion::at_key<presentation::styles::TextDecorationLine>(computed),
					boost::fusion::at_key<presentation::styles::TextDecorationColor>(computed),
					boost::fusion::at_key<presentation::styles::TextDecorationStyle>(computed),
					boost::fusion::at_key<presentation::styles::TextDecorationSkip>(computed),
					boost::fusion::at_key<presentation::styles::TextUnderlinePosition>(computed)),
				textEmphasis(
					boost::fusion::at_key<presentation::styles::TextEmphasisStyle>(computed),
					boost::fusion::at_key<presentation::styles::TextEmphasisColor>(computed),
					boost::fusion::at_key<presentation::styles::TextEmphasisPosition>(computed))/*,
				textShadow(boost::fusion::at_key<decltype(textShadow)>(computed))*/ {}
		}
	}
}
