/**
 * @file actual-text-styles.cpp
 * @author exeal
 * @date 2015-01-10 Created.
 */

#include <ascension/graphics/font/actual-text-styles.hpp>
#include <ascension/presentation/text-run-style.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			ActualTextRunStyleCore::ActualTextRunStyleCore(const presentation::ComputedTextRunStyle& computed) :
				color(boost::fusion::at_key<presentation::styles::Color>(computed)),
				backgroundColor(boost::fusion::at_key<presentation::styles::BackgroundColor>(computed)),
				borders(boost::fusion::at_key<>(computed)),
				textDecoration(
					boost::fusion::at_key<presentation::styles::TextDecorationLine>(computed),
					boost::fusion::at_key<presentation::styles::TextDecorationColor>(computed),
					boost::fusion::at_key<presentation::styles::TextDecorationStyle>(computed),
					boost::fusion::at_key<presentation::styles::TextDecorationSkip>(computed),
					boost::fusion::at_key<presentation::styles::TextUnderlinePosition>(computed)),
				textEmphasis(boost::fusion::at_key<decltype(textEmphasis)>(computed))/*,
				textShadow(boost::fusion::at_key<decltype(textShadow)>(computed))*/ {}
		}
	}
}
