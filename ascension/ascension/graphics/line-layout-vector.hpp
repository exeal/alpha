/**
 * @file line-layout-vector.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010 was rendering.hpp
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-05-21 separated from rendering.hpp
 */

#ifndef ASCENSION_LINE_LAYOUT_VECTOR_HPP
#define ASCENSION_LINE_LAYOUT_VECTOR_HPP
#include <ascension/graphics/text-layout.hpp>
#include <algorithm>	// std.sort
#include <list>
#include <vector>

namespace ascension {
	namespace graphics {
		namespace font {

			/**
			 * Interface for objects which are interested in getting informed about change of
			 * visual lines of @c LineLayoutVector.
			 * @see LineLayoutVector#addVisualLinesListener,
			 *      LineLayoutVector#removeVisualLinesListener
			 */
			class VisualLinesListener {
			private:
				/**
				 * Several visual lines were deleted.
				 * @param first The first of created lines
				 * @param last The last of created lines (exclusive)
				 * @param sublines The total number of sublines of created lines
				 * @param longestLineChanged Set @c true if the longest line is changed
				 */
				virtual void visualLinesDeleted(length_t first, length_t last,
					length_t sublines, bool longestLineChanged) /*throw()*/ = 0;
				/**
				 * Several visual lines were inserted.
				 * @param first The first of inserted lines
				 * @param last The last of inserted lines (exclusive)
				 */
				virtual void visualLinesInserted(length_t first, length_t last) /*throw()*/ = 0;
				/**
				 * A visual lines were modified.
				 * @param first The first of modified lines
				 * @param last The last of modified lines (exclusive)
				 * @param sublinesDifference The difference of the number of sublines between
				 *        before and after the modification
				 * @param documentChanged Set @c true if the layouts were modified for the document
				 *                        change
				 * @param longestLineChanged Set @c true if the longest line is changed
				 */
				virtual void visualLinesModified(
					length_t first, length_t last, signed_length_t sublinesDifference,
					bool documentChanged, bool longestLineChanged) /*throw()*/ = 0;
				friend class LineLayoutVector;
			};

			/**
			 * Manages a vector of layout (@c TextLayout) and holds the longest line and the number
			 * of the visual lines.
			 * @see TextLayout, TextRenderer
			 * @note This class is not intended to be subclassed.
			 */
			class LineLayoutVector : public kernel::DocumentListener, public kernel::DocumentPartitioningListener {
				ASCENSION_NONCOPYABLE_TAG(LineLayoutVector);
			public:
				// constructors
				template<typename LayoutGenerator>
				LineLayoutVector(kernel::Document& document,
					LayoutGenerator layoutGenerator, length_t bufferSize, bool autoRepair);
				~LineLayoutVector() /*throw()*/;
				// accessors
				const TextLayout& operator[](length_t line) const;
				const TextLayout& at(length_t line) const;
				const TextLayout* atIfCached(length_t line) const /*throw()*/;
				// attributes
				const kernel::Document& document() const /*throw()*/;
				Scalar maximumInlineProgressionDimension() const /*throw()*/;
				length_t numberOfSublinesOfLine(length_t) const;
				length_t numberOfVisualLines() const /*throw()*/;
				// listeners
				void addVisualLinesListener(VisualLinesListener& listener);
				void removeVisualLinesListener(VisualLinesListener& listener);
				// position translations
				length_t mapLogicalLineToVisualLine(length_t line) const;
				length_t mapLogicalPositionToVisualPosition(
					const kernel::Position& position, length_t* column) const;
//				length_t mapVisualLineToLogicalLine(length_t line, length_t* subline) const;
//				kernel::Position mapVisualPositionToLogicalPosition(const kernel::Position& position) const;
				void offsetVisualLine(length_t& line, length_t& subline,
					signed_length_t offset, bool* overflowedOrUnderflowed = 0) const /*throw()*/;
				// invalidations
				typedef std::pair<length_t, const TextLayout*> LineLayout;
				void invalidate() /*throw()*/;
				template<typename Function> void invalidateIf(Function f);
				void invalidate(length_t first, length_t last);
			protected:
				void invalidate(length_t line);
			private:
				typedef std::list<LineLayout>::iterator Iterator;
				void clearCaches(length_t first, length_t last, bool repair);
				void deleteLineLayout(length_t line, TextLayout* newLayout = 0) /*throw()*/;
				void fireVisualLinesDeleted(length_t first, length_t last, length_t sublines);
				void fireVisualLinesInserted(length_t first, length_t last);
				void fireVisualLinesModified(length_t first, length_t last,
					length_t newSublines, length_t oldSublines, bool documentChanged);
				void initialize();
				void invalidate(const std::vector<length_t>& lines);
				void presentationStylistChanged();
				void updateLongestLine(length_t line, Scalar ipd) /*throw()*/;
				// kernel.DocumentListener
				void documentAboutToBeChanged(const kernel::Document& document);
				void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
				// kernel.DocumentPartitioningListener
				void documentPartitioningChanged(const kernel::Region& changedRegion);
			private:
				struct GeneratorBase {
					virtual ~GeneratorBase() /*throw()*/ {}
					virtual std::auto_ptr<const TextLayout> generate(length_t line) const = 0;
				};
				template<typename Function>
				class Generator : public GeneratorBase {
				public:
					Generator(Function function) : function_(function) {}
					std::auto_ptr<const TextLayout> generate(length_t line) const {return function_(line);}
				private:
					const Function function_;
				};
			private:
				kernel::Document& document_;
				std::auto_ptr<GeneratorBase> layoutGenerator_;
				std::list<LineLayout> layouts_;	// should use GapVector instead?
				const std::size_t bufferSize_;
				const bool autoRepair_;
				enum {ABOUT_TO_CHANGE, CHANGING, NONE} documentChangePhase_;
				struct {
					length_t first, last;
				} pendingCacheClearance_;	// ドキュメント変更中に呼び出された clearCaches の引数
				Scalar maximumIpd_;
				length_t longestLine_, numberOfVisualLines_;
				detail::Listeners<VisualLinesListener> listeners_;
			};


			/**
			 * Constructor.
			 * @tparam LayoutGenerator The type of @a layoutGenerator
			 * @param document The document
			 * @param layoutGenerator The function object generates the layout for the requested
			 *                        line. This object should be called with one parameter means
			 *                        the line number and return @c std#auto&lt;TextLayout&gt;
			 *                        object means the line layout
			 * @param bufferSize The maximum number of lines cached
			 * @param autoRepair Set @c true to repair disposed layout automatically if the line
			 *                   number of its line was not changed
			 * @throw std#invalid_argument @a bufferSize is zero
			 */
			template<typename LayoutGenerator>
			inline LineLayoutVector::LineLayoutVector(kernel::Document& document,
					LayoutGenerator layoutGenerator, length_t bufferSize, bool autoRepair) :
					document_(document), layoutGenerator_(new Generator<LayoutGenerator>(layoutGenerator)),
					bufferSize_(bufferSize), autoRepair_(autoRepair), documentChangePhase_(NONE),
					maximumIpd_(0), longestLine_(INVALID_INDEX), numberOfVisualLines_(document.numberOfLines()) {
				initialize();
			}

			/**
			 * Returns the layout of the specified line.
			 * @param line The line
			 * @return The layout
			 * @throw kernel#BadPositionException @a line is greater than the number of the lines
			 * @see #operator[], #atIfCached
			 */
			inline const TextLayout& LineLayoutVector::at(length_t line) const {
				if(line > document().numberOfLines())
					throw kernel::BadPositionException(kernel::Position(line, 0));
				return (*this)[line];
			}

			/**
			 * Returns the layout of the specified line.
			 * @param line The line
			 * @return The layout or @c null if the layout is not cached
			 * @see #oprator[], #at
			 */
			inline const TextLayout* LineLayoutVector::atIfCached(length_t line) const /*throw()*/ {
				if(pendingCacheClearance_.first != INVALID_INDEX
						&& line >= pendingCacheClearance_.first && line < pendingCacheClearance_.last)
					return 0;
				for(std::list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
					if(i->first == line)
						return i->second;
				}
				return 0;
			}

			/// Returns the document.
			inline const kernel::Document& LineLayoutVector::document() const /*throw()*/ {
				return document_;
			}

			/**
			 * Invalidates all layouts @a pred returns @c true.
			 * @tparam Pred The type of @a pred
			 * @param pred The predicate which takes a parameter of type @c LineLayout and returns
			 *             @c true if invalidates the layout
			 */
			template<typename Pred>
			inline void LineLayoutVector::invalidateIf(Pred pred) /*throw()*/ {
				std::vector<length_t> linesToInvalidate;
				for(std::list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
					if(pred(*i))
						linesToInvalidate.push_back(i->first);
				}
				if(!linesToInvalidate.empty()) {
					std::sort(linesToInvalidate.begin(), inesToInvalidate.end());
					invalidate(linesToInvalidate);
				}
			}

			/// Returns the width of the longest line.
			inline Scalar LineLayoutVector::maximumInlineProgressionDimension() const /*throw()*/ {
				return maximumIpd_;
			}

			/**
			 * Returns the number of sublines of the specified line.
			 * If the layout of the line is not calculated, this method returns 1.
			 * @param line The line
			 * @return The count of the sublines
			 * @throw BadPositionException @a line is outside of the document
			 * @see #lineLayout, TextLayout#numberOfLines
			 */
			inline length_t LineLayoutVector::numberOfSublinesOfLine(length_t line) const /*throw()*/ {
				const TextLayout* const layout = atIfCached(line);
				return (layout != 0) ? layout->numberOfLines() : 1;
			}

			/// Returns the number of the visual lines.
			inline length_t LineLayoutVector::numberOfVisualLines() const /*throw()*/ {
				return numberOfVisualLines_;
			}

			/**
			 * Removes the visual lines listener.
			 * @param listener The listener to be removed
			 * @throw std#invalid_argument @a listener is not registered
			 */
			inline void LineLayoutVector::removeVisualLinesListener(VisualLinesListener& listener) {
				listeners_.remove(listener);
			}

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_LINE_LAYOUT_VECTOR_HPP
