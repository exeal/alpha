/**
 * @file visual-lines-listener.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010 was rendering.hpp
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-05-21 separated from rendering.hpp
 * @date 2015-03-21 Separated from line-layout-vector.hpp
 */

#ifndef ASCENSION_VISUAL_LINES_LISTENER_HPP
#define ASCENSION_VISUAL_LINES_LISTENER_HPP
#include <ascension/corelib/basic-types.hpp>
#include <boost/range/irange.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/**
			 * Interface for objects which are interested in getting informed about change of
			 * visual lines of @c LineLayoutVector.
			 */
			class VisualLinesModificationListener {
			private:
				/**
				 * Several visual lines were modified.
				 * @param lines The range of modified lines. @a lines.end() is exclusive
				 * @param sublinesDifference The difference of the number of sublines between before and after the
				 *                           modification
				 * @param documentChanged Set @c true if the layouts were modified for the document change
				 * @param longestLineChanged Set @c true if the longest line is changed
				 */
				virtual void visualLinesModified(
					const boost::integer_range<Index>& lines, SignedIndex sublinesDifference,
					bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT = 0;
				friend class LineLayoutVector;
			};

			/**
			 * Interface for objects which are interested in getting informed about change of visual lines of
			 * @c LineLayoutVector.
			 * @see LineLayoutVector#addVisualLinesListener,
			 *      LineLayoutVector#removeVisualLinesListener,
			 *      TextViewport#addVisualLinesListener, TextViewport#removeVisualLinesListener
			 */
			class VisualLinesListener : public VisualLinesModificationListener {
			private:
				/**
				 * Several visual lines were deleted.
				 * @param lines The range of created lines. @a lines.end() is exclusive
				 * @param sublines The total number of sublines of created lines
				 * @param longestLineChanged Set @c true if the longest line is changed
				 */
				virtual void visualLinesDeleted(const boost::integer_range<Index>& lines,
					Index sublines, bool longestLineChanged) BOOST_NOEXCEPT = 0;
				/**
				 * Several visual lines were inserted.
				 * @param lines The range of inserted lines. @a lines.end() is exclusive
				 */
				virtual void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT = 0;
				friend class LineLayoutVector;
			};
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_VISUAL_LINES_LISTENER_HPP
