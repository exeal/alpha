/**
 * @file line-layout-vector.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2010 was rendering.hpp
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-05-21 separated from rendering.hpp
 */

#ifndef ASCENSION_LINE_LAYOUT_VECTOR_HPP
#define ASCENSION_LINE_LAYOUT_VECTOR_HPP
#include <ascension/corelib/range.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/graphics/font/visual-lines-listener.hpp>
#include <ascension/graphics/geometry/common.hpp>
#include <algorithm>	// std.sort
#include <list>
#include <memory>		// std.unique_ptr
#include <utility>		// std.move
#include <vector>
#include <boost/range/algorithm/find_if.hpp>

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

			class TextLayout;

			/**
			 * Manages a vector of layout (@c TextLayout) and holds the longest line and the number
			 * of the visual lines.
			 * @see TextLayout, TextRenderer
			 * @note This class is not intended to be subclassed.
			 */
			class LineLayoutVector : public kernel::DocumentListener,
				public kernel::DocumentPartitioningListener, private boost::noncopyable {
			public:
				class UseCalculatedLayoutTag : private boost::noncopyable {
					UseCalculatedLayoutTag() BOOST_NOEXCEPT {}
					friend class LineLayoutVector;
				};
				static const UseCalculatedLayoutTag USE_CALCULATED_LAYOUT;

			public:
				// constructors
				template<typename LayoutGenerator>
				LineLayoutVector(kernel::Document& document,
					LayoutGenerator layoutGenerator, Index bufferSize, bool autoRepair);
				~LineLayoutVector() BOOST_NOEXCEPT;

				/// @name Accessors
				/// @{
				const TextLayout& operator[](Index line);
				const TextLayout* at(Index line) const BOOST_NOEXCEPT;
				const TextLayout& at(Index line, const UseCalculatedLayoutTag&);
				std::unique_ptr<const TextLayout> createIsolatedLayout(Index line) const;
				/// @}

				/// @a name Attributes
				/// @{
				const kernel::Document& document() const BOOST_NOEXCEPT;
				Scalar maximumMeasure() const BOOST_NOEXCEPT;
				Index numberOfSublinesOfLine(Index line) const;
				Index numberOfSublinesOfLine(Index line, const UseCalculatedLayoutTag&);
				Index numberOfVisualLines() const BOOST_NOEXCEPT;
				/// @}

				/// @name Listeners
				/// @{
				void addVisualLinesListener(VisualLinesListener& listener);
				void removeVisualLinesListener(VisualLinesListener& listener);
				/// @}

				/// @a name Position Translations
				/// @{
				Index mapLogicalLineToVisualLine(Index line) const;
				Index mapLogicalPositionToVisualPosition(
					const kernel::Position& position, Index* offsetInVisualLine) const;
//				Index mapVisualLineToLogicalLine(Index line, Index* subline) const;
//				kernel::Position mapVisualPositionToLogicalPosition(const kernel::Position& position) const;
				SignedIndex offsetVisualLine(VisualLine& line, SignedIndex offset) const;
				SignedIndex offsetVisualLine(VisualLine& line, SignedIndex offset, const UseCalculatedLayoutTag&);
				/// @}

				/// @name Invalidations
				/// @{
				void invalidate() BOOST_NOEXCEPT;
				template<typename Function> void invalidateIf(Function f);
				void invalidate(const boost::integer_range<Index>& lines);
				/// @}

			protected:
				void invalidate(Index line);
			private:
				struct NumberedLayout : private boost::noncopyable {
					Index lineNumber;
					std::unique_ptr<const TextLayout> layout;
					NumberedLayout() BOOST_NOEXCEPT {}
					NumberedLayout(NumberedLayout&& other) BOOST_NOEXCEPT
						: lineNumber(other.lineNumber), layout(std::move(other.layout)) {}
				};
				void clearCaches(const boost::integer_range<Index>& lines, bool repair);
				void deleteLineLayout(Index line, TextLayout* newLayout = nullptr) BOOST_NOEXCEPT;
				void fireVisualLinesDeleted(const boost::integer_range<Index>& lines, Index sublines);
				void fireVisualLinesInserted(const boost::integer_range<Index>& lines);
				void fireVisualLinesModified(const boost::integer_range<Index>& lines,
					Index newSublines, Index oldSublines, bool documentChanged);
				void initialize();
				void invalidate(const std::vector<Index>& lines);
				SignedIndex offsetVisualLine(VisualLine& line, SignedIndex offset, bool useCalculatedLayout) const;
				void presentationStylistChanged();
				void updateLongestLine(boost::optional<Index> line, Scalar measure) BOOST_NOEXCEPT;
				// kernel.DocumentListener
				void documentAboutToBeChanged(const kernel::Document& document);
				void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
				// kernel.DocumentPartitioningListener
				void documentPartitioningChanged(const kernel::Region& changedRegion);
			private:
				struct GeneratorBase {
					virtual ~GeneratorBase() BOOST_NOEXCEPT {}
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
				std::list<NumberedLayout> layouts_;	// should use GapVector instead?
				const std::size_t bufferSize_;
				const bool autoRepair_;
				enum {ABOUT_TO_CHANGE, CHANGING, NONE} documentChangePhase_;
				boost::optional<boost::integer_range<Index>> pendingCacheClearance_;	// parameters of clearCaches called when document changed
				Scalar maximumMeasure_;
				boost::optional<Index> longestLine_;
				Index numberOfVisualLines_;
				ascension::detail::Listeners<VisualLinesListener> listeners_;
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
			 * @return The layout or @c null if the layout is not cached
			 * @see #oprator[], #at(Index, const UseCalculatedLayoutTag&)
			 */
			inline const TextLayout* LineLayoutVector::at(Index line) const BOOST_NOEXCEPT {
				if(pendingCacheClearance_ && includes(*pendingCacheClearance_, line))
					return nullptr;
				const std::list<NumberedLayout>::const_iterator cached = boost::find_if(layouts_, [line](const NumberedLayout& layout) {
					return layout.lineNumber == line;
				});
				return (cached != boost::end(layouts_)) ? cached->layout.get() : nullptr;
			}

			/**
			 * Returns the layout of the specified line.
			 * @param line The line
			 * @return The layout
			 * @throw IndexOutOfBoundsException @a line is greater than the number of the lines
			 * @see #operator[], #at(Index)
			 */
			inline const TextLayout& LineLayoutVector::at(Index line, const UseCalculatedLayoutTag&) {
				if(line > document().numberOfLines())
					throw IndexOutOfBoundsException("line");
				return (*this)[line];
			}

			/**
			 * Creates and returns an isolated layout for the specified line. This layout is not inserted into the
			 * vector and the instances of @c VisualLinesListener are not invoked.
			 * @param line The line number
			 * @return The layout
			 * @throw IndexOutOfBoundsException @a line is greater than the number of the lines
			 */
			inline std::unique_ptr<const TextLayout> LineLayoutVector::createIsolatedLayout(Index line) const {
				if(line > document().numberOfLines())
					throw IndexOutOfBoundsException("line");
				return layoutGenerator_->generate(line);
			}

			/// Returns the document.
			inline const kernel::Document& LineLayoutVector::document() const BOOST_NOEXCEPT {
				return document_;
			}

			/**
			 * Invalidates all layouts @a pred returns @c true.
			 * @tparam Pred The type of @a pred
			 * @param pred The predicate which takes a parameter of type @c LineLayout and returns
			 *             @c true if invalidates the layout
			 */
			template<typename Pred>
			inline void LineLayoutVector::invalidateIf(Pred pred) {
				std::vector<Index> linesToInvalidate;
				for(std::list<LineLayout>::const_iterator i(std::begin(layouts_)), e(std::end(layouts_)); i != e; ++i) {
					if(pred(*i))
						linesToInvalidate.push_back(i->first);
				}
				if(!linesToInvalidate.empty()) {
					std::sort(linesToInvalidate.begin(), inesToInvalidate.end());
					invalidate(linesToInvalidate);
				}
			}

			/// Returns the measure (inline-progression-dimension) of the longest line in user units.
			inline Scalar LineLayoutVector::maximumMeasure() const BOOST_NOEXCEPT {
				return maximumMeasure_;
			}

			/// Returns the number of the visual lines.
			/// @note This method treats an uncalculated line as single visual line.
			inline Index LineLayoutVector::numberOfVisualLines() const BOOST_NOEXCEPT {
				return numberOfVisualLines_;
			}

			/**
			 * Offsets visual line. The line whose layout not calculated is treat as single visual line.
			 * @param[in,out] line The visual line
			 * @param[in] offset The offset
			 * @return The number of visual lines actually moved
			 * @throw IndexOutOfBoundsException @a line is invalid
			 */
			inline SignedIndex LineLayoutVector::offsetVisualLine(VisualLine& line, SignedIndex offset) const {
				return offsetVisualLine(line, offset, false);
			}

			/**
			 * Offsets visual line.
			 * @param[in,out] line The visual line
			 * @param[in] offset The offset
			 * @return The number of visual lines actually moved
			 * @throw IndexOutOfBoundsException @a line is invalid
			 */
			inline SignedIndex LineLayoutVector::offsetVisualLine(VisualLine& line, SignedIndex offset, const UseCalculatedLayoutTag&) {
				return const_cast<LineLayoutVector*>(this)->offsetVisualLine(line, offset, true);
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
