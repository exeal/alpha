/**
 * @file session.cpp
 * @author exeal
 * @date 2006-2013
 */

#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/text-editor/kill-ring.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/kernel/searcher.hpp>
#include <boost/range/algorithm/find.hpp>
#include <stdexcept>	// std.invalid_argument
using namespace ascension;
using namespace ascension::texteditor;
using namespace std;


// Session ////////////////////////////////////////////////////////////////////////////////////////

/**
 * Adds the document.
 * @param document The document to be added
 * @throw std#invalid_argument @a document is already registered
 */
void Session::addDocument(kernel::Document& document) {
	if(boost::range::find(documents_, &document) != boost::end(documents_))
		throw invalid_argument("The specified document is already registered.");
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
const kernel::fileio::PathString& Session::migemoDictionaryPathName() BOOST_NOEXCEPT {
	return migemoDictionaryPathName_;
}

/**
 * Returns the directory of C/Migemo library.
 * @return The path name of the directory
 * @see #migemoDictionaryPathName, #setMigemoLibraryPathName
 */
const kernel::fileio::PathString& Session::migemoLibraryPathName() BOOST_NOEXCEPT {
	return migemoLibraryPathName_;
}
#endif // !ASCENSION_NO_MIGEMO

/**
 * Removes the document.
 * @param document The document to be removed
 * @throw std#invalid_argument @a document is not registered
 */
void Session::removeDocument(kernel::Document& document) {
	vector<kernel::Document*>::iterator i(boost::range::find(documents_, &document));
	if(i == boost::end(documents_))
		throw invalid_argument("The specified document is not registered.");
	documents_.erase(i);
}

#ifndef ASCENSION_NO_MIGEMO
/**
 * Sets the directory of C/Migemo dictionary.
 * This method does not check if the specified path is exist.
 * @param pathName The path name of the directory
 * @param std#length_error @a @pathName is too long
 */
void Session::setMigemoDictionaryPathName(const kernel::fileio::PathString& pathName) {
#ifdef ASCENSION_OS_WINDOWS
	if(pathName.length() > MAX_PATH - 1)
#else // ASCENSION_OS_POSIX
	if(pathName.length() > PATH_MAX - 1)
#endif
		throw length_error("pathName");
	migemoDictionaryPathName_ = pathName;
}

/**
 * Sets the directory of C/Migemo library.
 * This method does not check if the specified path is exist.
 * @param pathName The path name of the directory
 * @param std#length_error @a @pathName is too long
 */
void Session::setMigemoLibraryPathName(const kernel::fileio::PathString& pathName) {
#ifdef ASCENSION_OS_WINDOWS
	if(pathName.length() > MAX_PATH - 1)
#else // ASCENSION_OS_POSIX
	if(pathName.length() > PATH_MAX - 1)
#endif
		throw length_error("pathName");
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
