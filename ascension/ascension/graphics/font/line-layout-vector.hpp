/**
 * @file line-layout-vector.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010 was rendering.hpp
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-05-21 separated from rendering.hpp
 * @date 2011-2012
 */

#ifndef ASCENSION_LINE_LAYOUT_VECTOR_HPP
#define ASCENSION_LINE_LAYOUT_VECTOR_HPP
#include <ascension/corelib/range.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <algorithm>	// std.sort
#include <list>
#include <memory>		// std.unique_ptr
#include <vector>

namespace ascension {
	namespace graphics {
		namespace font {

			/**
			 * Interface for objects which are interested in getting informed about change of
			 * visual lines of @c LineLayoutVector.
			 * This interface is called by also @c TextViewport. See the documentation of
			 * @c TextViewport class.
			 * @see LineLayoutVector#addVisualLinesListener,
			 *      LineLayoutVector#removeVisualLinesListener,
			 *      TextViewport#addVisualLinesListener, TextViewport#removeVisualLinesListener
			 */
			class VisualLinesListener {
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
				/**
				 * A visual lines were modified.
				 * @param lines The range of modified lines. @a lines.end() is exclusive
				 * @param sublinesDifference The difference of the number of sublines between
				 *        before and after the modification
				 * @param documentChanged Set @c true if the layouts were modified for the document
				 *                        change
				 * @param longestLineChanged Set @c true if the longest line is changed
				 */
				virtual void visualLinesModified(
					const boost::integer_range<Index>& lines, SignedIndex sublinesDifference,
					bool documentChanged, bool longestLineChanged) /*throw()*/ = 0;
				friend class LineLayoutVector;
				friend class TextViewport;
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
					LayoutGenerator layoutGenerator, Index bufferSize, bool autoRepair);
				~LineLayoutVector() /*throw()*/;
				// accessors
				const TextLayout& operator[](Index line) const;
				const TextLayout& at(Index line) const;
				const TextLayout* atIfCached(Index line) const /*throw()*/;
				// attributes
				const kernel::Document& document() const /*throw()*/;
				Scalar maximumMeasure() const /*throw()*/;
				Index numberOfSublinesOfLine(Index line) const;
				Index numberOfVisualLines() const /*throw()*/;
				// listeners
				void addVisualLinesListener(VisualLinesListener& listener);
				void removeVisualLinesListener(VisualLinesListener& listener);
				// position translations
				Index mapLogicalLineToVisualLine(Index line) const;
				Index mapLogicalPositionToVisualPosition(
					const kernel::Position& position, Index* offsetInVisualLine) const;
//				Index mapVisualLineToLogicalLine(Index line, Index* subline) const;
//				kernel::Position mapVisualPositionToLogicalPosition(const kernel::Position& position) const;
				bool offsetVisualLine(VisualLine& line, SignedIndex offset) const /*throw()*/;
				// invalidations
				typedef std::pair<Index, const TextLayout*> LineLayout;
				void invalidate() /*throw()*/;
				template<typename Function> void invalidateIf(Function f);
				void invalidate(const boost::integer_range<Index>& lines);
			protected:
				void invalidate(Index line);
			private:
				typedef std::list<LineLayout>::iterator Iterator;
				void clearCaches(const boost::integer_range<Index>& lines, bool repair);
				void deleteLineLayout(Index line, TextLayout* newLayout = nullptr) /*throw()*/;
				void fireVisualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines);
				void fireVisualLinesInserted(const boost::integer_range<Index>& lines);
				void fireVisualLinesModified(const boost::integer_range<Index>& lines,
					Index newSublines, Index oldSublines, bool documentChanged);
				void initialize();
				void invalidate(const std::vector<Index>& lines);
				void presentationStylistChanged();
				void updateLongestLine(boost::optional<Index> line, Scalar measure) /*throw()*/;
				// kernel.DocumentListener
				void documentAboutToBeChanged(const kernel::Document& document);
				void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
				// kernel.DocumentPartitioningListener
				void documentPartitioningChanged(const kernel::Region& changedRegion);
			private:
				struct GeneratorBase {
					virtual ~GeneratorBase() /*throw()*/ {}
					virtual std::unique_ptr<const TextLayout> generate(Index line) const = 0;
				};
				template<typename Function>
				class Generator : public GeneratorBase {
				public:
					Generator(Function function) : function_(function) {}
					std::unique_ptr<const TextLayout> generate(Index line) const {return function_(line);}
				private:
					const Function function_;
				};
			private:
				kernel::Document& document_;
				std::unique_ptr<GeneratorBase> layoutGenerator_;
				std::list<LineLayout> layouts_;	// should use GapVector instead?
				const std::size_t bufferSize_;
				const bool autoRepair_;
				enum {ABOUT_TO_CHANGE, CHANGING, NONE} documentChangePhase_;
				boost::optional<boost::integer_range<Index>> pendingCacheClearance_;	// parameters of clearCaches called when document changed
				Scalar maximumMeasure_;
				boost::optional<Index> longestLine_;
				Index numberOfVisualLines_;
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
					LayoutGenerator layoutGenerator, Index bufferSize, bool autoRepair) :
					document_(document), layoutGenerator_(new Generator<LayoutGenerator>(layoutGenerator)),
					bufferSize_(bufferSize), autoRepair_(autoRepair), documentChangePhase_(NONE),
					maximumMeasure_(0), numberOfVisualLines_(document.numberOfLines()) {
				initialize();
			}

			/**
			 * Returns the layout of the specified line.
			 * @param line The line
			 * @return The layout
			 * @throw kernel#BadPositionException @a line is greater than the number of the lines
			 * @see #operator[], #atIfCached
			 */
			inline const TextLayout& LineLayoutVector::at(Index line) const {
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
			inline const TextLayout* LineLayoutVector::atIfCached(Index line) const /*throw()*/ {
				if(pendingCacheClearance_ && includes(*pendingCacheClearance_, line))
					return nullptr;
				for(std::list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
					if(i->first == line)
						return i->second;
				}
				return nullptr;
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
				std::vector<Index> linesToInvalidate;
				for(std::list<LineLayout>::const_iterator i(layouts_.begin()), e(layouts_.end()); i != e; ++i) {
					if(pred(*i))
						linesToInvalidate.push_back(i->first);
				}
				if(!linesToInvalidate.empty()) {
					std::sort(linesToInvalidate.begin(), inesToInvalidate.end());
					invalidate(linesToInvalidate);
				}
			}

			/// Returns the measure (inline-progression-dimension) of the longest line.
			inline Scalar LineLayoutVector::maximumMeasure() const /*throw()*/ {
				return maximumMeasure_;
			}

			/**
			 * Returns the number of sublines of the specified line.
			 * If the layout of the line is not calculated, this method returns 1.
			 * @param line The line
			 * @return The count of the sublines
			 * @throw BadPositionException @a line is outside of the document
			 * @see #lineLayout, TextLayout#numberOfLines
			 */
			inline Index LineLayoutVector::numberOfSublinesOfLine(Index line) const /*throw()*/ {
				const TextLayout* const layout = atIfCached(line);
				return (layout != nullptr) ? layout->numberOfLines() : 1;
			}

			/// Returns the number of the visual lines.
			inline Index LineLayoutVector::numberOfVisualLines() const /*throw()*/ {
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
