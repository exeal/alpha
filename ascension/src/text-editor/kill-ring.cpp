/**
 * @file kill-ring.cpp
 * @author exeal
 * @date 2006-2011 was session.cpp
 * @date 2011-05-06 separated from session.hpp
 */

#include <ascension/text-editor/kill-ring.hpp>
#include <ascension/corelib/basic-exceptions.hpp>
#include <algorithm>
using namespace ascension;
using namespace ascension::texteditor;
using namespace std;


// KillRing ///////////////////////////////////////////////////////////////////////////////////////

/**
 * @class ascension::texteditor::KillRing
 */

/**
 * Constructor.
 * @param maximumNumberOfKills Initial maximum number of kills. This setting can be change by
 *                             @c #setMaximumNumberOfKills later
 */
KillRing::KillRing(size_t maximumNumberOfKills /* = ASCENSION_DEFAULT_MAXIMUM_KILLS */) BOOST_NOEXCEPT
		: yankPointer_(contents_.end()), maximumNumberOfKills_(maximumNumberOfKills) {
}

/**
 * Registers the listener.
 * @param listener The listener to be registered
 * @throw std#invalid_argument @a listener is already registered
 */
void KillRing::addListener(KillRingListener& listener) {
	listeners_.add(listener);
}

/**
 * Makes the given content tha latest kill in the kill ring.
 * @param text The content
 * @param rectangle Set to @c true if the content is a rectangle
 * @param replace Set to @c true to replace the front of the kill ring. Otherwise the new content
 *                will be added
 */
void KillRing::addNew(const String& text, bool rectangle, bool replace /* = false */) {
	if(!contents_.empty() && replace)
		contents_.front() = make_pair(text, rectangle);
	else {
		contents_.push_front(make_pair(text, rectangle));
		if(contents_.size() > maximumNumberOfKills_)
			contents_.pop_back();
	}
	yankPointer_ = begin(contents_);
	listeners_.notify(&KillRingListener::killRingChanged);
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
	yankPointer_ = begin(contents_);
	listeners_.notify(&KillRingListener::killRingChanged);
}

KillRing::Contents::iterator KillRing::at(ptrdiff_t index) const {
	if(contents_.empty())
		throw IllegalStateException("the kill ring is empty.");
	Contents::iterator i(const_cast<KillRing*>(this)->yankPointer_);
	if(index >= 0) {
		for(index -= index - (index % contents_.size()); index > 0; --index) {
			if(++i == end(contents_))
				i = begin(const_cast<KillRing*>(this)->contents_);
		}
	} else {
		for(index += -index -(-index % contents_.size()); index < 0; ++index) {
			if(i == begin(contents_))
				i = end(const_cast<KillRing*>(this)->contents_);
			--i;
		}
	}
	return i;
}

/**
 * Returns the content.
 * @param places
 * @return the content
 * @throw IllegalStateException The kill ring is empty
 */
const pair<String, bool>& KillRing::get(ptrdiff_t places /* = 0 */) const {
	return *at(places);
}

/// Returns the maximum number of kills.
size_t KillRing::maximumNumberOfKills() const BOOST_NOEXCEPT {
	return maximumNumberOfKills_;
}

/// Returns the number of kills.
size_t KillRing::numberOfKills() const BOOST_NOEXCEPT {
	return contents_.size();
}

/**
 * Removes the listener.
 * @param listener The listener to be removed
 * @throw std#invalid_argument @a listener is not registered
 */
void KillRing::removeListener(KillRingListener& listener) {
	listeners_.remove(listener);
}

/**
 * Rotates the yanking point by the given number of places.
 * @param places
 * @return The content
 * @throw IllegalStateException The kill ring is empty
 */
const pair<String, bool>& KillRing::setCurrent(ptrdiff_t places) {
	yankPointer_ = at(places);
	return *yankPointer_;
}
