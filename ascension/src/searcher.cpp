/**
 * @file searcher.cpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.cpp)
 * @date 2006-2009
 */

#include <ascension/searcher.hpp>
#include <ascension/point.hpp>
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
 * Constructor compiles the pattern.
 * @param pattern the search pattern
 * @param direction the direction to search
 * @param caseSensitive set @c true to perform case-sensitive search
 * @param collator the collator or @c null if not needed
 * @throw std#invalid_argument @a pattern is empty
 */
LiteralPattern::LiteralPattern(const String& pattern, bool caseSensitive /* = true */,
		auto_ptr<const Collator> collator /* = null */) : pattern_(pattern), caseSensitive_(caseSensitive)
#ifndef ASCENSION_NO_UNICODE_COLLATION
		, collator_(collator)
#endif // !ASCENSION_NO_UNICODE_COLLATION
{
	// TODO: use collator.
	if(pattern.empty())
		throw invalid_argument("pattern");
	lastOccurences_[0] = firstOccurences_[0] = numeric_limits<ptrdiff_t>::min();
	// build pseudo collation elements
	first_ = last_ = new int[pattern_.length()];
	for(StringCharacterIterator i(pattern_); i.hasNext(); ++i, ++last_)
		*last_ = caseSensitive_ ? *i : CaseFolder::fold(*i);
}

/// Destructor.
LiteralPattern::~LiteralPattern() /*throw()*/ {
	delete[] first_;
}

// builds BM shift table for forward/backward search
inline void LiteralPattern::makeShiftTable(Direction direction) /*throw()*/ {
	if(direction == Direction::FORWARD) {
		if(lastOccurences_[0] == numeric_limits<ptrdiff_t>::min()) {
			fill(lastOccurences_, MANAH_ENDOF(lastOccurences_), last_ - first_);
			for(const int* e = first_; e < last_; ++e)
				lastOccurences_[*e] = last_ - e - 1;
		}
	} else if(firstOccurences_[0] == numeric_limits<ptrdiff_t>::min()) {
		fill(firstOccurences_, MANAH_ENDOF(firstOccurences_), last_ - first_);
		for(const int* e = last_ - 1; ; --e) {
			firstOccurences_[*e] = e - first_;
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
		if(*e != static_cast<int>(caseSensitive_ ? i->current() : CaseFolder::fold(i->current())))
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
 * @param direction the direction to search. if this is @c Direction#FORWARD, the method finds the
 *                  first occurence of the pattern in @a target. otherwise finds the last one
 * @param[out] matchedFirst 
 * @param[out] matchedLast 
 * @return true if the pattern was found
 */
bool LiteralPattern::search(const CharacterIterator& target, Direction direction,
		auto_ptr<CharacterIterator>& matchedFirst, auto_ptr<CharacterIterator>& matchedLast) const {
	const_cast<LiteralPattern*>(this)->makeShiftTable(direction);
	// TODO: this implementation is just scrath.
	auto_ptr<CharacterIterator> t(target.clone());
	if(direction == Direction::FORWARD) {
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

/**
 * @class ascension::searcher::TextSearcher
 *
 * Searches the specified pattern in the document.
 *
 * A session holds an instance of this class, while client can create instances.
 *
 * @c TextSearcher has the list of patterns used for search. The pattern which is given by
 * #setPattern method is pushed into this list, and the client can reuse those patterns
 * later. @c IncrementalSearcher uses this list to get the pattern used previously. To get
 * this stored patterns, call #pattern method. To get the length of the list, call
 * @c #numberOfStoredPatterns method. The maximum length of the list can be changed by
 * calling @c #setMaximumNumberOfStoredStrings method. Default length is 16 and the minimum
 * is 4.
 *
 * @note This class is not intended to be subclassed.
 *
 * @see texteditor#Session#getTextSearcher, texteditor#commands#FindAllCommand,
 * texteditor#commands#FindNextCommand
 */

/// Default constructor.
TextSearcher::TextSearcher() : searchType_(LITERAL),
		wholeMatch_(MATCH_UTF32_CODE_UNIT), maximumNumberOfStoredStrings_(DEFAULT_NUMBER_OF_STORED_STRINGS) {
}

/**
 * Returns the collation weight level.
 * This feature is not implemented and returns always zero.
 */
int TextSearcher::collationWeight() const /*throw()*/ {
	return 0;
}

/// Returns @c false if caseless match is enabled. This setting is obtained from the pattern.
bool TextSearcher::isCaseSensitive() const {
	if(literalPattern_.get() != 0)
		return literalPattern_->isCaseSensitive();
#ifndef ASCENSION_NO_REGEX
	else if(regexPattern_.get() != 0)
		return (regexPattern_->flags() & regex::Pattern::CASE_INSENSITIVE) != 0;
#ifndef ASCENSION_NO_MIGEMO
	else if(migemoPattern_.get() != 0)
		return  (migemoPattern_->flags() & regex::Pattern::CASE_INSENSITIVE) != 0;
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
	throw "unreachable.";
}

/// Returns @c true if Migemo is available.
bool TextSearcher::isMigemoAvailable() const /*throw()*/ {
#ifdef ASCENSION_NO_MIGEMO
	return false;
#else
	return regex::MigemoPattern::isMigemoInstalled();
#endif // ASCENSION_NO_MIGEMO
}

#if 0
/**
 * Returns true if the pattern matches the specified text.
 * @param document the document
 * @param target the target to match
 * @return true if matched
 * @throw IllegalStateException the pattern is not specified
 * @throw regex#PatternSyntaxException the speicifed regular expression was invalid
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occurred
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
#endif // !ASCENSION_NO_MIGEMO
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
#endif // !ASCENSION_NO_REGEX
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
	const list<String>::iterator d(find(history.begin(), history.end(), s));
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
#endif // !ASCENSION_NO_MIGEMO
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
#endif // !ASCENSION_NO_REGEX
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
#endif // 0

inline bool checkBoundary(const DocumentCharacterIterator& first, const DocumentCharacterIterator& last, TextSearcher::WholeMatch wholeMatch) {
	switch(wholeMatch) {
		case TextSearcher::MATCH_GRAPHEME_CLUSTER: {
			const GraphemeBreakIterator<DocumentCharacterIterator> bi(first);
			return bi.isBoundary(first) && bi.isBoundary(last);
		} case TextSearcher::MATCH_WORD: {
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
 * This method does not begin and terminate an <em>compound change</em>.
 * @param document the document
 * @param scope the region to search and replace
 * @param callback the callback object for interactive replacement. if @c null, this method
 * replaces all the occurences silently
 * @return the number of replaced occurences
 * @throw IllegalStateException the pattern is not specified
 * @throw ReadOnlyDocumentException @a document is read-only
 * @throw BadRegionException @a region intersects outside of the document
 * @throw ReplacementInterruptedException&lt;IDocumentInput#ChangeRejectedException&gt; the input
 *        of the document rejected this change. if thrown, the replacement will be interrupted
 * @throw ReplacementInterruptedException&lt;std#bad_alloc&gt; the internal memory allocation
 *        failed. if thrown, the replacement will be interrupted
 */
size_t TextSearcher::replaceAll(Document& document, const Region& scope, IInteractiveReplacementCallback* callback) const {
	if(document.isReadOnly())
		throw ReadOnlyDocumentException();
	else if(!document.region().encompasses(scope))
		throw BadRegionException(scope);

	const String replacement(!storedReplacements_.empty() ? storedReplacements_.front() : String());
	size_t numberOfMatches = 0, numberOfReplacements = 0;
	stack<pair<Position, Position> > history;	// for undo (ouch, Region does not support placement new)
	size_t documentRevision = document.revisionNumber();	// to detect other interruptions

	IInteractiveReplacementCallback::Action action;	// the action the callback returns
	IInteractiveReplacementCallback* const storedCallback = callback;
	if(callback != 0)
		callback->replacementStarted(document, Region(scope).normalize());

	if(type() == LITERAL) {
		auto_ptr<CharacterIterator> matchedFirst, matchedLast;
		Point endOfScope(document, scope.end());
		for(DocumentCharacterIterator i(document, scope); i.hasNext(); ) {
			if(!literalPattern_->search(i, Direction::FORWARD, matchedFirst, matchedLast))
				break;
			else if(!checkBoundary(
					static_cast<DocumentCharacterIterator&>(*matchedFirst),
					static_cast<DocumentCharacterIterator&>(*matchedLast), wholeMatch_)) {
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
					Position e;
					try {
						replace(document, matchedRegion, replacement, &e);
					} catch(const IDocumentInput::ChangeRejectedException&) {
						throw ReplacementInterruptedException<IDocumentInput::ChangeRejectedException>(numberOfReplacements);
					} catch(const bad_alloc& e) {
						throw ReplacementInterruptedException<bad_alloc>(e.what(), numberOfReplacements);
					}
					i.seek(e);
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
	if(type() == REGULAR_EXPRESSION
#ifndef ASCENSION_NO_MIGEMO
			|| type() == MIGEMO
#endif // !ASCENSION_NO_MIGEMO
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
			if(!checkBoundary(matcher->start(), matcher->end(), wholeMatch_))
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
					assert(!matchedRegion.isEmpty() || !replacement.empty());
					try {
						replace(document, matchedRegion, matcher->replaceInplace(replacement));
					} catch(const IDocumentInput::ChangeRejectedException&) {
						throw ReplacementInterruptedException<IDocumentInput::ChangeRejectedException>(numberOfReplacements);
					} catch(const bad_alloc& e) {
						throw ReplacementInterruptedException<bad_alloc>(e.what(), numberOfReplacements);
					}
					if(!matchedRegion.isEmpty())
						next = matchedRegion.beginning();
					if(!replacement.empty()) {
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
#endif // !ASCENSION_NO_REGEX

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
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occurred
 */
bool TextSearcher::search(const Document& document,
		const Position& from, const Region& scope, Direction direction, Region& matchedRegion) const {
	if(!scope.includes(from))
		throw BadPositionException(from);
	bool matched = false;
	if(type() == LITERAL) {
		auto_ptr<CharacterIterator> matchedFirst, matchedLast;
		for(DocumentCharacterIterator i(document, scope, from);
				(direction == Direction::FORWARD) ? i.hasNext() : i.hasPrevious();
				(direction == Direction::FORWARD) ? i.next() : i.previous()) {
			if(!literalPattern_->search(i, direction, matchedFirst, matchedLast))
				break;	// not found
			else if(checkBoundary(
					static_cast<DocumentCharacterIterator&>(*matchedFirst),
					static_cast<DocumentCharacterIterator&>(*matchedLast), wholeMatch_)) {
				matchedRegion.first = static_cast<DocumentCharacterIterator*>(matchedFirst.get())->tell();
				matchedRegion.second = static_cast<DocumentCharacterIterator*>(matchedLast.get())->tell();
				matched = true;
				break;
			}
		}
	}

#ifndef ASCENSION_NO_REGEX
	else if(type() == REGULAR_EXPRESSION
#ifndef ASCENSION_NO_MIGEMO
			|| type() == MIGEMO
#endif // !ASCENSION_NO_MIGEMO
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
		if(direction == Direction::FORWARD) {
			const DocumentCharacterIterator eob(document, scope.end());
			if(!maybeContinuous || from != lastResult_.matchedRegion.second)
				regexMatcher_->region(DocumentCharacterIterator(document, from), eob);
			while(regexMatcher_->find()) {
				if(matched = checkBoundary(regexMatcher_->start(), regexMatcher_->end(), wholeMatch_))
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
					if(matched = (regexMatcher_->lookingAt()
							&& checkBoundary(regexMatcher_->start(), regexMatcher_->end(), wholeMatch_)))
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
#endif // !ASCENSION_NO_REGEX

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
void TextSearcher::setMaximumNumberOfStoredStrings(size_t number) /*throw()*/ {
	number = max<size_t>(number, MINIMUM_NUMBER_OF_STORED_STRINGS);
	if(storedPatterns_.size() > number)
		storedPatterns_.resize(number);
	if(storedReplacements_.size() > number)
		storedReplacements_.resize(number);
	maximumNumberOfStoredStrings_ = number;
}

/**
 * @fn ascension::searcher::TextSearcher::setPattern
 * @brief Sets the new pattern.
 * @param pattern the pattern string
 * @param dontRemember set @c true to not add the pattern into the stored list. in this case, the
 *                     following @c #pattern call will not return the pattern set by this
 */

/**
 * Sets the new replacement string.
 * @param replacement the replacement string
 */
void TextSearcher::setReplacement(const String& replacement) {
	pushHistory(replacement, true);
}

/**
 * Sets the "whole match" condition.
 * @param newValue the new whole match value to set
 * @return this object
 * @throw UnknownValueException @a newValue is invalid
 * @see #wholeMatch
 */
TextSearcher& TextSearcher::setWholeMatch(WholeMatch newValue) {
	if(newValue < MATCH_UTF32_CODE_UNIT || newValue > MATCH_WORD)
		throw UnknownValueException("newValue");
	wholeMatch_ = newValue;
	return *this;
}

/**
 * Returns the type of search.
 * @see #setPattern
 */
TextSearcher::Type TextSearcher::type() const /*throw()*/ {
	if(literalPattern_.get() != 0)
		return LITERAL;
#ifndef ASCENSION_NO_REGEX
	else if(regexPattern_.get() != 0)
		return REGULAR_EXPRESSION;
#ifndef ASCENSION_NO_MIGEMO
	else if(migemoPattern_.get() != 0)
		return MIGEMO;
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
	throw "unreachable.";
}

/// Returns the "whole match" condition.
TextSearcher::WholeMatch TextSearcher::wholeMatch() const /*throw()*/ {
	return wholeMatch_;
}


// IncrementalSearcher //////////////////////////////////////////////////////

/// Constructor.
IncrementalSearcher::IncrementalSearcher() /*throw()*/ : type_(TextSearcher::LITERAL) {
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
	if(c < 0x010000u)
		return addCharacter(static_cast<Char>(c & 0xffffu));
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
void IncrementalSearcher::documentAboutToBeChanged(const Document&) {
	abort();
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
			setPatternToSearcher(true);	// store to reuse
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

inline void IncrementalSearcher::setPatternToSearcher(bool pushToHistory) {
	if(pattern_.empty())
		throw IllegalStateException("the pattern is empty.");
	switch(type_) {
	case TextSearcher::LITERAL:
		// TODO: specify 'collator' parameter.
		searcher_->setPattern(
			auto_ptr<const LiteralPattern>(new LiteralPattern(pattern_, searcher_->isCaseSensitive())), pushToHistory);
		break;
#ifndef ASCENSION_NO_REGEX
	case TextSearcher::REGULAR_EXPRESSION:
		searcher_->setPattern(regex::Pattern::compile(pattern_,
			regex::Pattern::MULTILINE | (searcher_->isCaseSensitive() ? 0 : regex::Pattern::CASE_INSENSITIVE)), pushToHistory);
		break;
#ifndef ASCENSION_NO_MIGEMO
	case TextSearcher::MIGEMO:
		searcher_->setPattern(regex::MigemoPattern::compile(
			pattern_.data(), pattern_.data() + pattern_.length(), searcher_->isCaseSensitive()), pushToHistory);
		break;
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
	}
}

/**
 * Starts the search.
 * @param document the document to search
 * @param from the position at which the search starts
 * @param searcher the text search object
 * @param type the search type
 * @param direction the initial search direction
 * @param callback the callback object. can be @c null
 */
void IncrementalSearcher::start(Document& document, const Position& from, TextSearcher& searcher,
		TextSearcher::Type type, Direction direction, IIncrementalSearchCallback* callback /* = 0 */) {
	if(isRunning())
		end();
	const Status s = {Region(from, from), direction};
	assert(statusHistory_.empty() && pattern_.empty());
	statusHistory_.push(s);
	(document_ = &document)->addListener(*this);
	document_->bookmarker().addListener(*this);
	searcher_ = &searcher;
	type_ = type;
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
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occurred
 */
bool IncrementalSearcher::update() {
	const Status& lastStatus = statusHistory_.top();
	if(pattern_.empty()) {
		assert(statusHistory_.size() == 1);
		matchedRegion_ = lastStatus.matchedRegion;
		if(callback_ != 0)
			callback_->incrementalSearchPatternChanged(
				IIncrementalSearchCallback::EMPTY_PATTERN, IIncrementalSearchCallback::NO_WRAPPED);
		return true;
	}
	setPatternToSearcher(false);

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
#endif // !ASCENSION_NO_REGEX
		matched_ = searcher_->search(*document_,
			(lastStatus.direction == Direction::FORWARD) ? lastStatus.matchedRegion.second : lastStatus.matchedRegion.first,
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
#endif // !ASCENSION_NO_REGEX

	if(matched_)
		matchedRegion_ = matchedRegion;
	if(callback_ != 0)
		callback_->incrementalSearchPatternChanged(matched_ ?
			IIncrementalSearchCallback::FOUND : IIncrementalSearchCallback::NOT_FOUND, IIncrementalSearchCallback::NO_WRAPPED);
	return matched_;
}
