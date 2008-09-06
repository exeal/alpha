/**
 * @file searcher.cpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.cpp)
 * @date 2006-2008
 */

#include "searcher.hpp"
#include "point.hpp"
using namespace ascension;
using namespace ascension::kernel;
using namespace ascension::searcher;
using namespace ascension::text;
using namespace std;

/**
 * @namespace ascension::searcher
 * Implementation of text search objects.
 *
 * @c TextSearcher class is the most fundamental interface for text search. It supports text match,
 * search, and replacement features, and also holds the search options. @c DocumentSearcher is the
 * variant of @c TextSearcher for search in the document.
 *
 * Ascension provides following text search objects:
 *
 * - Literal search (normal search)
 * - Regular expression search using <a href="http://www.boost.org/libs/regex/">Boost.Regex</a>
 * - Japanese direct search using <a href="http://www.kaoriya.net/#CMigemo">C/Migemo</a>
 *
 * <h3>Regular expression search (Boost.Regex)</h3>
 *
 * Perl-like regular expression match, search, and replacement are available unless the configuration
 * symbol @c ASCENSION_NO_REGEX. For the details, see the description of @c regex#Pattern class.
 *
 * <h3>Japanese direct search (C/Migemo)</h3>
 *
 * Japanese direct search is available if all of the following conditions are true:
 *
 * - Regular expressions are available
 * - The configuration symbol @c ASCENSION_NO_MIGEMO
 * - Succeeded to load C/Migemo library
 *
 * For the details, see the description of @c regex#MigemoPattern class.
 */


// LiteralPattern ///////////////////////////////////////////////////////////

/**
 * Constructor.
 * @param first the start of the search pattern
 * @param last the end of the search pattern
 * @param direction the direction to search
 * @param ignoreCase set true to perform case-insensitive search
 * @param collator the collator or @c null if not needed
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
LiteralPattern::LiteralPattern(const Char* first, const Char* last,
		Direction direction, bool ignoreCase /* = false */, const Collator* collator /* = 0 */) {
	compile(first, last, direction, ignoreCase, collator);
}

/**
 * Constructor.
 * @param pattern the search pattern
 * @param direction the direction to search
 * @param ignoreCase set true to perform case-insensitive search
 * @param collator the collator or @c null if not needed
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
LiteralPattern::LiteralPattern(const String& pattern,
		Direction direction, bool ignoreCase /* = false */, const Collator* collator /* = 0 */) {
	compile(pattern, direction, ignoreCase, collator);
}

/// Destructor.
LiteralPattern::~LiteralPattern() throw() {
	delete[] first_;
}

/**
 * Compiles the pattern.
 * @param first the start of the search pattern
 * @param last the end of the search pattern
 * @param direction the direction to search
 * @param ignoreCase set true to perform case-insensitive search
 * @param collator the collator or @c null if not needed
 * @throw std#invalid_argument @a first and/or @a last are invalid
 */
void LiteralPattern::compile(const Char* first, const Char* last,
		Direction direction, bool ignoreCase /* = false */, const Collator* collator /* = 0 */) {
	// TODO: use collator.
	if(first == 0 || last == 0 || first >= last)
		throw invalid_argument("invalid pattern input.");
	direction_ = direction;
	caseSensitive_ = !ignoreCase;
#ifndef ASCENSION_NO_UNICODE_COLLATION
	collator_ = collator;
#endif /* !ASCENSION_NO_UNICODE_COLLATION */
	// build pseudo collation elements
	first_ = last_ = new int[last - first];
	for(UTF16To32Iterator<const Char*> i(first, last); i.hasNext(); ++i, ++last_)
		*last_ = caseSensitive_ ? *i : CaseFolder::fold(*i);
	// build BM shift table
	fill(lastOccurences_, MANAH_ENDOF(lastOccurences_), last_ - first_);
	if(direction == FORWARD) {
		for(const int* e = first_; e < last_; ++e)
			lastOccurences_[*e] = last_ - e - 1;
	} else {
		for(const int* e = last_ - 1; ; --e) {
			lastOccurences_[*e] = e - first_;
			if(e == first_)
				break;
		}
	}
}

/**
 * Returns true if the pattern matches the specified character sequence.
 * @param target the character sequence to match
 * @return true if matched
 */
bool LiteralPattern::matches(const CharacterIterator& target) const {
	// TODO: compare using collation elements.
	auto_ptr<CharacterIterator> i(target.clone());
	for(const int* e = first_; e < last_ && i->hasNext(); ++e, i->next()) {
		if(*e != (caseSensitive_ ? i->current() : CaseFolder::fold(i->current())))
			return false;
	}
	return !i->hasNext();
}

namespace {
	template<typename Distance>
	inline void orzAdvance(CharacterIterator& i, Distance offset) {
		while(offset > 0) {i.next(); --offset;}
		while(offset < 0) {i.previous(); ++offset;}
	}
}

/**
 * Searches in the specified character sequence.
 * @param target the target character sequence
 * @param[out] matchedFirst 
 * @param[out] matchedLast 
 * @return true if the pattern was found
 */
bool LiteralPattern::search(const CharacterIterator& target,
		auto_ptr<CharacterIterator>& matchedFirst, auto_ptr<CharacterIterator>& matchedLast) const {
	// TODO: this implementation is just scrath.
	auto_ptr<CharacterIterator> t(target.clone());
	if(direction_ == FORWARD) {
		orzAdvance(*t, last_ - first_ - 1);
		for(const int* pattern; t->hasNext(); orzAdvance(*t,
				max<length_t>(lastOccurences_[caseSensitive_ ? t->current() : CaseFolder::fold(t->current())], last_ - pattern))) {
			for(pattern = last_ - 1;
				(caseSensitive_ ? t->current() : CaseFolder::fold(t->current())) == (caseSensitive_ ? *pattern : CaseFolder::fold(*pattern));
				t->previous(), --pattern) {
				if(pattern == first_) {
					matchedFirst = t;
					matchedLast = matchedFirst->clone();
					orzAdvance(*matchedLast, last_ - first_);
					return true;
				}
			}
		}
	} else {
		ptrdiff_t skipLength;
		orzAdvance(*t, first_ - last_);
		for(const int* pattern; ; orzAdvance(*t, -skipLength)) {
			for(pattern = first_;
					(caseSensitive_ ? t->current() : CaseFolder::fold(t->current())) == (caseSensitive_ ? *pattern : CaseFolder::fold(*pattern));
					t->next(), ++pattern) {
				if(pattern == last_ - 1) {
					orzAdvance(*t, first_ - last_ + 1);
					matchedFirst = t;
					matchedLast = matchedFirst->clone();
					orzAdvance(*matchedLast, last_ - first_);
					return true;
				}
			}
			skipLength = max(lastOccurences_[caseSensitive_ ? t->current() : CaseFolder::fold(t->current())], pattern - first_ + 1);
			if(skipLength > t->offset() - target.offset())
				break;
		}
	}
	return false;
}


// TextSearcher /////////////////////////////////////////////////////////////

/// Default constructor.
TextSearcher::TextSearcher() : maximumNumberOfStoredStrings_(DEFAULT_NUMBER_OF_STORED_STRINGS) {
}

/// Destructor.
TextSearcher::~TextSearcher() {
	clearPatternCache();
}

inline bool TextSearcher::checkBoundary(const DocumentCharacterIterator& first, const DocumentCharacterIterator& last) const {
	switch(options_.wholeMatch) {
		case SearchOptions::GRAPHEME_CLUSTER: {
			const GraphemeBreakIterator<DocumentCharacterIterator> bi(first);
			return bi.isBoundary(first) && bi.isBoundary(last);
		} case SearchOptions::WORD: {
			const Document& document = *first.document();
			const WordBreakIterator<DocumentCharacterIterator> bi1(first, AbstractWordBreakIterator::START_OF_SEGMENT,
				document.contentTypeInformation().getIdentifierSyntax(document.partitioner().contentType(first.tell())));
			if(!bi1.isBoundary(first))
				return false;
			const WordBreakIterator<DocumentCharacterIterator> bi2(last, AbstractWordBreakIterator::END_OF_SEGMENT,
				document.contentTypeInformation().getIdentifierSyntax(document.partitioner().contentType(last.tell())));
			return bi2.isBoundary(last);
		}
	}
	return true;
}

/// Clears the cache of the search pattern.
void TextSearcher::clearPatternCache() {
	literalPattern_.reset();
#ifndef ASCENSION_NO_REGEX
	regexPattern_.reset();
	regexMatcher_.reset();
#endif /* !ASCENSION_NO_REGEX */
}

/**
 * Compiles the regular expression pattern.
 * @param direction the direction to search (this is used by only @c LiteralPattern)
 * @throw IllegalStateException the pattern is not specified
 */
void TextSearcher::compilePattern(Direction direction) const {
	if(storedPatterns_.empty() && temporaryPattern_.empty())
		throw IllegalStateException("pattern is not set.");
	TextSearcher& self = *const_cast<TextSearcher*>(this);
	const String& p = temporaryPattern_.empty() ? storedPatterns_.front() : temporaryPattern_;
	switch(options_.type) {
	case LITERAL:
		if(self.literalPattern_.get() == 0)
			self.literalPattern_.reset(new LiteralPattern(p.data(), p.data() + p.length(), direction, !options_.caseSensitive));
		break;
#ifndef ASCENSION_NO_REGEX
	case REGULAR_EXPRESSION:
		if(regexPattern_.get() == 0) {
			self.regexPattern_ = regex::Pattern::compile(p,
				regex::Pattern::MULTILINE | (options_.caseSensitive ? 0 : regex::Pattern::CASE_INSENSITIVE));
			if(regexMatcher_.get() != 0)
				self.regexMatcher_->reset();
		}
		break;
#ifndef ASCENSION_NO_MIGEMO
	case MIGEMO:
		if(regexPattern_.get() == 0)
			self.regexPattern_ = regex::MigemoPattern::compile(p.data(), p.data() + p.length(), !options_.caseSensitive);
		if(regexPattern_.get() == 0)
			throw runtime_error("failed to create a regular expression pattern by using C/Migemo.");
		break;
#endif /* !ASCENSION_NO_MIGEMO */
#endif /* !ASCENSION_NO_REGEX */
	}
	if(!temporaryPattern_.empty())
		self.temporaryPattern_.erase();
}

/// Returns true if Migemo is available.
bool TextSearcher::isMigemoAvailable() const throw() {
#ifdef ASCENSION_NO_MIGEMO
	return false;
#else
	return regex::MigemoPattern::isMigemoInstalled();
#endif /* ASCENSION_NO_MIGEMO */
}

#if 0
/**
 * Returns true if the pattern matches the specified text.
 * @param document the document
 * @param target the target to match
 * @return true if matched
 * @throw IllegalStateException the pattern is not specified
 * @throw regex#PatternSyntaxException the speicifed regular expression was invalid
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool TextSearcher::match(const Document& document, const Region& target) const {
	bool matched = false;
	const DocumentCharacterIterator b(document, target.beginning()), e(document, target.end());
	compilePattern((options_.type == LITERAL && literalPattern_.get() != 0) ? literalPattern_->getDirection() : FORWARD);
	switch(options_.type) {
		case LITERAL:
			matched = literalPattern_->matches(DocumentCharacterIterator(document, target)) && checkBoundary(b, e);
			break;
#ifndef ASCENSION_NO_REGEX
		case REGULAR_EXPRESSION:
#ifndef ASCENSION_NO_MIGEMO
		case MIGEMO:
#endif /* !ASCENSION_NO_MIGEMO */
			if(regexMatcher_.get() == 0) {
				TextSearcher& self = const_cast<TextSearcher&>(*this);
				self.regexMatcher_ = regexPattern_->matcher(document.begin(), document.end());
				self.regexMatcher_->useAnchoringBounds(false).useTransparentBounds(true);
			}
			const DocumentCharacterIterator oldRegionStart(regexMatcher_->regionStart());
			const DocumentCharacterIterator oldRegionEnd(regexMatcher_->regionEnd());
			matched = regexMatcher_->region(b, e).matches() && checkBoundary(b, e);
			regexMatcher_->region(oldRegionStart, oldRegionEnd);
			if(!matched)
				lastResult_.reset();
			break;
#endif /* !ASCENSION_NO_REGEX */
	}
	if(matched) {
		// remember the result for efficiency
		lastResult_.updateDocumentRevision(document);
		lastResult_.matchedRegion = target;
		lastResult_.direction = FORWARD;
	}
	return matched;
}
#endif /* 0 */

/**
 * Pushes the new string to the stored list.
 * @param s the string to push
 * @param forReplacements set true to push to the replacements list
 */
void TextSearcher::pushHistory(const String& s, bool forReplacements) {
	list<String>& history = forReplacements ? storedReplacements_ : storedPatterns_;
	const list<String>::iterator d = find(history.begin(), history.end(), s);
	if(d != history.end())
		history.erase(d);
	else if(history.size() == maximumNumberOfStoredStrings_)
		history.pop_back();
	history.push_front(s);
}

#if 0
/**
 * Replaces the specified region with the replacement string.
 * <p>If the current search pattern does not match @a target, this method will fail and return false.<p>
 * <p>If the stored replacements list is empty, an empty is used as the replacement string.</p>
 * <p>This method does not begin and terminate an edit collection.</p>
 * @param document the document
 * @param target the region to replace
 * @param[out] endOfReplacement the end of the region covers the replacement. can be @c null
 * @return true if replaced
 * @throw kernel#ReadOnlyDocumentException @a document is read only
 * @throw IllegalStateException the pattern is not specified
 */
bool TextSearcher::replace(Document& document, const Region& target, Position* endOfReplacement) const {
	if(document.isReadOnly())
		throw ReadOnlyDocumentException();
	// check the last result
	if(!lastResult_.matched() || lastResult_.matchedRegion != target || !lastResult_.checkDocumentRevision(document)) {
		if(!match(document, target))
			return false;
	}

	String replacement = !storedReplacements_.empty() ? storedReplacements_.front() : String();
	Position eor;
	if(!target.isEmpty())
		document.erase(target);
	if(replacement.empty())
		eor = target.beginning();
	else {
		switch(options_.type) {
		case LITERAL:
			eor = document.insert(target.beginning(), replacement);
			break;
#ifndef ASCENSION_NO_REGEX
		case REGULAR_EXPRESSION:
#ifndef ASCENSION_NO_MIGEMO
		case MIGEMO:
#endif /* !ASCENSION_NO_MIGEMO */
		{
			assert(regexMatcher_.get() != 0);
			replacement.assign(regexMatcher_->replaceInplace(replacement));
			const Point regionEnd(document, regexMatcher_->regionEnd().tell());
			eor = !replacement.empty() ? document.insert(target.beginning(), replacement) : target.beginning();
			regexMatcher_->endInplaceReplacement(document.begin(), document.end(),
				regexMatcher_->regionStart(), DocumentCharacterIterator(document, regionEnd),
				DocumentCharacterIterator(document, eor));
			break;
		}
#endif /* !ASCENSION_NO_REGEX */
		default:
			assert(false);
		}
	}

	assert(lastResult_.matched() && lastResult_.matchedRegion.first == target.beginning());
	if(target.isEmpty())
		lastResult_.matchedRegion.first = eor;
	lastResult_.matchedRegion.second = eor;
	lastResult_.documentRevisionNumber = document.getRevisionNumber();
	if(endOfReplacement != 0)
		*endOfReplacement = eor;
	return true;
}
#endif /* 0 */

/**
 * Searches and replaces all occurences in the specified region.
 *
 * If @a callback parameter is not @c null, this method begins <em>interactive replacement</em>.
 * In interactive replacement, this method finds the occurences match the pattern one by one,
 * queries the callback object whether to replace it.
 *
 * When the callback object changed the document during replacements, this method will stop.
 *
 * If the stored replacements list is empty, an empty is used as the replacement string.
 *
 * This method does not begin and terminate an <em>sequential edit</em>.
 * @param document the document
 * @param scope the region to search and replace
 * @param callback the callback object for interactive replacement. if @c null, this method
 * replaces all the occurences silently
 * @return the number of replacements
 * @throw IllegalStateException the pattern is not specified
 * @throw ReadOnlyDocumentException @a document is read-only
 * @throw BadRegionException @a region intersects outside of the document
 */
size_t TextSearcher::replaceAll(Document& document, const Region& scope, IInteractiveReplacementCallback* callback) const {
	if(document.isReadOnly())
		throw ReadOnlyDocumentException();
	else if(!document.region().encompasses(scope))
		throw BadRegionException();

	const String replacement = !storedReplacements_.empty() ? storedReplacements_.front() : String();
	size_t numberOfMatches = 0, numberOfReplacements = 0;
	stack<pair<Position, Position> > history;	// for undo (ouch, Region does not support placement new)
	size_t documentRevision = document.revisionNumber();	// to detect other interruptions

	IInteractiveReplacementCallback::Action action;	// the action the callback returns
	IInteractiveReplacementCallback* const storedCallback = callback;
	if(callback != 0)
		callback->replacementStarted(document, Region(scope).normalize());

	compilePattern(FORWARD);
	if(options_.type == LITERAL) {
		if(literalPattern_->direction() != FORWARD) {	// recompile to change the direction
			const String& p = storedPatterns_.front();
			literalPattern_->compile(p.data(), p.data() + p.length(), FORWARD, !options_.caseSensitive);
		}
		auto_ptr<CharacterIterator> matchedFirst, matchedLast;
		Point endOfScope(document, scope.end());
		for(DocumentCharacterIterator i(document, scope); i.hasNext(); ) {
			if(!literalPattern_->search(i, matchedFirst, matchedLast))
				break;
			else if(!checkBoundary(
					static_cast<DocumentCharacterIterator&>(*matchedFirst),
					static_cast<DocumentCharacterIterator&>(*matchedLast))) {
				i.next();
				continue;
			}

			// matched -> replace
			++numberOfMatches;
			Region matchedRegion(
				static_cast<DocumentCharacterIterator&>(*matchedFirst).tell(),
				static_cast<DocumentCharacterIterator&>(*matchedLast).tell());
			while(true) {
				action = (callback != 0) ?
					callback->queryReplacementAction(matchedRegion, !history.empty()) : IInteractiveReplacementCallback::REPLACE;
				if(action != IInteractiveReplacementCallback::UNDO)
					break;
				if(!history.empty()) {
					// undo the last replacement
					matchedRegion.first = history.top().first;
					matchedRegion.second = history.top().second;
					history.pop();
					document.undo();
					documentRevision = document.revisionNumber();
					--numberOfMatches;
					--numberOfReplacements;
				}
			}

			// stop if interrupted
			if(documentRevision != document.revisionNumber())
				break;

			if(action == IInteractiveReplacementCallback::REPLACE
					|| action == IInteractiveReplacementCallback::REPLACE_ALL
					|| action == IInteractiveReplacementCallback::REPLACE_AND_EXIT) {
				// replace? -- yes
				if(action == IInteractiveReplacementCallback::REPLACE_ALL)
					callback = 0;
				if(!matchedRegion.isEmpty() || !replacement.empty()) {
					if(!matchedRegion.isEmpty())
						i.seek(document.erase(matchedRegion));
					if(!replacement.empty())
						i.seek(document.insert(matchedRegion.first, replacement));
					i.setRegion(Region(scope.beginning(), endOfScope));
					documentRevision = document.revisionNumber();
				}
				++numberOfReplacements;
				history.push(matchedRegion);
			} else if(action == IInteractiveReplacementCallback::SKIP)
				i.seek(matchedRegion.second);
			if(action == IInteractiveReplacementCallback::REPLACE_AND_EXIT || action == IInteractiveReplacementCallback::EXIT)
				break;
		}
	}

#ifndef ASCENSION_NO_REGEX
	if(options_.type == REGULAR_EXPRESSION
#ifndef ASCENSION_NO_MIGEMO
			|| options_.type == MIGEMO
#endif /* !ASCENSION_NO_MIGEMO */
	) {
		const Point endOfScope(document, scope.end());
		Position lastEOS(endOfScope);
		DocumentCharacterIterator e(document, endOfScope);
		DocumentCharacterIterator b(e);
		auto_ptr<regex::Matcher<DocumentCharacterIterator> > matcher(regexPattern_->matcher(document.begin(), document.end()));
		matcher->region(
			DocumentCharacterIterator(document, scope.beginning()),
			DocumentCharacterIterator(document, scope.end()))
			.useAnchoringBounds(false).useTransparentBounds(true);
		lastResult_.reset();

		while(matcher->find()) {
			if(!checkBoundary(matcher->start(), matcher->end()))
				matcher->region(++DocumentCharacterIterator(matcher->start()), matcher->end());
			else {
				Position next(Position::ZERO_POSITION);
				// matched -> replace
				++numberOfMatches;
				Region matchedRegion(matcher->start().tell(), matcher->end().tell());
				while(true) {
					action = (callback != 0) ?
						callback->queryReplacementAction(matchedRegion, !history.empty()) : IInteractiveReplacementCallback::REPLACE;
					if(action != IInteractiveReplacementCallback::UNDO)
						break;
					if(!history.empty()) {
						// undo the last replacement
						matchedRegion.first = history.top().first;
						matchedRegion.second = history.top().second;
						history.pop();
						document.undo();
						documentRevision = document.revisionNumber();
						--numberOfMatches;
						--numberOfReplacements;
					}
				}

				// stop if interrupted
				if(documentRevision != document.revisionNumber())
					break;

				if(action == IInteractiveReplacementCallback::REPLACE
						|| action == IInteractiveReplacementCallback::REPLACE_ALL
						|| action == IInteractiveReplacementCallback::REPLACE_AND_EXIT) {
					// replace? -- yes
					if(action == IInteractiveReplacementCallback::REPLACE_ALL)
						callback = 0;
					history.push(matchedRegion);
					if(!matchedRegion.isEmpty())
						next = document.erase(matchedRegion);
					if(!replacement.empty()) {
						const String r(matcher->replaceInplace(replacement));
						if(!r.empty())
							next = document.insert(matchedRegion.beginning(), r);
						matcher->endInplaceReplacement(document.begin(), document.end(),
							DocumentCharacterIterator(document, scope.beginning()), DocumentCharacterIterator(document, endOfScope),
							DocumentCharacterIterator(document, next));
						documentRevision = document.revisionNumber();
					}
				} else if(action == IInteractiveReplacementCallback::SKIP)
					next = matchedRegion.second;
				if(action == IInteractiveReplacementCallback::REPLACE_AND_EXIT || action == IInteractiveReplacementCallback::EXIT)
					break;

				if(matchedRegion.second == e.tell())	// reached the end of the scope
					break;
				else if(endOfScope.position() != lastEOS) {
					e.setRegion(Region(scope.beginning(), endOfScope));
					e.seek(endOfScope);
					lastEOS = endOfScope;
				}
				if(next < matchedRegion.second)
					next = matchedRegion.second;
			}
		}
	}
#endif /* !ASCENSION_NO_REGEX */

	if(storedCallback != 0)
		storedCallback->replacementEnded(numberOfMatches, numberOfReplacements);
	return numberOfReplacements;
}

/**
 * Searches the pattern in the document.
 * @param document the document
 * @param from the position where the search begins
 * @param scope the region to search
 * @param direction the direction to search
 * @param[out] matchedRegion the matched region. the value is not changed unless the process successes
 * @return true if the pattern is found
 * @throw IllegalStateException the pattern is not specified
 * @throw kernel#BadPositionException @a from is outside of @a scope
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool TextSearcher::search(const Document& document,
		const Position& from, const Region& scope, Direction direction, Region& matchedRegion) const {
	if(!scope.includes(from))
		throw BadPositionException();
	bool matched = false;
	compilePattern(direction);
	if(options_.type == LITERAL) {
		if(direction != literalPattern_->direction()) {	// recompile to change the direction
			const String& p = storedPatterns_.front();
			literalPattern_->compile(p.data(), p.data() + p.length(), direction, !options_.caseSensitive);
		}
		auto_ptr<CharacterIterator> matchedFirst, matchedLast;
		for(DocumentCharacterIterator i(document, scope, from);
				(direction == FORWARD) ? i.hasNext() : i.hasPrevious(); (direction == FORWARD) ? i.next() : i.previous()) {
			if(!literalPattern_->search(i, matchedFirst, matchedLast))
				break;	// not found
			else if(checkBoundary(
					static_cast<DocumentCharacterIterator&>(*matchedFirst),
					static_cast<DocumentCharacterIterator&>(*matchedLast))) {
				matchedRegion.first = static_cast<DocumentCharacterIterator*>(matchedFirst.get())->tell();
				matchedRegion.second = static_cast<DocumentCharacterIterator*>(matchedLast.get())->tell();
				matched = true;
				break;
			}
		}
	}

#ifndef ASCENSION_NO_REGEX
	else if(options_.type == REGULAR_EXPRESSION
#ifndef ASCENSION_NO_MIGEMO
			|| options_.type == MIGEMO
#endif /* !ASCENSION_NO_MIGEMO */
	) {
		if(regexMatcher_.get() == 0)
			(const_cast<TextSearcher*>(this)->regexMatcher_ = regexPattern_->matcher(
				document.begin(), document.end()))->useAnchoringBounds(false).useTransparentBounds(true);
		else if(!lastResult_.checkDocumentRevision(document) || direction != lastResult_.direction) {
			const_cast<TextSearcher*>(this)->regexMatcher_->reset(document.begin(), document.end());
			lastResult_.reset();
		}

		const bool maybeContinuous = lastResult_.matched()
			&& direction == lastResult_.direction && lastResult_.checkDocumentRevision(document);
		if(direction == FORWARD) {
			const DocumentCharacterIterator eob(document, scope.end());
			if(!maybeContinuous || from != lastResult_.matchedRegion.second)
				regexMatcher_->region(DocumentCharacterIterator(document, from), eob);
			while(regexMatcher_->find()) {
				if(matched = checkBoundary(regexMatcher_->start(), regexMatcher_->end()))
					break;
				regexMatcher_->region(++DocumentCharacterIterator(regexMatcher_->start()), eob);
			}
		} else {
			// ascension.regex does not support backward searches...
			const bool continuous = maybeContinuous && from == lastResult_.matchedRegion.first;
			const DocumentCharacterIterator e(document, continuous ? lastResult_.matchedRegion.second : from);
			DocumentCharacterIterator b(document, from);	// position from where the match should start
			if(!continuous || b.tell() > scope.beginning()) {
				if(continuous)
					b.previous();
				while(true) {
					regexMatcher_->region(b, e);
					if(matched = (regexMatcher_->lookingAt() && checkBoundary(regexMatcher_->start(), regexMatcher_->end())))
						break;
					else if(b.tell() <= scope.beginning())
						break;
					b.previous();	// move to the next search start
				}
			}
		}
		if(matched) {
			matchedRegion.first = regexMatcher_->start().tell();
			matchedRegion.second = regexMatcher_->end().tell();
		}
	}
#endif /* !ASCENSION_NO_REGEX */

	if(matched) {
		// remember the result for efficiency
		lastResult_.updateDocumentRevision(document);
		lastResult_.matchedRegion = matchedRegion;
		lastResult_.direction = direction;
	} else
		lastResult_.reset();
	return matched;
}

#undef CREATE_BREAK_ITERATOR

/// Sets the maximum number of the stored patterns or replacement strings.
void TextSearcher::setMaximumNumberOfStoredStrings(size_t number) throw() {
	number = max<size_t>(number, MINIMUM_NUMBER_OF_STORED_STRINGS);
	if(storedPatterns_.size() > number)
		storedPatterns_.resize(number);
	if(storedReplacements_.size() > number)
		storedReplacements_.resize(number);
	maximumNumberOfStoredStrings_ = number;
}

/// Sets the new search options
void TextSearcher::setOptions(const SearchOptions& options) throw() {
	if(options != options_) {
		clearPatternCache();
		options_ = options;
	}
}

/**
 * Sets the new pattern.
 * @param pattern the pattern string
 * @param dontRemember set true to not add the pattern into the stored list. in this case, the
 * following #pattern call will not return the pattern set by this
 * @throw std#invalid_argument @a pattern is empty
 */
void TextSearcher::setPattern(const String& pattern, bool dontRemember /* = false */) {
	if(pattern.empty())
		throw invalid_argument("the pattern is empty.");
	else if(storedPatterns_.empty() || pattern != storedPatterns_.front()) {
		if(!dontRemember)
			pushHistory(pattern, false);
		else
			temporaryPattern_ = pattern;
		clearPatternCache();
	}
}

/**
 * Sets the new replacement string.
 * @param replacement the replacement string
 */
void TextSearcher::setReplacement(const String& replacement) {
	pushHistory(replacement, true);
}


// IncrementalSearcher //////////////////////////////////////////////////////

/// Constructor.
IncrementalSearcher::IncrementalSearcher() throw() {
}

/// Aborts the search.
void IncrementalSearcher::abort() {
	if(isRunning()) {
		if(callback_ != 0) {
			while(statusHistory_.size() > 1)
				statusHistory_.pop();
			callback_->incrementalSearchAborted(statusHistory_.top().matchedRegion.first);
		}
		end();
	}
}

/**
 * Appends the specified character to the end of the current search pattern.
 * @param c the character to append
 * @return true if the pattern is found
 * @throw IllegalStateException the searcher is not running
 */
bool IncrementalSearcher::addCharacter(Char c) {
	checkRunning();
	pattern_ += c;
	operationHistory_.push(TYPE);
	return update();
}


/**
 * Appends the specified character to the end of the current search pattern.
 * @param c the character to append
 * @return true if the pattern is found
 * @throw IllegalStateException the searcher is not running
 */
bool IncrementalSearcher::addCharacter(CodePoint c) {
	checkRunning();
	if(c < 0x010000U)
		return addCharacter(static_cast<Char>(c & 0xFFFFU));
	Char surrogates[2];
	surrogates::encode(c, surrogates);
	return addString(surrogates, surrogates + 2);
}

/**
 * Appends the specified string to the end of the search pattern.
 * @param first the start of the string to append
 * @param last the end og the string to append
 * @return true if the pattern is found
 * @throw IllegalStateException the searcher is not running
 * @throw std#invalid_argument the string is empty
 */
bool IncrementalSearcher::addString(const Char* first, const Char* last) {
	assert(first != 0 && last != 0 && first <= last);
	checkRunning();
	if(first == last)
		throw invalid_argument("Added string is empty.");
	pattern_.append(first, last);
	for(const Char* p = first; p < last; ++p)
		operationHistory_.push(TYPE);
	return update();
}

/// @see kernel#IBookmarkListener#bookmarkChanged
void IncrementalSearcher::bookmarkChanged(length_t) {
	abort();
}

/// @see kernel#IBookmarkListener#bookmarkCleared
void IncrementalSearcher::bookmarkCleared() {
	abort();
}

/// @internal Throws an @c IllegalStateException if not in running.
void IncrementalSearcher::checkRunning() const {
	if(!isRunning())
		throw IllegalStateException("The incremental searcher is not running.");
}

/// @see kernel#IDocumentListener#documentAboutToBeChanged
bool IncrementalSearcher::documentAboutToBeChanged(const Document&, const DocumentChange&) {
	abort();
	return true;
}

/// @see kernel#IDocumentListener#documentChanged
void IncrementalSearcher::documentChanged(const Document&, const DocumentChange&) {
}

/// Ends the search.
void IncrementalSearcher::end() {
	if(isRunning()) {
		document_->removeListener(*this);
		document_->bookmarker().removeListener(*this);
		while(!statusHistory_.empty())
			statusHistory_.pop();
		if(callback_ != 0)
			callback_->incrementalSearchCompleted();
		if(!pattern_.empty())
			searcher_->setPattern(pattern_);	// store to reuse
		searcher_ = 0;
		callback_ = 0;
		pattern_.erase();
	}
}

/**
 * Search the next match. If the pattern is empty, this method uses the last used pattern.
 * @param direction the new direction of the search
 * @return true if matched after jump
 * @throw IllegalStateException the searcher is not running
 */
bool IncrementalSearcher::next(Direction direction) {
	checkRunning();
	if(pattern_.empty()) {
		statusHistory_.top().direction = direction;
		if(searcher_->numberOfStoredPatterns() > 0)
			return addString(searcher_->pattern());	// use the most recent used
		else {
			if(callback_ != 0)
				callback_->incrementalSearchPatternChanged(IIncrementalSearchCallback::EMPTY_PATTERN, IIncrementalSearchCallback::NO_WRAPPED);
			return true;
		}
	} else if(!matched_
			&& !operationHistory_.empty()
			&& operationHistory_.top() == JUMP
			&& statusHistory_.top().direction == direction)
		return false;	// tried to next when not matched
	else {
		const Status s = {matchedRegion_, direction};
		statusHistory_.push(s);
		if(update())
			return true;
		statusHistory_.pop();
		operationHistory_.push(JUMP);
		return false;
	}
}

/**
 * Reverts to the initial state.
 * @throw IllegalStateException the searcher is not running
 */
void IncrementalSearcher::reset() {
	checkRunning();
	while(!operationHistory_.empty())
		operationHistory_.pop();
	while(statusHistory_.size() > 1)
		statusHistory_.pop();
	pattern_.erase();
	if(callback_ != 0)
		callback_->incrementalSearchPatternChanged(IIncrementalSearchCallback::EMPTY_PATTERN, IIncrementalSearchCallback::NO_WRAPPED);
}

/**
 * Starts the search.
 * @param document the document to search
 * @param from the position at which the search starts
 * @param searcher the text search object
 * @param direction the initial search direction
 * @param callback the callback object. can be @c null
 */
void IncrementalSearcher::start(Document& document, const Position& from,
		TextSearcher& searcher, Direction direction, IIncrementalSearchCallback* callback /* = 0 */) {
	if(isRunning())
		end();
	const Status s = {Region(from, from), direction};
	assert(statusHistory_.empty() && pattern_.empty());
	statusHistory_.push(s);
	(document_ = &document)->addListener(*this);
	document_->bookmarker().addListener(*this);
	searcher_ = &searcher;
	matchedRegion_ = statusHistory_.top().matchedRegion;
	if(0 != (callback_ = callback)) {
		callback_->incrementalSearchStarted(document);
		callback_->incrementalSearchPatternChanged(IIncrementalSearchCallback::EMPTY_PATTERN, IIncrementalSearchCallback::NO_WRAPPED);
	}
}

/**
 * Undoes the last search command. If the last command is typing, the end of the pattern is removed.
 * Otherwise if researching, reverts to the last state.
 * @return true if matched after the undo 
 * @throw IllegalStateException the searcher is not running or the undo buffer is empty
 */
bool IncrementalSearcher::undo() {
	checkRunning();
	if(!canUndo())
		throw IllegalStateException("Undo buffer of incremental search is empty and not undoable.");

	const Operation lastOperation = operationHistory_.top();
	operationHistory_.pop();
	if(lastOperation == TYPE) {	// 文字の入力を元に戻す -> 検索式の末尾を削る
		if(pattern_.length() > 1
				&& surrogates::isHighSurrogate(pattern_[pattern_.length() - 2])
				&& surrogates::isLowSurrogate(pattern_[pattern_.length() - 1])) {
			pattern_.erase(pattern_.length() - 2);
			operationHistory_.pop();
		} else
			pattern_.erase(pattern_.length() - 1);
		return update();
	} else if(lastOperation == JUMP) {	// 次のマッチ位置へのジャンプを元に戻す -> 1 つ前の状態に戻る
		matchedRegion_ = statusHistory_.top().matchedRegion;
		statusHistory_.pop();
		assert(!statusHistory_.empty());
		if(!matched_) {	// ジャンプを元に戻すと必ずマッチした状態になる
			matched_ = true;
			if(callback_ != 0)
				callback_->incrementalSearchPatternChanged(IIncrementalSearchCallback::FOUND, IIncrementalSearchCallback::NO_WRAPPED);
		}
		return true;
	}
	assert(false);
	return false;	// あり得ぬ
}

/**
 * Re-searches using the current state.
 * @return true if the pattern is found
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool IncrementalSearcher::update() {
	const Status& lastStatus = statusHistory_.top();
	if(pattern_.empty()) {
		assert(statusHistory_.size() == 1);
		matchedRegion_ = lastStatus.matchedRegion;
		if(callback_ != 0)
			callback_->incrementalSearchPatternChanged(IIncrementalSearchCallback::EMPTY_PATTERN, IIncrementalSearchCallback::NO_WRAPPED);
		return true;
	}

	searcher_->setPattern(pattern_, true);
	Region matchedRegion;
	Region scope(document_->accessibleRegion());
/*	if(statusHistory_.size() > 1 && lastStatus.matchedRegion.isEmpty()) {
		// handle the previous zero-width match
		if(lastStatus.direction == FORWARD) {
			DocumentCharacterIterator temp(*document_, scope.first);
			temp.next();
			scope.first = temp.tell();
		} else {
			DocumentCharacterIterator temp(*document_, scope.second);
			temp.previous();
			scope.second = temp.tell();
		}
	}
*/	matched_ = false;
#ifndef ASCENSION_NO_REGEX
	try {
#endif /* !ASCENSION_NO_REGEX */
		matched_ = searcher_->search(*document_,
			(lastStatus.direction == FORWARD) ? lastStatus.matchedRegion.second : lastStatus.matchedRegion.first,
			scope, lastStatus.direction, matchedRegion);
#ifndef ASCENSION_NO_REGEX
	} catch(boost::regex_error&) {
		if(callback_ != 0)
			callback_->incrementalSearchPatternChanged(IIncrementalSearchCallback::BAD_REGEX, IIncrementalSearchCallback::NO_WRAPPED);
		return false;
	} catch(runtime_error&) {
		if(callback_ != 0)
			callback_->incrementalSearchPatternChanged(IIncrementalSearchCallback::COMPLEX_REGEX, IIncrementalSearchCallback::NO_WRAPPED);
		return false;
	}
#endif /* !ASCENSION_NO_REGEX */

	if(matched_)
		matchedRegion_ = matchedRegion;
	if(callback_ != 0)
		callback_->incrementalSearchPatternChanged(matched_ ?
			IIncrementalSearchCallback::FOUND : IIncrementalSearchCallback::NOT_FOUND, IIncrementalSearchCallback::NO_WRAPPED);
	return matched_;
}
