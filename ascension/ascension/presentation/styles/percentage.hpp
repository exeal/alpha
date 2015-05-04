/**
 * @file percentage.hpp
 * Defines @c Percentage data type.
 * @note Precision and ranges of types defined by this header file depend on the compiler and the other systems.
 * @author exeal
 * @date 2014-10-05 Created.
 * @date 2015-05-04 Separated from numeric-data-types.hpp.
 * @see CSS Values and Units Module Level 3, 4. Numeric Data Types (http://www.w3.org/TR/css3-values/#numeric-types)
 * @see numeric-data-types.hpp
 */

#ifndef ASCENSION_STYLES_PERCENTAGE_HPP
#define ASCENSION_STYLES_PERCENTAGE_HPP
#include <ascension/presentation/styles/numeric-data-types.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/rational.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/**
			 * @c Percentage represents &lt;percentage&gt; numeric data type in CSS level 3.
			 * @see CSS Values and Units Module Level 3, 4.3. Percentages: the Åe<percentage>Åf type
			 *      (http://www.w3.org/TR/css3-values/#percentages)
			 */
			typedef boost::rational<Integer> Percentage;

			/// Extends @c boost#hash_value for @c Percentage.
			inline std::size_t hash_value(const Percentage& percentage) {
				std::size_t seed = 0;
				boost::hash_combine(seed, percentage.numerator());
				boost::hash_combine(seed, percentage.denominator());
				return seed;
			}

			/**
			 * Converts a percentage into a scalar value.
			 * @tparam Value The type of the return values
			 */
			template<typename Value>
			struct PercentageResolver {
				/// Destructor.
				virtual ~PercentageResolver() BOOST_NOEXCEPT {}
				/**
				 * Converts the given percentage in vertical coordinate into the value.
				 * @param percentage The percentage value
				 * @return The resolved value
				 */
				virtual Value resolvePercentageForHeight(const Percentage& percentage) BOOST_NOEXCEPT = 0;
				/**
				 * Converts the given percentage in horizontal coordinate into the value.
				 * @param percentage The percentage value
				 * @return The resolved value
				 */
				virtual Value resolvePercentageForWidth(const Percentage& percentage) BOOST_NOEXCEPT = 0;
			};
		}
	}
}

namespace boost {
	inline std::size_t hash_value(const ascension::presentation::styles::Percentage& percentage) {
		return ascension::presentation::styles::hash_value(percentage);
	}
}

#endif // !ASCENSION_STYLES_PERCENTAGE_HPP
