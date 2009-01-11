/**
 * @file session.cpp
 * @author exeal
 * @date 2006-2009
 */

#include <ascension/session.hpp>
#include <ascension/searcher.hpp>
#include <algorithm>
using namespace ascension;
using namespace ascension::texteditor;
using namespace std;


// KillRing /////////////////////////////////////////////////////////////////

/**
 * @class ascension::texteditor::KillRing
 */

/**
 * Constructor.
 * @param maximumNumberOfKills initial maximum number of kills. this setting can be change by
 * @c #setMaximumNumberOfKills later
 */
KillRing::KillRing(size_t maximumNumberOfKills /* = ASCENSION_DEFAULT_MAXIMUM_KILLS */) /*throw()*/
		: yankPointer_(contents_.end()), maximumNumberOfKills_(maximumNumberOfKills) {
}

/**
 * Registers the listener.
 * @param listener the listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void KillRing::addListener(IKillRingListener& listener) {
	listeners_.add(listener);
}

/**
 * Makes the given content tha latest kill in the kill ring.
 * @param text the content
 * @param rectangle true if the content is a rectangle
 * @param replace set true to replace the front of the kill ring. otherwise the new content will be
 * added
 */
void KillRing::addNew(const String& text, bool rectangle, bool replace /* = false */) {
	if(!contents_.empty() && replace)
		contents_.front() = make_pair(text, rectangle);
	else {
		contents_.push_front(make_pair(text, rectangle));
		if(contents_.size() > maximumNumberOfKills_)
			contents_.pop_back();
	}
	yankPointer_ = contents_.begin();
	listeners_.notify(&IKillRingListener::killRingChanged);
}

/**
 *
 * @param text
 * @param prepend
 */
void KillRing::append(const String& text, bool prepend) {
	if(contents_.empty())
		return addNew(text, false, true);
	else if(!prepend)
		contents_.front().first.append(text);
	else
		contents_.front().first.insert(0, text);
	yankPointer_ = contents_.begin();
	listeners_.notify(&IKillRingListener::killRingChanged);
}

KillRing::Contents::iterator KillRing::at(ptrdiff_t index) const {
	if(contents_.empty())
		throw IllegalStateException("the kill ring is empty.");
	Contents::iterator i(const_cast<KillRing*>(this)->yankPointer_);
	if(index >= 0) {
		for(index -= index - (index % contents_.size()); index > 0; --index) {
			if(++i == contents_.end())
				i = const_cast<KillRing*>(this)->contents_.begin();
		}
	} else {
		for(index += -index -(-index % contents_.size()); index < 0; ++index) {
			if(i == contents_.begin())
				i = const_cast<KillRing*>(this)->contents_.end();
			--i;
		}
	}
	return i;
}

/**
 * Returns the content.
 * @param places
 * @return the content
 * @throw IllegalStateException the kill ring is empty
 */
const pair<String, bool>& KillRing::get(ptrdiff_t places /* = 0 */) const {
	return *at(places);
}

/// Returns the maximum number of kills.
size_t KillRing::maximumNumberOfKills() const /*throw()*/ {
	return maximumNumberOfKills_;
}

/// Returns the number of kills.
size_t KillRing::numberOfKills() const /*throw()*/ {
	return contents_.size();
}

/**
 * Removes the listener.
 * @param listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void KillRing::removeListener(IKillRingListener& listener) {
	listeners_.remove(listener);
}

/**
 * Rotates the yanking point by the given number of places.
 * @param places
 * @return the content
 * @throw IllegalStateException the kill ring is empty
 */
const pair<String, bool>& KillRing::setCurrent(ptrdiff_t places) {
	yankPointer_ = at(places);
	return *yankPointer_;
}


// InputSequenceCheckers ////////////////////////////////////////////////////

/// Destructor.
InputSequenceCheckers::~InputSequenceCheckers() {
	for(list<InputSequenceChecker*>::iterator i = strategies_.begin(); i != strategies_.end(); ++i)
		delete *i;
}

/**
 * Registers the sequence checker.
 * @param checker the sequence checker to be registered.
 * @throw std#invalid_argument @a checker is already registered
 */
void InputSequenceCheckers::add(auto_ptr<InputSequenceChecker> checker) {
	if(find(strategies_.begin(), strategies_.end(), checker.get()) != strategies_.end())
		throw invalid_argument("Specified checker is already registered.");
	strategies_.push_back(checker.release());
}

/**
 * Checks the sequence.
 * @param first start of the string preceding the character to be input
 * @param last end of the string preceding the character to be input
 * @param cp the code point of the character to be input
 * @return true if the input is acceptable
 */
bool InputSequenceCheckers::check(const Char* first, const Char* last, CodePoint cp) const {
	assert(first != 0 && last != 0 && first <= last);
	for(list<InputSequenceChecker*>::const_iterator i = strategies_.begin(); i != strategies_.end(); ++i) {
		if(!(*i)->check(keyboardLayout_, first, last, cp))
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
 * @param keyboardLayout the keyboard layout
 */
void InputSequenceCheckers::setKeyboardLayout(HKL keyboardLayout) /*throw()*/ {
	keyboardLayout_ = keyboardLayout;
}


// Session //////////////////////////////////////////////////////////////////

/// Constructor.
Session::Session() /*throw()*/ : isearch_(0), textSearcher_(0) {
#ifndef ASCENSION_NO_MIGEMO
	wcscpy(migemoRuntimePathName_, L"");
	wcscpy(migemoDictionaryPathName_, L"");
#endif // !ASCENSION_NO_MIGEMO
}

/// Destructor.
Session::~Session() /*throw()*/ {
	delete textSearcher_;
	delete isearch_;
}

/**
 * Adds the document.
 * @param document the document to be added
 * @throw std#invalid_argument @a document is already registered
 */
void Session::addDocument(kernel::Document& document) {
	if(find(documents_.begin(), documents_.end(), &document) != documents_.end())
		throw invalid_argument("The specified document is already registered.");
	documents_.push_back(&document);
	static_cast<internal::ISessionElement&>(document).setSession(*this);
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
 * @param runtime true to get about DLL, false to get about dictionary
 * @return the path name of the directory
 */
const WCHAR* Session::migemoPathName(bool runtime) /*throw()*/ {
	return runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_;
}
#endif // !ASCENSION_NO_MIGEMO

/**
 * Removes the document.
 * @param document the document to be removed
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
 * @param pathName the path name of the directory or @c null
 * @param runtime true to set about DLL, false to set about dictionary
 * @param std#length_error @a @pathName is too long
 */
void Session::setMigemoPathName(const WCHAR* pathName, bool runtime) {
	if(pathName == 0)
		wcscpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, L"");
	else if(wcslen(pathName) > MAX_PATH - 1)
		throw length_error("pathName");
	else
		wcscpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, pathName);
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
