/**
 * @file searcher.cpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.cpp)
 * @date 2006-2007
 */

#include "stdafx.h"
#include "searcher.hpp"
#ifndef ASCENSION_NO_REGEX
#include "regex.hpp"
#endif /* !ASCENSION_NO_REGEX */

using namespace ascension;
using namespace ascension::searcher;
using namespace ascension::text;
using namespace ascension::unicode;
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
 * For the detailes, see the description of @c regex#MigemoPattern class.
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
	if(first == 0 || last == 0 || first >= last)
		throw invalid_argument("invalid pattern input.");
	direction_ = direction;
	caseSensitive_ = !ignoreCase;
#ifndef ASCENSION_NO_UNICODE_COLLATION
	collator_ = collator;
#endif /* !ASCENSION_NO_UNICODE_COLLATION */
	// build pseudo collation elements
	first_ = last_ = new int[last - first];
	for(UTF16To32Iterator<const Char*, utf16boundary::USE_BOUNDARY_ITERATORS> i(first, last); !i.isLast(); ++i, ++last_)
		*last_ = caseSensitive_ ? *i : CaseFolder::fold(*i);
	// build BM shift table
	fill(lastOccurences_, endof(lastOccurences_), last_ - first_);
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
	auto_ptr<CharacterIterator> i(target.clone());
	for(const int* e = first_; e < last_ && !i->isLast(); ++e, i->next()) {
		if(*e != (caseSensitive_ ? i->current() : CaseFolder::fold(i->current())))
			return false;
	}
	return i->isLast();
}

namespace {
	inline CharacterIterator& slowAdvance(CharacterIterator& i, ptrdiff_t delta) {
		for(; delta > 0; --delta) i.next();
		for(; delta < 0; ++delta) i.previous();
		return i;
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
		slowAdvance(*t, last_ - first_ - 1);
		for(const int* pattern; !t->isLast(); slowAdvance(*t,
				max<length_t>(lastOccurences_[caseSensitive_ ? t->current() : CaseFolder::fold(t->current())], last_ - pattern))) {
			for(pattern = last_ - 1;
				(caseSensitive_ ? t->current() : CaseFolder::fold(t->current())) == (caseSensitive_ ? *pattern : CaseFolder::fold(*pattern));
				t->previous(), --pattern) {
				if(pattern == first_) {
					matchedFirst = t;
					matchedLast = matchedFirst->clone();
					slowAdvance(*matchedLast, last_ - first_);
					return true;
				}
			}
		}
	} else {
		ptrdiff_t skipLength;
		slowAdvance(*t, first_ - last_);
		for(const int* pattern; ; slowAdvance(*t, -skipLength)) {
			for(pattern = first_;
					(caseSensitive_ ? t->current() : CaseFolder::fold(t->current())) == (caseSensitive_ ? *pattern : CaseFolder::fold(*pattern));
					t->next(), ++pattern) {
				if(pattern == last_ - 1) {
					slowAdvance(*t, first_ - last_ + 1);
					matchedFirst = t;
					matchedLast = matchedFirst->clone();
					slowAdvance(*matchedLast, last_ - first_);
					return true;
				}
			}
			skipLength = max(lastOccurences_[caseSensitive_ ? t->current() : CaseFolder::fold(t->current())], pattern - first_ + 1);
			if(skipLength > t->getOffset() - target.getOffset())
				break;
		}
	}
	return false;
}


// TextSearcher /////////////////////////////////////////////////////////////

namespace {
	inline void setupRegexRegionalMatchOptions(const DocumentCharacterIterator& first,
			const DocumentCharacterIterator& last, regex::Pattern::MatchOptions& options) throw() {
		options.set(regex::Pattern::TARGET_FIRST_IS_NOT_BOB, first.tell() != first.getDocument()->getStartPosition(false));
		options.set(regex::Pattern::TARGET_LAST_IS_NOT_EOB, last.tell() != last.getDocument()->getEndPosition(false));
		options.set(regex::Pattern::TARGET_FIRST_IS_NOT_BOL, first.tell().column != 0);
		options.set(regex::Pattern::TARGET_LAST_IS_NOT_EOL, last.tell().column != last.getDocument()->getLineLength(last.tell().line));
	}
}

/// Default constructor.
TextSearcher::TextSearcher() : 
#ifndef ASCENSION_NO_REGEX
		lastResult_(0),
#endif /* !ASCENSION_NO_REGEX */
		maximumNumberOfStoredStrings_(DEFAULT_NUMBER_OF_STORED_STRINGS) {
	pattern_.literal = 0;
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
			const Document& document = *first.getDocument();
			const WordBreakIterator<DocumentCharacterIterator> bi1(first, AbstractWordBreakIterator::START_OF_SEGMENT,
				document.getContentTypeInformation().getIdentifierSyntax(document.getPartitioner().getContentType(first.tell())));
			if(!bi1.isBoundary(first))
				return false;
			const WordBreakIterator<DocumentCharacterIterator> bi2(last, AbstractWordBreakIterator::END_OF_SEGMENT,
				document.getContentTypeInformation().getIdentifierSyntax(document.getPartitioner().getContentType(last.tell())));
			return bi2.isBoundary(last);
		}
	}
	return true;
}

/// Clears the cache of the search pattern.
void TextSearcher::clearPatternCache() {
#ifndef ASCENSION_NO_REGEX
	delete lastResult_;
	lastResult_ = 0;
#endif /* !ASCENSION_NO_REGEX */
	switch(options_.type) {
	case LITERAL:
		delete pattern_.literal;
		pattern_.literal = 0;
		break;
#ifndef ASCENSION_NO_REGEX
	case REGULAR_EXPRESSION:
#ifndef ASCENSION_NO_MIGEMO
	case MIGEMO:
#endif /* !ASCENSION_NO_MIGEMO */
		delete pattern_.regex;
		pattern_.regex = 0;
		break;
#endif /* !ASCENSION_NO_REGEX */
	}
}

/**
 * Compiles the regular expression pattern.
 * @param direction the direction to search (this is used by only @c LiteralPattern)
 * @throw std#logic_error the pattern is not specified
 */
void TextSearcher::compilePattern(Direction direction) const {
	if(storedPatterns_.empty() && temporaryPattern_.empty())
		throw logic_error("pattern is not set.");
	TextSearcher& self = *const_cast<TextSearcher*>(this);
	const String& p = temporaryPattern_.empty() ? storedPatterns_.front() : temporaryPattern_;
	switch(options_.type) {
	case LITERAL:
		if(self.pattern_.literal == 0)
			self.pattern_.literal = new LiteralPattern(p.data(), p.data() + p.length(), direction, !options_.caseSensitive);
		break;
#ifndef ASCENSION_NO_REGEX
	case REGULAR_EXPRESSION:
		if(self.pattern_.regex == 0)
			self.pattern_.regex = new regex::Pattern(p.data(), p.data() + p.length(),
				options_.caseSensitive ? regex::Pattern::NORMAL : regex::Pattern::CASE_INSENSITIVE);
		break;
#ifndef ASCENSION_NO_MIGEMO
	case MIGEMO:
		if(self.pattern_.regex == 0)
			self.pattern_.regex = regex::MigemoPattern::create(p.data(), p.data() + p.length(), !options_.caseSensitive).release();
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

/**
 * Returns true if the pattern matches the specified text.
 * @param document the document
 * @param target the target to match
 * @return true if matched
 * @throw std#logic_error the pattern is not specified
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool TextSearcher::match(const Document& document, const Region& target) const {
	const DocumentCharacterIterator b(document, target.getTop()), e(document, target.getBottom());
	compilePattern((options_.type == LITERAL && pattern_.literal != 0) ? pattern_.literal->getDirection() : FORWARD);
	switch(options_.type) {
		case LITERAL:
			return pattern_.literal->matches(DocumentCharacterIterator(document, target)) && checkBoundary(b, e);
#ifndef ASCENSION_NO_REGEX
		case REGULAR_EXPRESSION:
#ifndef ASCENSION_NO_MIGEMO
		case MIGEMO:
#endif /* !ASCENSION_NO_MIGEMO */
		{
			regex::Pattern::MatchOptions options(regex::Pattern::MULTILINE);
			setupRegexRegionalMatchOptions(b, e, options);
			delete lastResult_;
			lastResult_ = pattern_.regex->matches(b, e, options).release();
			if(lastResult_ != 0 && !checkBoundary(b, e)) {
				delete lastResult_;
				lastResult_ = 0;
			}
			return lastResult_ != 0;
		}
#endif /* !ASCENSION_NO_REGEX */
	}
	return false;
}

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

/**
 * Replaces the specified text with the replacement string. If the stored replacements list is
 * empty, an empty is used as the replacement string.
 * @param document the document
 * @param target the region to replace
 * @param[out] replaced the result string
 * @return true if matched
 * @throw std#logic_error the pattern is not specified
 */
bool TextSearcher::replace(const Document& document, const Region& target, String& replaced) const {
	if(!match(document, target))
		return false;
	const String replacement = !storedReplacements_.empty() ? storedReplacements_.front() : String();
	switch(options_.type) {
	case LITERAL:
		replaced = replacement;
		break;
#ifndef ASCENSION_NO_REGEX
	case REGULAR_EXPRESSION:
#ifndef ASCENSION_NO_MIGEMO
	case MIGEMO:
#endif /* !ASCENSION_NO_MIGEMO */
		replaced = lastResult_->replace(replacement.data(), replacement.data() + replacement.length());
		break;
#endif /* !ASCENSION_NO_REGEX */
	}
	return true;
}

/**
 * Searches the pattern in the document.
 * @param document the document
 * @param scope the region to search
 * @param direction the direction to search
 * @param[out] matchedRegion the matched region
 * @return true if the pattern is found
 * @throw std#logic_error the pattern is not specified
 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
 */
bool TextSearcher::search(const Document& document, const Region& scope, Direction direction, Region& matchedRegion) const {
	compilePattern(direction);
	if(options_.type == LITERAL) {
		if(direction != pattern_.literal->getDirection()) {	// recompile to change the direction
			const String& p = storedPatterns_.front();
			pattern_.literal->compile(p.data(), p.data() + p.length(), direction, !options_.caseSensitive);
		}
		auto_ptr<CharacterIterator> matchedFirst, matchedLast;
		for(DocumentCharacterIterator i = (direction == FORWARD) ?
				DocumentCharacterIterator(document, scope) : DocumentCharacterIterator(document, scope, scope.second);
				(direction == FORWARD) ? !i.isLast() : !i.isFirst(); (direction == FORWARD) ? ++i : --i) {
			if(!pattern_.literal->search(i, matchedFirst, matchedLast))
				break;
			else if(checkBoundary(
					static_cast<DocumentCharacterIterator&>(*matchedFirst),
					static_cast<DocumentCharacterIterator&>(*matchedLast))) {
				matchedRegion.first = static_cast<DocumentCharacterIterator*>(matchedFirst.get())->tell();
				matchedRegion.second = static_cast<DocumentCharacterIterator*>(matchedLast.get())->tell();
				return true;
			}
		}
	}
#ifndef ASCENSION_NO_REGEX
	else if(options_.type == REGULAR_EXPRESSION
#ifndef ASCENSION_NO_MIGEMO
			|| options_.type == MIGEMO
#endif /* !ASCENSION_NO_MIGEMO */
#endif /* !ASCENSION_NO_REGEX */
	) {
		regex::Pattern::MatchOptions options = regex::Pattern::MULTILINE;
		const DocumentCharacterIterator end(document, scope.second);
		auto_ptr<regex::MatchResult<DocumentCharacterIterator> > result;
		delete lastResult_;
		lastResult_ = 0;

		if(direction == FORWARD) {	// 前方検索
			DocumentCharacterIterator i(document, scope.first);	// 検索開始位置
			setupRegexRegionalMatchOptions(i, end, options);
			do {
				result = pattern_.regex->search(i, end, options);
				if(result.get() == 0 || checkBoundary(result->getStart(), result->getEnd()))
					break;
				i = result->getEnd();	// 次の検索開始位置へ
				options |= regex::Pattern::TARGET_FIRST_IS_NOT_BOB;
				options |= regex::Pattern::TARGET_FIRST_IS_NOT_BOL;
			} while(i < end);
		} else {	// 後方検索
			DocumentCharacterIterator i(document, scope.second);	// 検索開始位置
			if(i.tell() != scope.first)
				--i;
			setupRegexRegionalMatchOptions(i, end, options);
			// マッチ対象先頭でのみマッチするようにする
			options |= regex::Pattern::MATCH_AT_ONLY_TARGET_FIRST;
			while(true) {
				result = pattern_.regex->search(i, end, options);
				if(result.get() != 0 && checkBoundary(result->getStart(), result->getEnd()))
					break;
				else if(i.tell() <= scope.first)
					break;
				--i;	// 次の検索開始位置へ
				options.set(regex::Pattern::TARGET_FIRST_IS_NOT_BOB, i.tell() != document.getStartPosition(false));
				options.set(regex::Pattern::TARGET_FIRST_IS_NOT_BOL, i.tell().column != 0);
			}
		}
		if(result.get() != 0) {
			matchedRegion.first = result->getStart().tell();
			matchedRegion.second = result->getEnd().tell();
			lastResult_ = result.release();
			return true;
		}
	}
	return false;
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
 * following #getPattern call will not return the pattern set by this
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
		if(listener_ != 0) {
			while(statusHistory_.size() > 1)
				statusHistory_.pop();
			listener_->incrementalSearchAborted(statusHistory_.top().matchedRegion.first);
		}
		end();
	}
}

/**
 * Appends the specified character to the end of the current search pattern.
 * @param ch the character to append
 * @return true if the pattern is found
 * @throw NotRunningException the searcher is not running
 */
bool IncrementalSearcher::addCharacter(Char ch) {
	if(!isRunning())
		throw NotRunningException();
	pattern_ += ch;
	operationHistory_.push(TYPE);
	return update();
}


/**
 * Appends the specified character to the end of the current search pattern.
 * @param cp the character to append
 * @return true if the pattern is found
 * @throw NotRunningException the searcher is not running
 */
bool IncrementalSearcher::addCharacter(CodePoint cp) {
	if(!isRunning())
		throw NotRunningException();
	if(cp < 0x010000)
		return addCharacter(static_cast<Char>(cp & 0xFFFF));
	Char surrogates[2];
	surrogates::encode(cp, surrogates);
	return addString(surrogates, surrogates + 2);
}

/**
 * Appends the specified string to the end of the search pattern.
 * @param first the start of the string to append
 * @param last the end og the string to append
 * @return true if the pattern is found
 * @throw NotRunningException the searcher is not running
 * @throw std#invalid_argument the string is empty
 */
bool IncrementalSearcher::addString(const Char* first, const Char* last) {
	assert(first != 0 && last != 0 && first <= last);
	if(!isRunning())
		throw NotRunningException();
	else if(first == last)
		throw invalid_argument("Added string is empty.");
	pattern_.append(first, last);
	for(const Char* p = first; p < last; ++p)
		operationHistory_.push(TYPE);
	return update();
}

/// @see text#IDocumentListener#documentAboutToBeChanged
void IncrementalSearcher::documentAboutToBeChanged(const Document&) {
	abort();
}

/// @see text#IDocumentListener#documentChanged
void IncrementalSearcher::documentChanged(const Document&, const DocumentChange&) {
}

/// Ends the search.
void IncrementalSearcher::end() {
	if(isRunning()) {
		document_->removeListener(*this);
		while(!statusHistory_.empty())
			statusHistory_.pop();
		if(listener_ != 0)
			listener_->incrementalSearchCompleted();
		if(!pattern_.empty())
			searcher_->setPattern(pattern_);	// store to reuse
		searcher_ = 0;
		listener_ = 0;
		pattern_.erase();
	}
}

/**
 * Search the next match. If the pattern is empty, this method uses the last used pattern.
 * @param direction the new direction of the search
 * @return true if matched after jump
 * @throw NotRunningException the searcher is not running
 */
bool IncrementalSearcher::next(Direction direction) {
	if(!isRunning())
		throw NotRunningException();

	if(pattern_.empty()) {
		statusHistory_.top().direction = direction;
		if(searcher_->getNumberOfStoredPatterns() > 0)
			return addString(searcher_->getPattern());	// use the most recent used
		else {
			if(listener_ != 0)
				listener_->incrementalSearchPatternChanged(IIncrementalSearchListener::EMPTY_PATTERN);
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
 * @throw NotRunningException the searcher is not running
 */
void IncrementalSearcher::reset() {
	if(statusHistory_.empty())
		throw NotRunningException();
	while(!operationHistory_.empty())
		operationHistory_.pop();
	while(statusHistory_.size() > 1)
		statusHistory_.pop();
	pattern_.erase();
	if(listener_ != 0)
		listener_->incrementalSearchPatternChanged(IIncrementalSearchListener::EMPTY_PATTERN);
}

/**
 * Starts the search.
 * @param document the document to search
 * @param from the position at which the search starts
 * @param searcher the text search object
 * @param direction the initial search direction
 * @param listener the listener. can be @c null
 */
void IncrementalSearcher::start(Document& document, const Position& from,
		TextSearcher& searcher, Direction direction, IIncrementalSearchListener* listener /* = 0 */) {
	if(isRunning())
		end();
	const Status s = {Region(from, from), direction};
	assert(statusHistory_.empty() && pattern_.empty());
	statusHistory_.push(s);
	(document_ = &document)->addListener(*this);
	searcher_ = &searcher;
	matchedRegion_ = statusHistory_.top().matchedRegion;
	if(listener_ = listener) {
		listener_->incrementalSearchStarted(document);
		listener_->incrementalSearchPatternChanged(IIncrementalSearchListener::EMPTY_PATTERN);
	}
}

/**
 * Starts the search.
 * @param document the document to search
 * @param from the position at which the search starts
 * @param searcher the text search object
 * @param type the type of the search
 * @param direction the initial search direction
 * @param listener the listener. can be @c null
 */
void IncrementalSearcher::start(Document& document, const Position& from,
		TextSearcher& searcher, SearchType type, Direction direction, IIncrementalSearchListener* listener /* = 0 */) {
	if(isRunning())
		end();
	SearchOptions options = searcher.getOptions();
	options.type = type;
	searcher.setOptions(options);
	start(document, from, searcher, direction, listener);
}

/**
 * Undoes the last search command. If the last command is typing, the end of the pattern is removed.
 * Otherwise if researching, reverts to the last state.
 * @return true if matched after the undo 
 * @throw NotRunningException the searcher is not running
 * @throw EmptyUndoBufferException can't undo
 */
bool IncrementalSearcher::undo() {
	if(!isRunning())
		throw NotRunningException();
	else if(!canUndo())
		throw EmptyUndoBufferException();

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
			if(listener_ != 0)
				listener_->incrementalSearchPatternChanged(IIncrementalSearchListener::FOUND);
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
		if(listener_ != 0)
			listener_->incrementalSearchPatternChanged(IIncrementalSearchListener::EMPTY_PATTERN);
		return true;
	}

	searcher_->setPattern(pattern_, true);
	Region matchedRegion;
	Region scope(
		(lastStatus.direction == FORWARD) ? lastStatus.matchedRegion.second : document_->getStartPosition(),
		(lastStatus.direction == FORWARD) ? document_->getEndPosition() : lastStatus.matchedRegion.first);
	if(statusHistory_.size() > 1 && lastStatus.matchedRegion.isEmpty()) {
		// handle the previous zero-width match
		if(lastStatus.direction == FORWARD) {
			DocumentCharacterIterator temp(*document_, scope.first);
			scope.first = (++temp).tell();
		} else {
			DocumentCharacterIterator temp(*document_, scope.second);
			scope.second = (--temp).tell();
		}
	}
	matched_ = false;
#ifndef ASCENSION_NO_REGEX
	try {
#endif /* !ASCENSION_NO_REGEX */
		matched_ = searcher_->search(*document_, scope, lastStatus.direction, matchedRegion);
#ifndef ASCENSION_NO_REGEX
	} catch(boost::regex_error&) {
		if(listener_ != 0)
			listener_->incrementalSearchPatternChanged(IIncrementalSearchListener::BAD_REGEX);
		return false;
	} catch(runtime_error&) {
		if(listener_ != 0)
			listener_->incrementalSearchPatternChanged(IIncrementalSearchListener::COMPLEX_REGEX);
		return false;
	}
#endif /* !ASCENSION_NO_REGEX */

	if(matched_)
		matchedRegion_ = matchedRegion;
	if(listener_ != 0)
		listener_->incrementalSearchPatternChanged(matched_ ? IIncrementalSearchListener::FOUND : IIncrementalSearchListener::NOT_FOUND);
	return matched_;
}
