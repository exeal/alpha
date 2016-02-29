/**
 * @file text-renderer-observers.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2013
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 * @date 2013-11-21 separated from text-renderer.hpp
 * @see text-renderer.hpp, TextRenderer
 */

#ifndef ASCENSION_TEXT_RENDERER_OBSERVERS_HPP
#define ASCENSION_TEXT_RENDERER_OBSERVERS_HPP
#include <ascension/corelib/basic-types.hpp>	// Index
#include <boost/range/irange.hpp>
#include <memory>
#include <vector>

namespace ascension {
	namespace graphics {
		class Paint;

		namespace font {
			class InlineObject;

			/**
			 * Options for line rendering of @c TextRenderer object.
			 * @see TextRenderer#setLineRenderingOptions
			 */
			class LineRenderingOptions {
			public:
				/// Returned by @c #overrideTextPaint methods.
				struct OverriddenSegment {
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

				/**
				 * Returns the inline object renders the end of line.
				 * @param line The line to render
				 * @return The inline object renders the end of line, or @c null
				 */
				virtual std::unique_ptr<const InlineObject> endOfLine(Index line) const BOOST_NOEXCEPT = 0;
				/**
				 * Returns a vector of text segments which describe override the paints of the specified character
				 * range in the line.
				 * @param line The line to render
				 * @param rangeInLine The character range in the line
				 * @param[out] segments The result. Empty if there is no overrides
				 */
				virtual void overrideTextPaint(Index line,
					const boost::integer_range<Index>& rangeInLine,
					std::vector<const OverriddenSegments>& segments) const BOOST_NOEXCEPT = 0;
				/**
				 * Returns the inline object renders the mark of text wrapping.
				 * @param line The line to render
				 * @return The inline object renders the mark of text wrapping, or @c null
				 */
				virtual std::unique_ptr<const InlineObject> textWrappingMark(Index line) const BOOST_NOEXCEPT = 0;
			};
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_RENDERER_OBSERVERS_HPP
