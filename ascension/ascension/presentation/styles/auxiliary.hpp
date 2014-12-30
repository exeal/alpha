/**
 * @file auxiliary.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-24 Separated from text-style.hpp
 */

#ifndef ASCENSION_STYLES_AUXILIARY_HPP
#define ASCENSION_STYLES_AUXILIARY_HPP

#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <ascension/graphics/font/number-substitution.hpp>
#include <ascension/presentation/style-property.hpp>
#include <functional>
#include <string>
#include <boost/functional/hash/extensions.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/operators.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/// Set @c false to disable shaping. Default is @c true.
			typedef StyleProperty<
				Enumerated<bool, true>,
				Inherited<false>
			> ShapingEnabled;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/// Set @c true to make the deprecated format characters (NADS, NODS, ASS and ISS) not effective.
			typedef StyleProperty<
				Enumerated<bool, false>,
				Inherited<false>
			> DeprecatedFormatCharactersDisabled;

			/// Set @c true to inhibit from generating mirrored glyphs.
			typedef StyleProperty<
				Enumerated<bool, false>,
				Inherited<false>
			> SymmetricSwappingInhibited;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			/// @see graphics#font#NumberSubstitution
			typedef StyleProperty<
				Complex<
					graphics::font::NumberSubstitution
				>, Inherited<true>
			> NumberSubstitution;
		}
	}
}

#endif // !ASCENSION_STYLES_AUXILIARY_HPP
