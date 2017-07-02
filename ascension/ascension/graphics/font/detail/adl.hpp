/**
 * @file adl.hpp
 * Defines internal @c Adl class.
 * @author exeal
 * @date 2017-05-28 Created.
 */

#ifndef ASCENSION_ADL_HPP
#define ASCENSION_ADL_HPP
#include <ascension/corelib/numeric-range.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			namespace detail {
				/**
				 * @internal Packing structure of an ascent, a descent and a leading values.
				 * @tparam T An arithmetic type
				 */
				template<typename T>
				class Adl {
				public:
					/// An arithmetic type.
					typedef T Representation;

					/// Creates an @c Adl instance and does not initialize the values.
					Adl() {}

					/**
					 * Creates an @c Adl instance with initial values.
					 * @param ascent The 'ascent'
					 * @param descent The 'descent'
					 * @param leading The 'leading'
					 */
					Adl(Representation ascent, Representation descent, Representation leading) : ascent_(ascent), descent_(descent), leading_(leading) {}

					/// Returns the 'ascent'.
					Representation ascent() const {
						return ascent_;
					}

					/// Returns the 'descent'.
					Representation descent() const {
						return descent_;
					}

					/**
					 * Returns the extent in block-progression-dimension.
					 * @param baselineOffset The baseline offset
					 * @param negativeVertical Set @c true if the layout is negative vertical
					 */
					NumericRange<Representation> extent(Representation baselineOffset, bool negativeVertical) const {
						return !negativeVertical ? nrange(baselineOffset - ascent(), baselineOffset + descent()) : nrange(baselineOffset - descent(), baselineOffset + ascent());
					}

					/**
					 * Returns the extent in block-progression-dimension with leading. The leading is processed as
					 * 'half-leading's described by CSS 2.1 (http://www.w3.org/TR/CSS21/visudet.html#leading).
					 * @param baselineOffset The baseline offset
					 * @param negativeVertical Set @c true if the layout is negative vertical
					 */
					NumericRange<Representation> extentWithHalfLeadings(Representation baselineOffset, bool negativeVertical) const {
						auto lineUnder = *boost::const_end(extent(baselineOffset, negativeVertical));
						lineUnder += leading() / 2;
						return nrange(lineUnder - height(), lineUnder);
					}

					/// Returns the height. Height is sum of 'ascent', 'descent' and 'leading'.
					Representation height() const {
						return ascent() + descent() + leading();
					}

					/// Returns the 'leading'.
					Representation leading() const {
						return leading_;
					}

				private:
					Representation ascent_, descent_, leading_;
				};
			}
		}
	}
}

#endif // !ASCENSION_ADL_HPP
