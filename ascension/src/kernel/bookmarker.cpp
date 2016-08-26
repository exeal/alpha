/**
 * @file bookmarker.cpp
 * Implements @c Bookmarker class.
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2016
 * @date 2016-07-20 Separated from document.cpp.
 */

#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document.hpp>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <boost/range/algorithm/lower_bound.hpp>


namespace ascension {
	namespace kernel {
		/**
		 * Private constructor.
		 * @param document The document
		 */
		Bookmarker::Bookmarker(Document& document) BOOST_NOEXCEPT : document_(document) {
			document.addListener(*this);
		}

		/// Destructor.
		Bookmarker::~Bookmarker() BOOST_NOEXCEPT {
			document_.removeListener(*this);
		}

		/**
		 * Registers the listener.
		 * @param listener The listener to be registered
		 * @throw std#invalid_argument @a listener is already registered
		 */
		void Bookmarker::addListener(BookmarkListener& listener) {
			listeners_.add(listener);
		}

		/**
		 * Returns a bidirectional iterator to addresses the first marked line.
		 * @see #end, #next
		 */
		Bookmarker::Iterator Bookmarker::begin() const {
			return Iterator(std::begin(markedLines_));
		}

		/// Deletes all bookmarks.
		void Bookmarker::clear() BOOST_NOEXCEPT {
			if(!boost::empty(markedLines_)) {
				markedLines_.clear();
				listeners_.notify(&BookmarkListener::bookmarkCleared);
			}
		}

		/// @see DocumentListener#documentAboutToBeChanged
		void Bookmarker::documentAboutToBeChanged(const Document&) {
			// do nothing
		}

		/// @see DocumentListener#documentChanged
		void Bookmarker::documentChanged(const Document& document, const DocumentChange& change) {
			// update markedLines_ based on the change
			if(&document_ != &document || markedLines_.empty())
				return;
			if(boost::size(change.erasedRegion().lines()) > 1) {
				const auto& region = change.erasedRegion();
				Index firstLine = line(*boost::const_begin(region));
				if(offsetInLine(*boost::const_begin(region)) > 0)
					++firstLine;
				const auto firstMarkedLine(boost::lower_bound(markedLines_, firstLine));
				const auto e(boost::const_end(markedLines_));
				if(firstMarkedLine != e) {
					const Index lastLine = line(*boost::const_end(region)) + 1;
					const auto lastMarkedLine(boost::lower_bound(markedLines_, lastLine));
					// slide the marks on lines after the erased
					const auto nlines = boost::size(change.erasedRegion().lines()) - 1;
					for(auto i(lastMarkedLine); i != e; ++i)
						*i -= nlines;
					// remove the marks on erased lines
					markedLines_.erase(firstMarkedLine, lastMarkedLine);
				}
			}
			if(boost::size(change.insertedRegion().lines()) > 1) {
				const Index lines = boost::size(change.insertedRegion().lines()) - 1;
				auto i(find(line(*boost::const_begin(change.insertedRegion()))));
				if(i != std::end(markedLines_)) {
					if(*i == line(*boost::const_begin(change.insertedRegion())) && offsetInLine(*boost::const_begin(change.insertedRegion())) != 0)
						++i;
					for(const auto e(std::end(markedLines_)); i != e; ++i)
						*i += lines;	// ??? - C4267@MSVC9
				}
			}
		}

		/**
		 * Returns a bidirectional iterator to addresses just beyond the last marked line.
		 * @see #begin, #next
		 */
		Bookmarker::Iterator Bookmarker::end() const {
			return Iterator(std::end(markedLines_));
		}

		inline ascension::detail::GapVector<Index>::iterator Bookmarker::find(Index line) const BOOST_NOEXCEPT {
			// TODO: can write faster implementation (and design) by internal.searchBound().
			Bookmarker& self = const_cast<Bookmarker&>(*this);
			return boost::lower_bound(self.markedLines_, line);
		}

		/**
		 * Returns @c true if the specified line is bookmarked.
		 * @param line The line
		 * @throw BadPositionException @a line is outside of the document
		 */
		bool Bookmarker::isMarked(Index line) const {
			if(line >= document_.numberOfLines())
				throw BadPositionException(Position::bol(line));
			const auto i(find(line));
			return i != std::end(markedLines_) && *i == line;
		}

		/**
		 * Sets or clears the bookmark of the specified line.
		 * @param line The line
		 * @param set @c true to set bookmark, @c false to clear
		 * @throw BadPositionException @a line is outside of the document
		 */
		void Bookmarker::mark(Index line, bool set) {
			if(line >= document_.numberOfLines())
				throw BadPositionException(Position::bol(line));
			const auto i(find(line));
			if(i != std::end(markedLines_) && *i == line) {
				if(!set) {
					markedLines_.erase(i);
					listeners_.notify<Index>(&BookmarkListener::bookmarkChanged, line);
				}
			} else {
				if(set) {
					markedLines_.insert(i, line);
					listeners_.notify<Index>(&BookmarkListener::bookmarkChanged, line);
				}
			}
		}

		/**
		 * Returns the line number of the next/previous bookmarked line.
		 * @param from The start line number to search
		 * @param direction The direction to search
		 * @param wrapAround Set @c true to enable "wrapping around". If set, this method starts again from the end (in
		 *                   forward case) or beginning (in backward case) of the document when the reached the end or
		 *                   beginning of the document
		 * @param marks The number of marks to scan
		 * @return The next bookmarked line or @c boost#none if not found. If @a marks is zero, this method returns
		 *         @a from if @a from is bookmarked, or @c boost#none otherwise
		 * @throw BadPositionException @a line is outside of the document
		 * @see #begin, #end
		 */
		boost::optional<Index> Bookmarker::next(Index from, Direction direction, bool wrapAround /* = true */, std::size_t marks /* = 1 */) const {
			// this code is tested by 'test/document-test.cpp'
			if(from >= document_.numberOfLines())
				throw BadPositionException(Position::bol(from));
			else if(boost::empty(markedLines_))
				return boost::none;
			else if(marks == 0u)
				return isMarked(from) ? boost::make_optional(from) : boost::none;

			else if(marks > numberOfMarks()) {
				if(!wrapAround)
					return boost::none;
				marks = marks % numberOfMarks();
				if(marks == 0u)
					marks = numberOfMarks();
			}

			std::size_t i = static_cast<ascension::detail::GapVector<Index>::const_iterator>(find(from)) - std::begin(markedLines_);
			if(direction == Direction::forward()) {
				if(i == numberOfMarks()) {
					if(!wrapAround)
						return boost::none;
					i = 0;
					--marks;
				} else if(markedLines_[i] != from)
					--marks;
				if((i += marks) >= numberOfMarks()) {
					if(wrapAround)
						i -= numberOfMarks();
					else
						return boost::none;
				}
			} else {
				if(i < marks) {
					if(wrapAround)
						i += numberOfMarks();
					else
						return boost::none;
				}
				i -= marks;
			}
			return markedLines_[i];
		}

		/// Returns the number of the lines bookmarked.
		std::size_t Bookmarker::numberOfMarks() const BOOST_NOEXCEPT {
			return boost::size(markedLines_);
		}

		/**
		 * Removes the listener.
		 * @param listener The listener to be removed
		 * @throw std#invalid_argument @a listener is not registered
		 */
		void Bookmarker::removeListener(BookmarkListener& listener) {
			listeners_.remove(listener);
		}

		/**
		 * Toggles the bookmark of the spcified line.
		 * @param line The line
		 * @throw BadPositionException @a line is outside of the document
		 */
		void Bookmarker::toggle(Index line) {
			if(line >= document_.numberOfLines())
				throw BadPositionException(Position::bol(line));
			const auto i(find(line));
			if(i == std::end(markedLines_) || *i != line)
				markedLines_.insert(i, line);
			else
				markedLines_.erase(i);
			listeners_.notify<Index>(&BookmarkListener::bookmarkChanged, line);
		}
	}
}
