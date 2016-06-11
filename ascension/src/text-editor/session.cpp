/**
 * @file session.cpp
 * @author exeal
 * @date 2006-2015
 */

#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/text-editor/kill-ring.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/kernel/searcher.hpp>
#include <boost/range/algorithm/find.hpp>
#include <stdexcept>	// std.invalid_argument

namespace ascension {
	namespace texteditor {
		// Session ////////////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Adds the document.
		 * @param document The document to be added
		 * @throw std#invalid_argument @a document is already registered
		 */
		void Session::addDocument(kernel::Document& document) {
			if(boost::range::find(documents_, &document) != boost::end(documents_))
				throw std::invalid_argument("The specified document is already registered.");
			documents_.push_back(&document);
			static_cast<detail::SessionElement&>(document).setSession(*this);
		}

		/// Returns the incremental searcher.
		searcher::IncrementalSearcher& Session::incrementalSearcher() BOOST_NOEXCEPT {
			if(isearch_.get() == nullptr)
				isearch_.reset(new searcher::IncrementalSearcher());
			return *isearch_;
		}

		/// Returns the incremental searcher.
		const searcher::IncrementalSearcher& Session::incrementalSearcher() const BOOST_NOEXCEPT {
			if(isearch_.get() == nullptr)
				const_cast<Session*>(this)->isearch_.reset(new searcher::IncrementalSearcher());
			return *isearch_;
		}

		/// Returns the kill ring.
		KillRing& Session::killRing() BOOST_NOEXCEPT {
			return killRing_;
		}

		/// Returns the kill ring.
		const KillRing& Session::killRing() const BOOST_NOEXCEPT {
			return killRing_;
		}

#ifndef ASCENSION_NO_MIGEMO
		/**
		 * Returns the directory of C/Migemo dictionary.
		 * @return The path name of the directory
		 * @see #migemoLibraryPathName, #setMigemoDictionaryPathName
		 */
		const boost::filesystem::path& Session::migemoDictionaryPathName() BOOST_NOEXCEPT {
			return migemoDictionaryPathName_;
		}

		/**
		 * Returns the directory of C/Migemo library.
		 * @return The path name of the directory
		 * @see #migemoDictionaryPathName, #setMigemoLibraryPathName
		 */
		const boost::filesystem::path& Session::migemoLibraryPathName() BOOST_NOEXCEPT {
			return migemoLibraryPathName_;
		}
#endif // !ASCENSION_NO_MIGEMO

		/**
		 * Removes the document.
		 * @param document The document to be removed
		 * @throw std#invalid_argument @a document is not registered
		 */
		void Session::removeDocument(kernel::Document& document) {
			std::vector<kernel::Document*>::iterator i(boost::range::find(documents_, &document));
			if(i == boost::end(documents_))
				throw std::invalid_argument("The specified document is not registered.");
			documents_.erase(i);
		}

#ifndef ASCENSION_NO_MIGEMO
		/**
		 * Sets the directory of C/Migemo dictionary.
		 * This method does not check if the specified path is exist.
		 * @param pathName The path name of the directory
		 * @throw std#length_error @a pathName is too long
		 */
		void Session::setMigemoDictionaryPathName(const boost::filesystem::path& pathName) {
#if 0
#ifdef BOOST_OS_WINDOWS
			if(pathName.length() > MAX_PATH - 1)
#else // ASCENSION_OS_POSIX
			if(pathName.length() > PATH_MAX - 1)
#endif
#endif
				throw std::length_error("pathName");
			migemoDictionaryPathName_ = pathName;
		}

		/**
		 * Sets the directory of C/Migemo library.
		 * This method does not check if the specified path is exist.
		 * @param pathName The path name of the directory
		 * @throw std#length_error @a pathName is too long
		 */
		void Session::setMigemoLibraryPathName(const boost::filesystem::path& pathName) {
#if 0
#ifdef BOOST_OS_WINDOWS
			if(pathName.length() > MAX_PATH - 1)
#else // ASCENSION_OS_POSIX
			if(pathName.length() > PATH_MAX - 1)
#endif
#endif
				throw std::length_error("pathName");
			migemoLibraryPathName_ = pathName;
		}
#endif // !ASCENSION_NO_MIGEMO

		/// Returns the text searcher.
		searcher::TextSearcher& Session::textSearcher() BOOST_NOEXCEPT {
			if(textSearcher_.get() == nullptr)
				textSearcher_.reset(new searcher::TextSearcher());
			return *textSearcher_;
		}

		/// Returns the text searcher.
		const searcher::TextSearcher& Session::textSearcher() const BOOST_NOEXCEPT {
			if(textSearcher_.get() == nullptr)
				const_cast<Session*>(this)->textSearcher_.reset(new searcher::TextSearcher());
			return *textSearcher_;
		}

		/**
		 * Calls @c IncrementalSearcher#abort from the specified document.
		 * @param document The document. If this does not have the session, this method does nothing
		 * @return true if the incremental search was running
		 * @see endIncrementalSearch, searcher#IncrementalSearcher
		 */
		bool abortIncrementalSearch(kernel::Document& document) BOOST_NOEXCEPT {
			if(Session* const session = document.session()) {
				if(session->incrementalSearcher().isRunning())
					return session->incrementalSearcher().abort(), true;
			}
			return false;
		}

		/**
		 * Calls @c IncrementalSearcher#end from the specified document.
		 * @param document The document. If this does not have the session, this method does nothing
		 * @return true if the incremental search was running
		 * @see abortIncrementalSearch, searcher#IncrementalSearcher
		 */
		bool endIncrementalSearch(kernel::Document& document) BOOST_NOEXCEPT {
			if(Session* const session = document.session()) {
				if(session->incrementalSearcher().isRunning())
					return session->incrementalSearcher().end(), true;
			}
			return false;
		}
	}
}
