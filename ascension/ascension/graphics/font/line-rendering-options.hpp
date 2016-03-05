/**
 * @file line-rendering-options.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 * @date 2013-11-21 separated from text-renderer.hpp
 * @date 2016-02-29 Renamed from text-renderer-observers.hpp.
 * @see text-renderer.hpp, TextRenderer
 */

#ifndef ASCENSION_LINE_RENDERING_OPTIONS_HPP
#define ASCENSION_LINE_RENDERING_OPTIONS_HPP
#include <ascension/corelib/basic-types.hpp>	// Index
#include <boost/range/irange.hpp>
#include <memory>
#include <vector>

namespace ascension {
	namespace graphics {
		class Paint;

		namespace font {
			class InlineObject;
			struct OverriddenSegment;

			/**
			 * Options for line rendering of @c TextRenderer object.
			 * @see TextRenderer#setLineRenderingOptions
			 */
			class LineRenderingOptions {
			public:
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
				 * @param[out] segments The result. Empty if there is no overrides
				 */
				virtual void overrideTextPaint(Index line,
					std::vector<const OverriddenSegment>& segments) const BOOST_NOEXCEPT = 0;
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

#endif // !ASCENSION_LINE_RENDERING_OPTIONS_HPP
