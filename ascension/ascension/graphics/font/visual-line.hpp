/**
 * @file visual-line.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010 was rendering.hpp
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-05-21 separated from rendering.hpp
 * @date 2015-04-28 Separated from line-layout-vector.hpp.
 */

#ifndef ASCENSION_VISUAL_LINE_HPP
#define ASCENSION_VISUAL_LINE_HPP
#include <ascension/corelib/basic-types.hpp>
#include <boost/operators.hpp>	// boost.totally_ordered<>

namespace ascension {
	namespace graphics {
		namespace font {
			struct VisualLine : private boost::totally_ordered<VisualLine> {
				/// Default constructor.
				VisualLine() BOOST_NOEXCEPT {}
				/**
				 * Constructor takes initial values.
				 * @param line The logical line number
				 * @param subline The visual offset in the logical line
				 */
				VisualLine(Index line, Index subline) BOOST_NOEXCEPT : line(line), subline(subline) {}
				Index line;		///< The logical line number.
				Index subline;	///< The visual offset in the logical line.
			};
			/// The equality operator.
			inline bool operator==(const VisualLine& lhs, const VisualLine& rhs) BOOST_NOEXCEPT {
				return lhs.line == rhs.line && lhs.subline == rhs.subline;
			}
			/// The less-than operator.
			inline bool operator<(const VisualLine& lhs, const VisualLine& rhs) BOOST_NOEXCEPT {
				return lhs.line < rhs.line || (lhs.line == rhs.line && lhs.subline < rhs.subline);
			}
		}
	}
}

#endif // !ASCENSION_VISUAL_LINE_HPP
