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

		inline boost::flyweight<styles::ComputedValue<TextToplevelStyle>::type> compute(const styles::SpecifiedValue<TextToplevelStyle>::type& specifiedValues) {
			styles::ComputedValue<TextToplevelStyle>::type computedValues;
			boost::fusion::at_key<styles::WritingMode>(computedValues) = boost::fusion::at_key<styles::WritingMode>(specifiedValues);
			return boost::flyweight<styles::ComputedValue<TextToplevelStyle>::type>(computedValues);
		}

		/// @c boost#hash_value for @c styles#ComputedValue&lt;TextToplevelStyle&gt;type.
		std::size_t hash_value(const styles::ComputedValue<TextToplevelStyle>::type& style) {
			return boost::hash_value(boost::fusion::at_key<styles::WritingMode>(style));
		}
	}
}
