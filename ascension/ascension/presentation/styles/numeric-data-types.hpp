/**
 * @file numeric-data-types.hpp
 * Defines numeric data types in CSS level 3.
 * &lt;percentage&gt; type is not defined by this header file. See length.hpp.
 * @note Precision and ranges of types defined by this header file depend on the compiler and the other systems.
 * @author exeal
 * @date 2014-10-05 Created.
 * @see CSS Values and Units Module Level 3, 4. Numeric Data Types (http://www.w3.org/TR/css3-values/#numeric-types)
 */

#ifndef ASCENSION_STYLES_NUMERIC_DATA_TYPES_HPP
#define ASCENSION_STYLES_NUMERIC_DATA_TYPES_HPP

namespace ascension {
	namespace presentation {
		namespace styles {
			/**
			 * @c Integer represents &lt;integer&gt; numeric data type in CSS level 3.
			 * @see CSS Values and Units Module Level 3, 4.1. Integers: the Åe&lt;integer&gt;Åf type
			 *      (http://www.w3.org/TR/css3-values/#integers)
			 */
			typedef int Integer;

			/**
			 * @c Number represents &lt;number&gt; numeric data type in CSS level 3.
			 * @see CSS Values and Units Module Level 3, 4.2. Numbers: the Åe&lt;number&gt;Åf type
			 *      (http://www.w3.org/TR/css3-values/#numbers)
			 */
			typedef float Number;
		}
	}
}

#endif // !ASCENSION_STYLES_NUMERIC_DATA_TYPES_HPP
