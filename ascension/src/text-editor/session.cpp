/**
 * @file session.cpp
 * @author exeal
 * @date 2006-2011
 */

#include <ascension/text-editor/kill-ring.hpp>
#include <ascension/text-editor/session.hpp>
#include <ascension/kernel/searcher.hpp>
#include <algorithm>
using namespace ascension;
using namespace ascension::texteditor;
using namespace std;


// InputSequenceCheckers //////////////////////////////////////////////////////////////////////////

/// Destructor.
InputSequenceCheckers::~InputSequenceCheckers() {
	for(list<InputSequenceChecker*>::iterator i = strategies_.begin(); i != strategies_.end(); ++i)
		delete *i;
}

/**
 * Registers the sequence checker.
 * @param checker The sequence checker to be registered.
 * @throw std#invalid_argument @a checker is already registered
 */
void InputSequenceCheckers::add(auto_ptr<InputSequenceChecker> checker) {
	if(find(strategies_.begin(), strategies_.end(), checker.get()) != strategies_.end())
		throw invalid_argument("Specified checker is already registered.");
	strategies_.push_back(checker.release());
}

/**
 * Checks the sequence.
 * @param preceding The string preceding the character to be input
 * @param c The code point of the character to be input
 * @return true if the input is acceptable
 * @throw NullPointerException @a receding is @c null
 */
bool InputSequenceCheckers::check(const StringPiece& preceding, CodePoint c) const {
	if(preceding.beginning() == 0 || preceding.end() == 0)
		throw NullPointerException("preceding");
	for(list<InputSequenceChecker*>::const_iterator i = strategies_.begin(); i != strategies_.end(); ++i) {
		if(!(*i)->check(keyboardLayout_, preceding, c))
			return false;
	}
	return true;
}

/// Removes all registered checkers.
void InputSequenceCheckers::clear() {
	for(list<InputSequenceChecker*>::iterator i = strategies_.begin(); i != strategies_.end(); ++i)
		delete *i;
	strategies_.clear();
}

/// Returns if no checker is registerd.
bool InputSequenceCheckers::isEmpty() const /*throw()*/ {
	return strategies_.empty();
}

/**
 * Activates the specified keyboard layout.
 * @param keyboardLayout The keyboard layout
 */
void InputSequenceCheckers::setKeyboardLayout(HKL keyboardLayout) /*throw()*/ {
	keyboardLayout_ = keyboardLayout;
}


// Session ////////////////////////////////////////////////////////////////////////////////////////

/// Constructor.
Session::Session() /*throw()*/ : isearch_(0), textSearcher_(0) {
#ifndef ASCENSION_NO_MIGEMO
#ifdef ASCENSION_OS_WINDOWS
	wcscpy(migemoRuntimePathName_, L"");
	wcscpy(migemoDictionaryPathName_, L"");
#else // ASCENSION_OS_POSIX
	strcpy(migemoRuntimePathName_, "");
	strcpy(migemoDictionaryPathName_, "");
#endif
#endif // !ASCENSION_NO_MIGEMO
}

/// Destructor.
Session::~Session() /*throw()*/ {
	delete textSearcher_;
	delete isearch_;
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
	if(isearch_ == 0)
		isearch_ = new searcher::IncrementalSearcher();
	return *isearch_;
}

/// Returns the incremental searcher.
const searcher::IncrementalSearcher& Session::incrementalSearcher() const /*throw()*/ {
	if(isearch_ == 0)
		const_cast<searcher::IncrementalSearcher*&>(isearch_) = new searcher::IncrementalSearcher();
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
const kernel::fileio::PathCharacter* Session::migemoPathName(bool runtime) /*throw()*/ {
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
 * @param pathName The path name of the directory or @c null
 * @param runtime Set @c true to set about DLL, @c false to set about dictionary
 * @param std#length_error @a @pathName is too long
 */
void Session::setMigemoPathName(const kernel::fileio::PathCharacter* pathName, bool runtime) {
	if(pathName == 0)
#ifdef ASCENSION_OS_WINDOWS
		wcscpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, L"");
	else if(wcslen(pathName) > MAX_PATH - 1)
#else // ASCENSION_OS_POSIX
		strcpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, "");
	else if(strlen(pathName) > MAX_PATH - 1)
#endif
		throw length_error("pathName");
	else
#ifdef ASCENSION_OS_WINDOWS
		wcscpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, pathName);
#else // ASCENSION_OS_POSIX
		strcpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, pathName);
#endif
}
#endif // !ASCENSION_NO_MIGEMO

/// Returns the text searcher.
searcher::TextSearcher& Session::textSearcher() /*throw()*/ {
	if(textSearcher_ == 0)
		textSearcher_ = new searcher::TextSearcher();
	return *textSearcher_;
}

/// Returns the text searcher.
const searcher::TextSearcher& Session::textSearcher() const /*throw()*/ {
	if(textSearcher_ == 0)
		const_cast<searcher::TextSearcher*&>(textSearcher_) = new searcher::TextSearcher();
	return *textSearcher_;
}
