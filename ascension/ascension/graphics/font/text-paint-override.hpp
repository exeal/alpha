/**
 * @file text-layout-styles.hpp
 * @see computed-text-styles.hpp, text-alignment.hpp, presentation/text-style.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010, 2014
 * @date 2010-11-20 renamed from ascension/layout.hpp
 * @date 2011-2012 was text-layout.hpp
 * @date 2012-08-17 separated from text-layout.hpp
 */

#ifndef ASCENSION_TEXT_PAINT_OVERRIDE_HPP
#define ASCENSION_TEXT_PAINT_OVERRIDE_HPP

#include <ascension/corelib/basic-types.hpp>
#include <boost/range/irange.hpp>
#include <memory>
#include <vector>

namespace ascension {
	namespace graphics {
		class Paint;

		namespace font {
			class TextPaintOverride {
			public:
				struct Segment {
					/// The length of this segment.
					Index length;
					/// The overridden foreground or @c null if does not override.
					std::shared_ptr<const Paint> foreground;
					/// The transparency of the overridden foreground. This value should be in the range from 0.0
					/// (fully transparent) to 1.0 (no additional transparency).
					double foregroundAlpha;
					/// The overridden background or @c null if does not override.
					std::shared_ptr<const Paint> background;
					/// The transparency of the overridden background. This value should be in the range from 0.0
					/// (fully transparent) to 1.0 (no additional transparency).
					double backgroundAlpha;
					/// Set @c false to paint only the glyphs' bounds with @c #background. Otherwise the logical
					/// highlight bounds of characters are painted as background.
					bool usesLogicalHighlightBounds;
				};
			public:
				/// Destructor.
				virtual ~TextPaintOverride() BOOST_NOEXCEPT {}
				/**
				 * Returns a vector of segments which describe override the paints of the specified character range in
				 * the line.
				 * @param range The character range in the line
				 * @param[out] result A vector of @c #Segment
				 */
				virtual void queryTextPaintOverride(
					const boost::integer_range<Index>& range, std::vector<const Segment>& result) const = 0;
			};
		}
	}
}

#endif // !ASCENSION_TEXT_PAINT_OVERRIDE_HPP
