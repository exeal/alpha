/**
 * @file number-substitution.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-07-16 reunioned with text-line-style.hpp
 * @date 2014-09-24 Separated from text-style.hpp
 * @date 2014-09-25 Separated from presentation/styles/auxiliary.hpp
 */

#ifndef ASCENSION_NUMBER_SUBSTITUTION_HPP
#define ASCENSION_NUMBER_SUBSTITUTION_HPP
#include <ascension/corelib/future/scoped-enum-emulation.hpp>
#include <boost/functional/hash/extensions.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/operators.hpp>
#include <functional>
#include <string>

namespace ascension {
	namespace graphics {
		namespace font {
			/// Specifies how numbers in text are displayed in different locales.
			struct NumberSubstitution : private boost::equality_comparable<NumberSubstitution> {
				/// Specifies how the locale for numbers in a text run is determined.
				ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(LocaleSource)
					/// Number locale is derived from the text run.
					TEXT,
					/// Number locale is derived from the value of the current thread.
					USER,
					/// Number locale is derived from @c #localeOverride.
					OVERRIDE
				ASCENSION_SCOPED_ENUM_DECLARE_END(LocaleSource)

				/// The type of number substitution to perform on numbers in a text run.
				ASCENSION_SCOPED_ENUM_DECLARE_BEGIN(Method)
					/// The substitution method should be determined based on the number locale.
					AS_LOCALE,
					/// If the number locale is an Arabic or Farsi, specifies that the digits depend on the context.
					/// Either traditional or Latin digits are used depending on the nearest preceding strong
					/// character, or if there is none, the text direction of the paragraph.
					CONTEXT,
					/// Code points U+0030..0039 are always rendered as European digits, in which case, no number
					/// substitution is performed.
					EUROPEAN,
					/// Numbers are rendered using the national digits for the number locale, as specified by the
					/// locale.
					NATIVE_NATIONAL,
					/// Numbers are rendered using the traditional digits for the number locale. For most locales, this
					/// is the same as @c NATIVE_NATIONAL enumeration value. However, using @c NATIVE_NATIONAL can
					/// result in Latin digits for some Arabic locales, whereas using @c TRADITIONAL results in Arabic
					/// digits for all Arabic locales.
					TRADITIONAL
				ASCENSION_SCOPED_ENUM_DECLARE_END(Method)

				/**
				 * The locale to use when the value of @c #localeSource is @c LocaleSource#OVERRIDE. If
				 * @c #localeSource is not @c LocaleSource#OVERRIDE, this is ignored. The default value is an empty
				 * string.
				 */
				std::string localeOverride;

				/**
				 * The source of the locale that is used to determine number substitution. The default value is
				 * @c LocaleSource#TEXT.
				 */
				LocaleSource localeSource;

				/**
				 * The substitution method that is used to determine number substitution. The default
				 * value is @c Method#AS_LOCALE.
				 */
				Method method;

				/// Default constructor initializes its data members with the initial values.
				NumberSubstitution() : localeSource(LocaleSource::TEXT), method(Method::AS_LOCALE) {}

				/// Equality operator.
				bool operator==(const NumberSubstitution& other) const BOOST_NOEXCEPT {
					return localeOverride == other.localeOverride
						&& localeSource == other.localeSource && method == other.method;
				}
			};

			/// Specialization of @c boost#hash_value function template for @c NumberSubstitution.
			inline std::size_t hash_value(const NumberSubstitution& object) BOOST_NOEXCEPT {
				std::size_t seed = 0;
				boost::hash_combine(seed, object.localeOverride);
				boost::hash_combine(seed, boost::native_value(object.localeSource));
				boost::hash_combine(seed, boost::native_value(object.method));
				return seed;
			}
		}
	}
} // namespace ascension.graphics.font

namespace std {
	/// Specialization of @c std#hash class template for @c NumberSubstitution.
	template<>
	class hash<ascension::graphics::font::NumberSubstitution> :
		public std::function<std::hash<void*>::result_type(const ascension::graphics::font::NumberSubstitution&)> {
	public:
		result_type operator()(const argument_type& key) const BOOST_NOEXCEPT {
			return boost::hash<argument_type>()(key);
		}
	};
}

#endif // !ASCENSION_NUMBER_SUBSTITUTION_HPP
