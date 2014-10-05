/**
 * @file numeric-data-types.hpp
 * Defines numeric data types in CSS level 3.
 * @note Precision and ranges of types defined by this header file depend on the compiler and the other systems.
 * @author exeal
 * @date 2014-10-05 Created.
 * @see CSS Values and Units Module Level 3, 4. Numeric Data Types (http://www.w3.org/TR/css3-values/#numeric-types)
 */

#ifndef ASCENSION_STYLES_NUMERIC_DATA_TYPES_HPP
#define ASCENSION_STYLES_NUMERIC_DATA_TYPES_HPP

#include <boost/rational.hpp>

namespace ascension {
	namespace presentation {
		namespace styles {
			/**
			 * @c Integer represents &lt;integer&gt; numeric data type in CSS level 3.
			 * @see CSS Values and Units Module Level 3, 4.1. Integers: the �e&lt;integer&gt;�f type
			 *      (http://www.w3.org/TR/css3-values/#integers)
			 */
			typedef int Integer;

			/**
			 * @c Number represents &lt;number&gt; numeric data type in CSS level 3.
			 * @see CSS Values and Units Module Level 3, 4.2. Numbers: the �e&lt;number&gt;�f type
			 *      (http://www.w3.org/TR/css3-values/#numbers)
			 */
			typedef float Number;

			/**
			 * @c Percentage represents &lt;percentage&gt; numeric data type in CSS level 3.
			 * @see CSS Values and Units Module Level 3, 4.3. Percentages: the �e<percentage>�f type
			 *      (http://www.w3.org/TR/css3-values/#percentages)
			 */
			typedef boost::rational<Integer> Percentage;
		}
	}
}

#endif // !ASCENSION_STYLES_NUMERIC_DATA_TYPES_HPP
