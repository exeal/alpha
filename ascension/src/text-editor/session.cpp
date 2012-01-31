/**
 * @file session.cpp
 * @author exeal
 * @date 2006-2011
 */

#include <ascension/text-editor/input-sequence-checker.hpp>
#include <ascension/text-editor/kill-ring.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/kernel/searcher.hpp>
#include <algorithm>	// std.find
#include <stdexcept>	// std.invalid_argument
using namespace ascension;
using namespace ascension::texteditor;
using namespace std;


// Session ////////////////////////////////////////////////////////////////////////////////////////

/// Constructor.
Session::Session() /*throw()*/ {
}

/// Destructor.
Session::~Session() /*throw()*/ {
}

/**
 * Adds the document.
 * @param document The document to be added
 * @throw std#invalid_argument @a document is already registered
 */
void Session::addDocument(kernel::Document& document) {
	if(find(documents_.begin(), documents_.end(), &document) != documents_.end())
		throw invalid_argument("The specified document is already registered.");
	documents_.push_back(&document);
	static_cast<detail::SessionElement&>(document).setSession(*this);
}

/// Returns the incremental searcher.
searcher::IncrementalSearcher& Session::incrementalSearcher() /*throw()*/ {
	if(isearch_.get() == nullptr)
		isearch_.reset(new searcher::IncrementalSearcher());
	return *isearch_;
}

/// Returns the incremental searcher.
const searcher::IncrementalSearcher& Session::incrementalSearcher() const /*throw()*/ {
	if(isearch_.get() == nullptr)
		const_cast<Session*>(this)->isearch_.reset(new searcher::IncrementalSearcher());
	return *isearch_;
}

/// Returns the kill ring.
KillRing& Session::killRing() /*throw()*/ {
	return killRing_;
}

/// Returns the kill ring.
const KillRing& Session::killRing() const /*throw()*/ {
	return killRing_;
}

#ifndef ASCENSION_NO_MIGEMO
/**
 * Returns the directory of C/Migemo DLL or dictionary.
 * @param runtime Set @c true to get about DLL, @c false to get about dictionary
 * @return The path name of the directory
 */
const kernel::fileio::PathString& Session::migemoPathName(bool runtime) /*throw()*/ {
	return runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_;
}
#endif // !ASCENSION_NO_MIGEMO

/**
 * Removes the document.
 * @param document The document to be removed
 * @throw std#invalid_argument @a document is not registered
 */
void Session::removeDocument(kernel::Document& document) {
	vector<kernel::Document*>::iterator i = find(documents_.begin(), documents_.end(), &document);
	if(i == documents_.end())
		throw invalid_argument("The specified document is not registered.");
	documents_.erase(i);
}

#ifndef ASCENSION_NO_MIGEMO
/**
 * Sets the directory of C/Migemo DLL or dictionary.
 * @param pathName The path name of the directory
 * @param runtime Set @c true to set about DLL, @c false to set about dictionary
 * @param std#length_error @a @pathName is too long
 */
void Session::setMigemoPathName(const kernel::fileio::PathString& pathName, bool runtime) {
#ifdef ASCENSION_OS_WINDOWS
	if(pathName.length() > MAX_PATH - 1)
#else // ASCENSION_OS_POSIX
	if(pathName.length() > PATH_MAX - 1)
#endif
		throw length_error("pathName");
	(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_) = pathName;
}
#endif // !ASCENSION_NO_MIGEMO

/// Returns the text searcher.
searcher::TextSearcher& Session::textSearcher() /*throw()*/ {
	if(textSearcher_.get() == nullptr)
		textSearcher_.reset(new searcher::TextSearcher());
	return *textSearcher_;
}

/// Returns the text searcher.
const searcher::TextSearcher& Session::textSearcher() const /*throw()*/ {
	if(textSearcher_.get() == nullptr)
		const_cast<Session*>(this)->textSearcher_.reset(new searcher::TextSearcher());
	return *textSearcher_;
}
