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
				presentation::FlowRelativeFourSides<ActualBorderSide> makeActualBorderSides(
						const presentation::styles::ComputedValuesOfParts<
							presentation::TextRunStyleParts::BackgroundsAndBorders
						>::type& computed,
						const presentation::styles::Length::Context& context) {
					presentation::FlowRelativeFourSides<ActualBorderSide> abstractSides;
					const auto& computedColors = boost::fusion::at_key<presentation::FlowRelativeFourSides<presentation::styles::BorderColor>>(computed);
					const auto& computedStyles = boost::fusion::at_key<presentation::FlowRelativeFourSides<presentation::styles::BorderStyle>>(computed);
					const auto& computedWidths = boost::fusion::at_key<presentation::FlowRelativeFourSides<presentation::styles::BorderWidth>>(computed);
					for(std::size_t i = 0; i < abstractSides.size(); ++i) {
						boost::fusion::at_key<presentation::styles::BorderColor>(abstractSides[i]) = computedColors[i];
						boost::fusion::at_key<presentation::styles::BorderStyle>(abstractSides[i]) = computedStyles[i];
						boost::fusion::at_key<presentation::styles::BorderWidth>(abstractSides[i]) = computedWidths[i].value(context);
					}

					return presentation::FlowRelativeFourSides<ActualBorderSide>(abstractSides);
				}

				template<typename Style>
				presentation::FlowRelativeFourSides<Scalar> makeActualMarginsOrPaddings(
						const presentation::styles::ComputedValuesOfParts<
							presentation::TextRunStyleParts::BasicBoxModel
						>::type& computed,
						const presentation::styles::Length::Context& context, Scalar computedParentMeasure) {
					presentation::FlowRelativeFourSides<Scalar> abstractSides;
					for(std::size_t i = 0; i < abstractSides.size(); ++i) {
						const auto& computedValue = boost::fusion::at_key<presentation::FlowRelativeFourSides<Style>>(computed)[i];
						if(const presentation::styles::Length* length = boost::get<presentation::styles::Length>(&computedValue))
							abstractSides[i] = length->value(context);
						else if(const presentation::styles::Percentage* percentage = boost::get<presentation::styles::Percentage>(&computedValue))
							abstractSides[i] = computedParentMeasure * boost::rational_cast<Scalar>(*percentage);
						else
							abstractSides[i] = 0;	// see https://drafts.csswg.org/css-box-3/#inline-non-replaced
					}

					return abstractSides;
				}
			}

			/**
			 * Creates @c ActualTextRunStyleCore object.
			 * @param computed The "Computed Value" of the text run styles
			 * @param context The rendering context to map between pixels and user units
			 * @param computedParentMeasure The measure of the parent in user units
			 */
			ActualTextRunStyleCore::ActualTextRunStyleCore(
					const presentation::ComputedTextRunStyle& computed, const presentation::styles::Length::Context& context, Scalar computedParentMeasure) :
				color(boost::fusion::at_key<presentation::styles::Color>(computed.colors)),
				backgroundColor(boost::fusion::at_key<presentation::styles::BackgroundColor>(computed.backgroundsAndBorders)),
				borders(makeActualBorderSides(computed.backgroundsAndBorders, context)),
				margins(makeActualMarginsOrPaddings<presentation::styles::MarginSide>(computed.basicBoxModel, context, computedParentMeasure)),
				paddings(makeActualMarginsOrPaddings<presentation::styles::PaddingSide>(computed.basicBoxModel, context, computedParentMeasure)),
				textDecoration(
					boost::fusion::at_key<presentation::styles::TextDecorationLine>(computed.textDecoration),
					boost::fusion::at_key<presentation::styles::TextDecorationColor>(computed.textDecoration),
					boost::fusion::at_key<presentation::styles::TextDecorationStyle>(computed.textDecoration),
					boost::fusion::at_key<presentation::styles::TextDecorationSkip>(computed.textDecoration),
					boost::fusion::at_key<presentation::styles::TextUnderlinePosition>(computed.textDecoration)),
				textEmphasis(
					boost::fusion::at_key<presentation::styles::TextEmphasisStyle>(computed.textDecoration),
					boost::fusion::at_key<presentation::styles::TextEmphasisColor>(computed.textDecoration),
					boost::fusion::at_key<presentation::styles::TextEmphasisPosition>(computed.textDecoration))/*,
				textShadow(boost::fusion::at_key<decltype(textShadow)>(computed))*/ {}
		}
	}
}
