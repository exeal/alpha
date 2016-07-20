/**
 * @file bookmarker.hpp
 * Defines @c Bookmarker class.
 * @author exeal
 * @date 2003-2006 (was EditDoc.h)
 * @date 2006-2012, 2014-2016
 * @date 2016-07-20 Separated from document.hpp.
 */

#ifndef ASCENSION_BOOKMARKER_HPP
#define ASCENSION_BOOKMARKER_HPP
#include <ascension/direction.hpp>
#include <ascension/corelib/detail/gap-vector.hpp>
#include <ascension/corelib/detail/listeners.hpp>
#include <ascension/kernel/document-exceptions.hpp>
#include <ascension/kernel/document-observers.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace kernel {
		/**
		 * Interface for objects which are interested in getting informed about change of bookmarks of the document.
		 * @see Bookmarker, Bookmarker#addListener, Bookmarker#removeListener
		 */
		class BookmarkListener {
		private:
			/**
			 * The bookmark on @a line was set or removed. Note that this is not called when the bookmarks were changed
			 * by the document's change.
			 * @param line The line number
			 */
			virtual void bookmarkChanged(Index line) = 0;
			/// All bookmarks were removed.
			virtual void bookmarkCleared() = 0;
			friend class Bookmarker;
		};

		/**
		 * A @c Bookmarker manages bookmarks of the document.
		 * @note This class is not intended to be subclassed.
		 * @see Document#bookmarker, locations#nextBookmark
		 */
		class Bookmarker : private DocumentListener, private boost::noncopyable {
		public:
			/// A @c Bookmarker#Iterator enumerates the all marked lines.
			class Iterator : public boost::iterators::iterator_facade<
				Iterator, Index, boost::bidirectional_traversal_tag, Index, std::ptrdiff_t> {
			private:
				Iterator(ascension::detail::GapVector<Index>::const_iterator impl) : impl_(impl) {}
				ascension::detail::GapVector<Index>::const_iterator impl_;
				// boost.iterators.iterator_facade requirements
				friend class boost::iterators::iterator_core_access;
				void decrement() {--impl_;}
				value_type dereference() const {return *impl_;}
				bool equal(const Iterator& other) const {return impl_ == other.impl_;}
				void increment() {++impl_;}
//				bool less(const Iterator& other) const {return impl_ < other.impl_;}
				friend class Bookmarker;
			};

		public:
			~Bookmarker() BOOST_NOEXCEPT;

			/// @name Marks
			/// @{
			void clear() BOOST_NOEXCEPT;
			bool isMarked(Index line) const;
			void mark(Index line, bool set = true);
			std::size_t numberOfMarks() const BOOST_NOEXCEPT;
			void toggle(Index line);
			/// @}

			/// @name Enumerations
			/// @{
			Iterator begin() const;
			Iterator end() const;
			boost::optional<Index> next(Index from, Direction direction, bool wrapAround = true, std::size_t marks = 1) const;
			/// @}

			/// @name Listeners
			/// @{
			void addListener(BookmarkListener& listener);
			void removeListener(BookmarkListener& listener);
			/// @}

		private:
			ascension::detail::GapVector<Index>::iterator find(Index line) const BOOST_NOEXCEPT;
			// DocumentListener
			void documentAboutToBeChanged(const Document& document);
			void documentChanged(const Document& document, const DocumentChange& change);
		private:
			explicit Bookmarker(Document& document) BOOST_NOEXCEPT;
			Document& document_;
			ascension::detail::GapVector<Index> markedLines_;
			ascension::detail::Listeners<BookmarkListener> listeners_;
			friend class Document;
		};
	}
} // namespace ascension.kernel

#endif // !ASCENSION_BOOKMARKER_HPP
