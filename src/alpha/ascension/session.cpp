/**
 * @file session.cpp
 * @author exeal
 * @date 2006-2007
 */

#include "stdafx.h"
#include "session.hpp"
#include "searcher.hpp"
#include <algorithm>
using namespace ascension;
using namespace ascension::texteditor;
using namespace std;


// ClipboardRing ////////////////////////////////////////////////////////////

/// Constructor.
ClipboardRing::ClipboardRing() throw() : capacity_(16), maxBytes_(100 * 1024), activeItem_(static_cast<size_t>(-1)) {
}

/**
 * Adds the new text to the ring.
 *
 * If the count of texts is over the limit, the oldest content is deleted.
 *
 * If the specified text is too long, listeners' IClipboardRingListener#clipboardRingAddingDenied are invoked.
 * @param text the text to be added. this can't be empty
 * @param box true if the text is rectangle
 */
void ClipboardRing::add(const String& text, bool box) {
	assert(!text.empty());
	if(text.length() * sizeof(Char) > maxBytes_) {
		listeners_.notify(IClipboardRingListener::clipboardRingAddingDenied);
		return;
	}

	ClipText ct;
	ct.text = text;
	ct.box = box;
	datas_.push_front(ct);
	if(datas_.size() > capacity_)
		datas_.pop_back();
	activeItem_ = 0;
	listeners_.notify(IClipboardRingListener::clipboardRingChanged);
}

/**
 * Returns the content of the specified index.
 * @param index the index
 * @param[out] text	the text content
 * @param[out] box true if the text is rectangle
 * @throw std#out_of_range @a index is out of range
 */
void ClipboardRing::getText(size_t index, String& text, bool& box) const {
	if(index >= datas_.size())
		throw out_of_range("the specified index is out of range.");
	list<ClipText>::const_iterator it = datas_.begin();
	for(size_t i = 0; i < index; ++i, ++it);
	text = it->text;
	box = it->box;
}

/**
 * Removes the specified text.
 * @param index the index of the text
 * @throw std#out_of_range	@a index is invalid
 */
void ClipboardRing::remove(size_t index) {
	if(index >= datas_.size())
		throw out_of_range("the specified index is out of range.");
	list<ClipText>::iterator it = datas_.begin();
	for(size_t i = 0; i < index; ++i, ++it);
	datas_.erase(it);
	if(index == datas_.size() && index == activeItem_)
		--activeItem_;
	listeners_.notify(IClipboardRingListener::clipboardRingChanged);
}

/// Removes all the stored texts.
void ClipboardRing::removeAll() {
	datas_.clear();
	listeners_.notify(IClipboardRingListener::clipboardRingChanged);
}

/**
 * Sets the number of texts than the ring can contain.
 *
 * If the specified capacity is less than the current one, the contents closest to the end are removed.
 * @param capacity the new capacity
 * @throw std#invalid_argument @a capacity is zero
 */
void ClipboardRing::setCapacity(size_t capacity) {
	if(capacity == 0)
		throw invalid_argument("the capacity must not be zero.");
	capacity_ = capacity;
	if(datas_.size() > capacity_) {
		datas_.resize(capacity_);
		listeners_.notify(IClipboardRingListener::clipboardRingChanged);
	}
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
bool InputSequenceCheckers::isEmpty() const throw() {
	return strategies_.empty();
}

/**
 * Activates the specified keyboard layout.
 * @param keyboardLayout the keyboard layout
 */
void InputSequenceCheckers::setKeyboardLayout(HKL keyboardLayout) throw() {
	keyboardLayout_ = keyboardLayout;
}


// Session //////////////////////////////////////////////////////////////////

/// Constructor.
Session::Session() throw() : isearch_(0), textSearcher_(0) {
#ifndef ASCENSION_NO_MIGEMO
	wcscpy(migemoRuntimePathName_, L"");
	wcscpy(migemoDictionaryPathName_, L"");
#endif /* !ASCENSION_NO_MIGEMO */
}

/// Destructor.
Session::~Session() throw() {
	delete textSearcher_;
	delete isearch_;
}

/**
 * Adds the document.
 * @param document the document to be added
 * @throw std#invalid_argument @a document is already registered
 */
void Session::addDocument(text::Document& document) {
	if(find(documents_.begin(), documents_.end(), &document) != documents_.end())
		throw invalid_argument("The specified document is already registered.");
	documents_.push_back(&document);
	static_cast<internal::ISessionElement&>(document).setSession(*this);
}

/// Returns the clipboard ring.
ClipboardRing& Session::getClipboardRing() throw() {
	return clipboardRing_;
}

/// Returns the clipboard ring.
const ClipboardRing& Session::getClipboardRing() const throw() {
	return clipboardRing_;
}

/// Returns the incremental searcher.
searcher::IncrementalSearcher& Session::getIncrementalSearcher() throw() {
	if(isearch_ == 0)
		isearch_ = new searcher::IncrementalSearcher();
	return *isearch_;
}

/// Returns the incremental searcher.
const searcher::IncrementalSearcher& Session::getIncrementalSearcher() const throw() {
	if(isearch_ == 0)
		const_cast<searcher::IncrementalSearcher*&>(isearch_) = new searcher::IncrementalSearcher();
	return *isearch_;
}

#ifndef ASCENSION_NO_MIGEMO
/**
 * Returns the directory of C/Migemo DLL or dictionary.
 * @param runtime true to get about DLL, false to get about dictionary
 * @return the path name of the directory
 */
const WCHAR* Session::getMigemoPathName(bool runtime) throw() {
	return runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_;
}
#endif /* !ASCENSION_NO_MIGEMO */

/// Returns the text searcher.
searcher::TextSearcher& Session::getTextSearcher() throw() {
	if(textSearcher_ == 0)
		textSearcher_ = new searcher::TextSearcher();
	return *textSearcher_;
}

/// Returns the text searcher.
const searcher::TextSearcher& Session::getTextSearcher() const throw() {
	if(textSearcher_ == 0)
		const_cast<searcher::TextSearcher*&>(textSearcher_) = new searcher::TextSearcher();
	return *textSearcher_;
}

/**
 * Removes the document.
 * @param document the document to be removed
 * @throw std#invalid_argument @a document is not registered
 */
void Session::removeDocument(text::Document& document) {
	vector<text::Document*>::iterator i = find(documents_.begin(), documents_.end(), &document);
	if(i == documents_.end())
		throw invalid_argument("The specified document is not registered.");
	documents_.erase(i);
}

#ifndef ASCENSION_NO_MIGEMO
/**
 * Sets the directory of C/Migemo DLL or dictionary.
 * @param pathName the path name of the directory or @c null
 * @param runtime true to set about DLL, false to set about dictionary
 */
void Session::setMigemoPathName(const WCHAR* pathName, bool runtime) {
	if(pathName == 0)
		wcscpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, L"");
	else if(wcslen(pathName) > MAX_PATH - 1)
		throw overflow_error("Too long path name.");
	else
		wcscpy(runtime ? migemoRuntimePathName_ : migemoDictionaryPathName_, pathName);
}
#endif /* !ASCENSION_NO_MIGEMO */
