/**
 * @file text-toplevel-style.cpp
 * @author exeal
 * @date 2014-10-14 Created.
 */

#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/flyweight.hpp>

namespace ascension {
	namespace presentation {
		/// Default constructor.
		DeclaredTextToplevelStyle::DeclaredTextToplevelStyle() {
			setLinesStyle(std::shared_ptr<const DeclaredTextLineStyle>());
		}

		/**
		 * Sets the default @c DeclaredTextLineStyle of this toplevel element.
		 * @param newStyle The style collection to set. If @c null, this @c DeclaredTextToplevelStyle holds a
		 *                 default-constructed @c DeclaredTextLineStyle
		 */
		void DeclaredTextToplevelStyle::setLinesStyle(std::shared_ptr<const DeclaredTextLineStyle> newStyle) BOOST_NOEXCEPT {
			if(newStyle.get() != nullptr)
				linesStyle_ = newStyle;
			else
				linesStyle_ = std::shared_ptr<const DeclaredTextLineStyle>(&DeclaredTextLineStyle::unsetInstance(), boost::null_deleter());
		}

		/// Returns a @c DeclaredTextToplevelStyle instance filled with @c styles#UNSET values.
		const DeclaredTextToplevelStyle& DeclaredTextToplevelStyle::unsetInstance() {
			static const DeclaredTextToplevelStyle SINGLETON;
			return SINGLETON;
		}

		inline boost::flyweight<ComputedTextToplevelStyle> compute(const SpecifiedTextToplevelStyle& specifiedValues) {
			ComputedTextToplevelStyle computedValues;
			*boost::fusion::find<styles::ComputedValue<styles::WritingMode>::type>(computedValues)
				= *boost::fusion::find<styles::SpecifiedValue<styles::WritingMode>::type>(specifiedValues);
			return boost::flyweight<ComputedTextToplevelStyle>(computedValues);
		}

		/// @c boost#hash_value for @c ComputedTextToplevelStyle.
		std::size_t hash_value(const ComputedTextToplevelStyle& style) {
			return boost::hash_value(*boost::fusion::find<styles::ComputedValue<styles::WritingMode>::type>(style));
		}
	}
}
