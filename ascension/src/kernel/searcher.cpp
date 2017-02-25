/**
 * @file searcher.cpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.cpp)
 * @date 2006-2014, 2016
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/corelib/text/case-folder.hpp>
#include <ascension/corelib/text/grapheme-break-iterator.hpp>
#include <ascension/corelib/text/string-character-iterator.hpp>
#include <ascension/corelib/text/word-break-iterator.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/kernel/point.hpp>
#include <ascension/kernel/searcher.hpp>
#include <ascension/corelib/text/break-iterator.hpp>
#include <boost/range/algorithm/find.hpp>


namespace ascension {
	/**
	 * Implementation of text search objects.
	 *
	 * @c TextSearcher class is the most fundamental interface for text search. It supports text match, search, and
	 * replacement features, and also holds the search options. @c DocumentSearcher is the variant of @c TextSearcher
	 * for search in the document.
	 *
	 * Ascension provides following text search objects:
	 *
	 * - Literal search (normal search)
	 * - Regular expression search using <a href="http://www.boost.org/libs/regex/">Boost.Regex</a>
	 * - Japanese direct search using <a href="http://www.kaoriya.net/#CMigemo">C/Migemo</a>
	 *
	 * <h3>Regular expression search (Boost.Regex)</h3>
	 *
	 * Perl-like regular expression match, search, and replacement are available unless the configuration symbol
	 * @c ASCENSION_NO_REGEX. For the details, see the description of @c regex#Pattern class.
	 *
	 * <h3>Japanese direct search (C/Migemo)</h3>
	 *
	 * Japanese direct search is available if all of the following conditions are true:
	 *
	 * - Regular expressions are available
	 * - The configuration symbol @c ASCENSION_NO_MIGEMO is not defined
	 * - Succeeded to load C/Migemo library at runtime
	 *
	 * For the details, see the description of @c regex#MigemoPattern class.
	 */
	namespace searcher {
		// LiteralPattern /////////////////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor compiles the pattern.
		 * @param pattern The search pattern
		 * @param caseSensitive Set @c true to perform case-sensitive search
		 * @param collator The collator or @c null if not needed. This parameter is not exist if the symbol
		 *                 ASCENSION_NO_UNICODE_COLLATION is defined
		 * @throw std#invalid_argument @a pattern is empty
		 */
		LiteralPattern::LiteralPattern(const String& pattern, bool caseSensitive /* = true */
#ifndef ASCENSION_NO_UNICODE_COLLATION
				, std::unique_ptr<const Collator> collator /* = nullptr */
#endif // !ASCENSION_NO_UNICODE_COLLATION
				) : pattern_(pattern), caseSensitive_(caseSensitive)
#ifndef ASCENSION_NO_UNICODE_COLLATION
				, collator_(collator)
#endif // !ASCENSION_NO_UNICODE_COLLATION
		{
			// TODO: use collator.
			if(pattern.empty())
				throw std::invalid_argument("pattern");
			lastOccurences_[0] = firstOccurences_[0] = std::numeric_limits<std::ptrdiff_t>::min();
			// build pseudo collation elements
			collationElements_.resize(pattern_.length());
			std::vector<int>::iterator collationElement(std::begin(collationElements_));
			for(text::StringCharacterIterator i(pattern_); i.hasNext(); ++i, ++collationElement)
				*collationElement = caseSensitive_ ? *i : text::CaseFolder::fold(*i);
		}

		// builds BM shift table for forward/backward search
		inline void LiteralPattern::makeShiftTable(Direction direction) BOOST_NOEXCEPT {
			if(direction == Direction::forward()) {
				if(lastOccurences_[0] == std::numeric_limits<std::ptrdiff_t>::min()) {
					lastOccurences_.fill(collationElements_.size());
//					std::fill(lastOccurences_, ASCENSION_ENDOF(lastOccurences_), last_ - first_);
					for(std::vector<int>::const_iterator i(std::begin(collationElements_)), e(std::end(collationElements_)); i < e; ++i)
						lastOccurences_[*i] = e - i - 1;
				}
			} else if(firstOccurences_[0] == std::numeric_limits<std::ptrdiff_t>::min()) {
				firstOccurences_.fill(collationElements_.size());
//				std::fill(firstOccurences_, ASCENSION_ENDOF(firstOccurences_), last_ - first_);
				for(std::vector<int>::const_iterator i(std::end(collationElements_) - 1), b(std::begin(collationElements_)); ; --i) {
					firstOccurences_[*i] = i - b;
					if(i == b)
						break;
				}
			}
		}

		bool LiteralPattern::matches(const text::detail::CharacterIterator& target) const {
			// TODO: compare using collation elements.
			text::detail::CharacterIterator i(target);
			for(std::vector<int>::const_iterator j(collationElements_.cbegin()), e(collationElements_.cend()); j < e && i.hasNext(); ++j, ++i) {
				if(*j != static_cast<int>(caseSensitive_ ? *i : text::CaseFolder::fold(*i)))
					return false;
			}
			return !i.hasNext();
		}

		bool LiteralPattern::search(const text::detail::CharacterIterator& target, Direction direction,
				text::detail::CharacterIterator& matchedFirst, text::detail::CharacterIterator& matchedLast) const {
			const_cast<LiteralPattern*>(this)->makeShiftTable(direction);
			// TODO: this implementation is just scrath.
			text::detail::CharacterIterator t(target);
			const std::vector<int>::const_iterator b(collationElements_.cbegin()), e(collationElements_.cend());
			if(direction == Direction::forward()) {
				std::advance(t, collationElements_.size() - 1);
				for(std::vector<int>::const_iterator pattern; t.hasNext(); std::advance(t,
						std::max<Index>(lastOccurences_[caseSensitive_ ? *t : text::CaseFolder::fold(*t)], e - pattern))) {
					for(pattern = e - 1;
						(caseSensitive_ ? *t : text::CaseFolder::fold(*t)) == (caseSensitive_ ? *pattern : text::CaseFolder::fold(static_cast<CodePoint>(*pattern)));
						--t, --pattern) {
						if(pattern == b) {
							matchedFirst = matchedLast = t;
							std::advance(matchedLast, collationElements_.size());
							return true;
						}
					}
				}
			} else {
				std::ptrdiff_t skipLength;
				std::advance(t, collationElements_.size());
				for(std::vector<int>::const_iterator pattern; ; std::advance(t, -skipLength)) {
					for(pattern = b;
							(caseSensitive_ ? *t : text::CaseFolder::fold(*t)) == (caseSensitive_ ? *pattern : text::CaseFolder::fold(static_cast<CodePoint>(*pattern)));
							++t, ++pattern) {
						if(pattern == e) {
							std::advance(t, collationElements_.size() + 1);
							matchedFirst = matchedLast = t;
							std::advance(matchedLast, collationElements_.size());
							return true;
						}
					}
					skipLength = std::max(lastOccurences_[caseSensitive_ ? *t : text::CaseFolder::fold(*t)], pattern - b + 1);
					if(skipLength > t.offset() - target.offset())
						break;
				}
			}
			return false;
		}


		// TextSearcher ///////////////////////////////////////////////////////////////////////////////////////////////

		const std::size_t TextSearcher::DEFAULT_NUMBER_OF_STORED_STRINGS = 16;
		const std::size_t TextSearcher::MINIMUM_NUMBER_OF_STORED_STRINGS = 4;

		namespace {
			inline kernel::DocumentCharacterIterator beginningOfDocument(const kernel::Document& document) BOOST_NOEXCEPT {
				return kernel::DocumentCharacterIterator(document, *boost::const_begin(document.region()));
			}
			inline kernel::DocumentCharacterIterator endOfDocument(const kernel::Document& document) BOOST_NOEXCEPT {
				return kernel::DocumentCharacterIterator(document, *boost::const_end(document.region()));
			}
		}

		/**
		 * @class ascension::searcher::TextSearcher
		 *
		 * Searches the specified pattern in the document.
		 *
		 * A session holds an instance of this class, while client can create instances.
		 *
		 * @c TextSearcher has the list of patterns used for search. The pattern which is given by @c #setPattern
		 * method is pushed into this list, and the client can reuse those patterns later. @c IncrementalSearcher uses
		 * this list to get the pattern used previously. To get this stored patterns, call #pattern method. To get the
		 * length of the list, call @c #numberOfStoredPatterns method. The maximum length of the list can be changed by
		 * calling @c #setMaximumNumberOfStoredStrings method. Default length is 16 and the minimum is 4.
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
		 * @note This feature is not implemented and returns always @c Collator#IDENTICAL (15).
		 */
		int TextSearcher::collationWeight() const BOOST_NOEXCEPT {
#ifndef ASCENSION_NO_UNICODE_COLLATION
			return Collator::IDENTICAL;
#else
			return 15;
#endif // !ASCENSION_NO_UNICODE_COLLATION
		}

		/// Returns @c false if caseless match is enabled. This setting is obtained from the pattern.
		bool TextSearcher::isCaseSensitive() const BOOST_NOEXCEPT {
			if(literalPattern_.get() != nullptr)
				return literalPattern_->isCaseSensitive();
#ifndef ASCENSION_NO_REGEX
			else if(regexPattern_.get() != nullptr)
				return (regexPattern_->flags() & regex::Pattern::CASE_INSENSITIVE) == 0;
#ifndef ASCENSION_NO_MIGEMO
			else if(migemoPattern_.get() != nullptr)
				return  (migemoPattern_->flags() & regex::Pattern::CASE_INSENSITIVE) == 0;
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
			else
				return true;
		}

		/// Returns @c true if Migemo is available.
		bool TextSearcher::isMigemoAvailable() const BOOST_NOEXCEPT {
#ifdef ASCENSION_NO_MIGEMO
			return false;
#else
			return regex::MigemoPattern::isMigemoInstalled();
#endif // ASCENSION_NO_MIGEMO
		}

#if 0
		/**
		 * Returns @c true if the pattern matches the specified text.
		 * @param document The document
		 * @param target The target to match
		 * @return true if matched
		 * @throw IllegalStateException The pattern is not specified
		 * @throw regex#PatternSyntaxException The speicifed regular expression was invalid
		 * @throw ... Any exceptions specified by Boost.Regex will be thrown if the regular expression error occurred
		 */
		bool TextSearcher::match(const kernel::Document& document, const kernel::Region& target) const {
			bool matched = false;
			const kernel::DocumentCharacterIterator b(document, *boost::const_begin(target)), e(document, *boost::const_end(target));
			compilePattern((options_.type == LITERAL && literalPattern_.get() != nullptr) ? literalPattern_->getDirection() : Direction::forward());
			switch(options_.type) {
				case LITERAL:
					matched = literalPattern_->matches(DocumentCharacterIterator(document, target)) && checkBoundary(b, e);
					break;
#ifndef ASCENSION_NO_REGEX
				case REGULAR_EXPRESSION:
#ifndef ASCENSION_NO_MIGEMO
				case MIGEMO:
#endif // !ASCENSION_NO_MIGEMO
					if(regexMatcher_.get() == nullptr) {
						TextSearcher& self = const_cast<TextSearcher&>(*this);
						self.regexMatcher_ = regexPattern_->matcher(beginningOfDocument(document), endOfDocument(document));
						self.regexMatcher_->useAnchoringBounds(false).useTransparentBounds(true);
					}
					const kernel::DocumentCharacterIterator oldRegionStart(regexMatcher_->regionStart());
					const kernel::DocumentCharacterIterator oldRegionEnd(regexMatcher_->regionEnd());
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
				lastResult_.direction = Direction::forward();
			}
			return matched;
		}
#endif // 0

		/**
		 * Pushes the new string to the stored list.
		 * @param s The string to push
		 * @param forReplacements Set @c true to push to the replacements list
		 */
		void TextSearcher::pushHistory(const String& s, bool forReplacements) {
			std::list<String>& history = forReplacements ? storedReplacements_ : storedPatterns_;
			const std::list<String>::iterator d(boost::range::find(history, s));
			if(d != boost::end(history))
				history.erase(d);
			else if(history.size() == maximumNumberOfStoredStrings_)
				history.pop_back();
			history.push_front(s);
		}

#if 0
		/**
		 * Replaces the specified region with the replacement string.
		 * <p>If the current search pattern does not match @a target, this method will fail and return @c false.<p>
		 * <p>If the stored replacements list is empty, an empty is used as the replacement string.</p>
		 * <p>This method does not begin and terminate an edit collection.</p>
		 * @param document The document
		 * @param target The region to replace
		 * @param[out] endOfReplacement The end of the region covers the replacement. can be @c null
		 * @return true if replaced
		 * @throw kernel#ReadOnlyDocumentException @a document is read only
		 * @throw IllegalStateException The pattern is not specified
		 */
		bool TextSearcher::replace(kernel::Document& document, const kernel::Region& target, kernel::Position* endOfReplacement) const {
			if(document.isReadOnly())
				throw kernel::ReadOnlyDocumentException();
			// check the last result
			if(!lastResult_.matched() || lastResult_.matchedRegion != target || !lastResult_.checkDocumentRevision(document)) {
				if(!match(document, target))
					return false;
			}

			String replacement = !storedReplacements_.empty() ? storedReplacements_.front() : String();
			kernel::Position eor;
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
						assert(regexMatcher_.get() != nullptr);
						replacement.assign(regexMatcher_->replaceInplace(replacement));
						const kernel::Point regionEnd(document, regexMatcher_->regionEnd().tell());
						eor = !replacement.empty() ? document.insert(target.beginning(), replacement) : target.beginning();
						regexMatcher_->endInplaceReplacement(beginningOfDocument(document), endOfDocument(document),
							regexMatcher_->regionStart(), kernel::DocumentCharacterIterator(document, regionEnd),
							kernel::DocumentCharacterIterator(document, eor));
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
			if(endOfReplacement != nullptr)
				*endOfReplacement = eor;
			return true;
		}
#endif // 0

		inline bool checkBoundary(const kernel::DocumentCharacterIterator& first, const kernel::DocumentCharacterIterator& last, TextSearcher::WholeMatch wholeMatch) {
			switch(wholeMatch) {
				case TextSearcher::MATCH_GRAPHEME_CLUSTER: {
					const text::GraphemeBreakIterator<kernel::DocumentCharacterIterator> bi(first);
					return bi.isBoundary(first) && bi.isBoundary(last);
				}
				case TextSearcher::MATCH_WORD: {
					const kernel::Document& document = first.document();
					const text::WordBreakIterator<kernel::DocumentCharacterIterator> bi1(first, text::WordBreakIteratorBase::START_OF_SEGMENT,
						document.contentTypeInformation().getIdentifierSyntax(document.partitioner().contentType(first.tell())));
					if(!bi1.isBoundary(first))
						return false;
					const text::WordBreakIterator<kernel::DocumentCharacterIterator> bi2(last, text::WordBreakIteratorBase::END_OF_SEGMENT,
						document.contentTypeInformation().getIdentifierSyntax(document.partitioner().contentType(last.tell())));
					return bi2.isBoundary(last);
				}
				default:
					return true;
			}
		}

		/**
		 * Searches and replaces all occurences in the specified region.
		 *
		 * If @a callback parameter is not @c null, this method begins <em>interactive replacement</em>. In interactive
		 * replacement, this method finds the occurences match the pattern one by one, queries the callback object
		 * whether to replace it.
		 *
		 * When the callback object changed the document during replacements, this method will stop.
		 *
		 * If the stored replacements list is empty, an empty is used as the replacement string.
		 *
		 * This method does not begin and terminate an <em>compound change</em>.
		 * @param document The document
		 * @param scope The region to search and replace
		 * @param replacement The replacement string
		 * @param callback The callback object for interactive replacement. If @c null, this method
		 *                 replaces all the occurences silently
		 * @return The number of replaced occurences
		 * @throw IllegalStateException The pattern is not specified
		 * @throw ReadOnlyDocumentException @a document is read-only
		 * @throw BadRegionException @a scope intersects outside of the document
		 * @throw ReplacementInterruptedException&lt;DocumentInput#ChangeRejectedException&gt; The input of the
		 *                                                                                     document rejected this
		 *                                                                                     change. If thrown, the
		 *                                                                                     replacement will be
		 *                                                                                     interrupted
		 * @throw ReplacementInterruptedException&lt;std#bad_alloc&gt; The internal memory allocation failed. If
		 *                                                             thrown, the replacement will be interrupted
		 */
		std::size_t TextSearcher::replaceAll(kernel::Document& document, const kernel::Region& scope, const String& replacement, InteractiveReplacementCallback* callback) {
			if(document.isReadOnly())
				throw kernel::ReadOnlyDocumentException();
			else if(!encompasses(document.region(), scope))
				throw kernel::BadRegionException(scope);

//			const String replacement(!storedReplacements_.empty() ? storedReplacements_.front() : String());
			std::size_t numberOfMatches = 0, numberOfReplacements = 0;
			std::stack<std::pair<kernel::Position, kernel::Position>> history;	// for undo (ouch, Region does not support placement new)
			std::size_t documentRevision = document.revisionNumber();	// to detect other interruptions

			InteractiveReplacementCallback::Action action;	// the action the callback returns
			InteractiveReplacementCallback* const storedCallback = callback;
			if(callback != nullptr)
				callback->replacementStarted(document, scope);

			if(type() == LITERAL) {
				kernel::DocumentCharacterIterator matchedFirst, matchedLast;
				kernel::Point endOfScope(document, *boost::const_end(scope));
				for(kernel::DocumentCharacterIterator i(document, scope); i.hasNext(); ) {
					if(!literalPattern_->search(i, Direction::forward(), matchedFirst, matchedLast))
						break;
					else if(!checkBoundary(matchedFirst, matchedLast, wholeMatch_)) {
						++i;
						continue;
					}

					// matched -> replace
					++numberOfMatches;
					kernel::Region matchedRegion(matchedFirst.tell(), matchedLast.tell());
					while(true) {
						action = (callback != nullptr) ?
							callback->queryReplacementAction(matchedRegion, !history.empty()) : InteractiveReplacementCallback::REPLACE;
						if(action != InteractiveReplacementCallback::UNDO)
							break;
						if(!history.empty()) {
							// undo the last replacement
							matchedRegion = kernel::Region::fromTuple(history.top());
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

					if(action == InteractiveReplacementCallback::REPLACE
							|| action == InteractiveReplacementCallback::REPLACE_ALL
							|| action == InteractiveReplacementCallback::REPLACE_AND_EXIT) {
						// replace? -- yes
						if(action == InteractiveReplacementCallback::REPLACE_ALL)
							callback = nullptr;
						if(!boost::empty(matchedRegion) || !replacement.empty()) {
							kernel::Position e;
							try {
								e = document.replace(matchedRegion, replacement);
							} catch(const kernel::DocumentInput::ChangeRejectedException&) {
								throw ReplacementInterruptedException<kernel::DocumentInput::ChangeRejectedException>(numberOfReplacements);
							} catch(const std::bad_alloc&) {
								throw ReplacementInterruptedException<std::bad_alloc>(numberOfReplacements);
							}
							i.seek(e);
							i.setRegion(kernel::Region(*boost::const_begin(scope), endOfScope.position()));
							documentRevision = document.revisionNumber();
						}
						++numberOfReplacements;
						history.push(std::make_pair(*boost::const_begin(matchedRegion), *boost::const_end(matchedRegion)));
					} else if(action == InteractiveReplacementCallback::SKIP)
						i.seek(*boost::const_end(matchedRegion));
					if(action == InteractiveReplacementCallback::REPLACE_AND_EXIT || action == InteractiveReplacementCallback::EXIT)
						break;
				}
			}

#ifndef ASCENSION_NO_REGEX
			if(type() == REGULAR_EXPRESSION
#ifndef ASCENSION_NO_MIGEMO
					|| type() == MIGEMO
#endif // !ASCENSION_NO_MIGEMO
			) {
				const kernel::Point endOfScope(document, *boost::const_end(scope));
				kernel::Position lastEOS(endOfScope.position());
				kernel::DocumentCharacterIterator e(document, endOfScope.position());
				kernel::DocumentCharacterIterator b(e);
				std::unique_ptr<regex::Matcher<kernel::DocumentCharacterIterator>> matcher(
					regexPattern_->matcher(beginningOfDocument(document), endOfDocument(document)));
				matcher->region(
					kernel::DocumentCharacterIterator(document, *boost::const_begin(scope)),
					kernel::DocumentCharacterIterator(document, *boost::const_end(scope)))
					.useAnchoringBounds(false).useTransparentBounds(true);
				lastResult_.reset();

				while(matcher->find()) {
					if(!checkBoundary(matcher->start(), matcher->end(), wholeMatch_))
						matcher->region(++kernel::DocumentCharacterIterator(matcher->start()), matcher->end());
					else {
						kernel::Position next(*boost::const_begin(document.region()));
						// matched -> replace
						++numberOfMatches;
						kernel::Region matchedRegion(matcher->start().tell(), matcher->end().tell());
						while(true) {
							action = (callback != nullptr) ?
								callback->queryReplacementAction(matchedRegion, !history.empty()) : InteractiveReplacementCallback::REPLACE;
							if(action != InteractiveReplacementCallback::UNDO)
								break;
							if(!history.empty()) {
								// undo the last replacement
								matchedRegion = kernel::Region::fromTuple(history.top());
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

						if(action == InteractiveReplacementCallback::REPLACE
								|| action == InteractiveReplacementCallback::REPLACE_ALL
								|| action == InteractiveReplacementCallback::REPLACE_AND_EXIT) {
							// replace? -- yes
							if(action == InteractiveReplacementCallback::REPLACE_ALL)
								callback = nullptr;
							history.push(std::make_pair(*boost::const_begin(matchedRegion), *boost::const_end(matchedRegion)));
							assert(!boost::empty(matchedRegion) || !replacement.empty());
							try {
								document.replace(matchedRegion, matcher->replaceInplace(replacement));
							} catch(const kernel::DocumentInput::ChangeRejectedException&) {
								throw ReplacementInterruptedException<kernel::DocumentInput::ChangeRejectedException>(numberOfReplacements);
							} catch(const std::bad_alloc&) {
								throw ReplacementInterruptedException<std::bad_alloc>(numberOfReplacements);
							}
							if(!boost::empty(matchedRegion))
								next = *boost::const_begin(matchedRegion);
							if(!replacement.empty()) {
								matcher->endInplaceReplacement(beginningOfDocument(document), endOfDocument(document),
									kernel::DocumentCharacterIterator(document, *boost::const_begin(scope)), kernel::DocumentCharacterIterator(document, endOfScope.position()),
									kernel::DocumentCharacterIterator(document, next));
								documentRevision = document.revisionNumber();
							}
						} else if(action == InteractiveReplacementCallback::SKIP)
							next = *boost::const_end(matchedRegion);
						if(action == InteractiveReplacementCallback::REPLACE_AND_EXIT || action == InteractiveReplacementCallback::EXIT)
							break;

						if(*boost::const_end(matchedRegion) == e.tell())	// reached the end of the scope
							break;
						else if(endOfScope.position() != lastEOS) {
							e.setRegion(kernel::Region(*boost::const_begin(scope), endOfScope.position()));
							e.seek(endOfScope.position());
							lastEOS = endOfScope.position();
						}
						if(next < *boost::const_end(matchedRegion))
							next = *boost::const_end(matchedRegion);
					}
				}
			}
#endif // !ASCENSION_NO_REGEX

			if(storedCallback != nullptr)
				storedCallback->replacementEnded(numberOfMatches, numberOfReplacements);
			pushHistory(replacement, true);	// only this call make this method not-const...
			return numberOfReplacements;
		}

		/**
		 * Searches the pattern in the document.
		 * @param document The document
		 * @param from The position where the search begins
		 * @param scope The region to search
		 * @param direction The direction to search
		 * @param[out] matchedRegion The matched region. The value is not changed unless the process successes
		 * @return true if the pattern is found
		 * @throw IllegalStateException The pattern is not specified
		 * @throw kernel#BadPositionException @a from is outside of @a scope
		 * @throw ... Any exceptions specified by Boost.Regex will be thrown if the regular expression error occurred
		 */
		bool TextSearcher::search(const kernel::Document& document,
				const kernel::Position& from, const kernel::Region& scope, Direction direction, kernel::Region& matchedRegion) const {
			if(!encompasses(scope, from))
				throw kernel::BadPositionException(from);
			bool matched = false;
			if(type() == LITERAL) {
				kernel::DocumentCharacterIterator matchedFirst, matchedLast;
				for(kernel::DocumentCharacterIterator i(document, scope, from);
						(direction == Direction::forward()) ? i.hasNext() : i.hasPrevious();
						(direction == Direction::forward()) ? ++i : --i) {
					if(!literalPattern_->search(i, direction, matchedFirst, matchedLast))
						break;	// not found
					else if(checkBoundary(matchedFirst, matchedLast, wholeMatch_)) {
						matchedRegion = kernel::Region(matchedFirst.tell(), matchedLast.tell());
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
				if(regexMatcher_.get() == nullptr)
					(const_cast<TextSearcher*>(this)->regexMatcher_ = regexPattern_->matcher(
						beginningOfDocument(document), endOfDocument(document)))->useAnchoringBounds(false).useTransparentBounds(true);
				else if(!lastResult_.checkDocumentRevision(document) || direction != lastResult_.direction) {
					const_cast<TextSearcher*>(this)->regexMatcher_->reset(beginningOfDocument(document), endOfDocument(document));
					lastResult_.reset();
				}

				const bool maybeContinuous = lastResult_.matched()
					&& direction == lastResult_.direction && lastResult_.checkDocumentRevision(document);
				if(direction == Direction::forward()) {
					const kernel::DocumentCharacterIterator eob(document, *boost::const_end(scope));
					if(!maybeContinuous || from != *boost::const_end(*lastResult_.matchedRegion))
						regexMatcher_->region(kernel::DocumentCharacterIterator(document, from), eob);
					while(regexMatcher_->find()) {
						if(matched = checkBoundary(regexMatcher_->start(), regexMatcher_->end(), wholeMatch_))
							break;
						regexMatcher_->region(++kernel::DocumentCharacterIterator(regexMatcher_->start()), eob);
					}
				} else {
					// ascension.regex does not support backward searches...
					const bool continuous = maybeContinuous && from == *boost::const_begin(*lastResult_.matchedRegion);
					const kernel::DocumentCharacterIterator e(document, continuous ? *boost::const_end(*lastResult_.matchedRegion) : from);
					kernel::DocumentCharacterIterator b(document, from);	// position from where the match should start
					if(!continuous || b.tell() > *boost::const_begin(scope)) {
						if(continuous)
							--b;
						while(true) {
							regexMatcher_->region(b, e);
							if(matched = (regexMatcher_->lookingAt()
									&& checkBoundary(regexMatcher_->start(), regexMatcher_->end(), wholeMatch_)))
								break;
							else if(b.tell() <= *boost::const_begin(scope))
								break;
							--b;	// move to the next search start
						}
					}
				}
				if(matched)
					matchedRegion = kernel::Region(regexMatcher_->start().tell(), regexMatcher_->end().tell());
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

		/// Sets the maximum number of the stored patterns or replacement strings.
		void TextSearcher::setMaximumNumberOfStoredStrings(std::size_t number) BOOST_NOEXCEPT {
			number = std::max<std::size_t>(number, MINIMUM_NUMBER_OF_STORED_STRINGS);
			if(storedPatterns_.size() > number)
				storedPatterns_.resize(number);
			if(storedReplacements_.size() > number)
				storedReplacements_.resize(number);
			maximumNumberOfStoredStrings_ = number;
		}
		
		/**
		 * @fn ascension::searcher::TextSearcher::setPattern
		 * @brief Sets the new pattern.
		 * @tparam PatternType The pattern type. Can be @c LiteralPattern, @c regex#Pattern or @c regex#MigemoPattern
		 * @param pattern The pattern string
		 * @param dontRemember Set @c true to not add the pattern into the stored list. In this case, the following
		 *                     @c #pattern call will not return the pattern set by this
		 * @return this object
		 */

		/**
		 * Sets the "whole match" condition.
		 * @param newValue the new whole match value to set
		 * @return This object
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
		TextSearcher::Type TextSearcher::type() const BOOST_NOEXCEPT {
#ifndef ASCENSION_NO_REGEX
			if(regexPattern_.get() != nullptr)
				return REGULAR_EXPRESSION;
#ifndef ASCENSION_NO_MIGEMO
			else if(migemoPattern_.get() != nullptr)
				return MIGEMO;
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
		//	if(literalPattern_.get() != nullptr)
				return LITERAL;
		}

		/// Returns @c true if the pattern uses Unicode canonical equivalents.
		bool TextSearcher::usesCanonicalEquivalents() const BOOST_NOEXCEPT {
#ifndef ASCENSION_NO_REGEX
			if(regexPattern_.get() != nullptr && (regexPattern_->flags() & regex::Pattern::CANON_EQ) != 0)
				return true;
#endif  // !ASCENSION_NO_REGEX
			return false;
		}

		/// Returns the "whole match" condition.
		TextSearcher::WholeMatch TextSearcher::wholeMatch() const BOOST_NOEXCEPT {
			return wholeMatch_;
		}


		// TextSearcher.LastResult ////////////////////////////////////////////////////////////////////////////////////

		inline bool TextSearcher::LastResult::checkDocumentRevision(const kernel::Document& current) const BOOST_NOEXCEPT {
			return document == &current && documentRevisionNumber == current.revisionNumber();
		}

		inline void TextSearcher::LastResult::updateDocumentRevision(const kernel::Document& document) BOOST_NOEXCEPT {
			this->document = &document;
			documentRevisionNumber = document.revisionNumber();
		}


		// IncrementalSearcher ////////////////////////////////////////////////////////////////////////////////////////

		/// Constructor.
		IncrementalSearcher::IncrementalSearcher() BOOST_NOEXCEPT : type_(TextSearcher::LITERAL) {
		}

		/// Aborts the search.
		void IncrementalSearcher::abort() {
			if(isRunning()) {
				if(callback_ != nullptr) {
					while(statusHistory_.size() > 1)
						statusHistory_.pop();
					callback_->incrementalSearchAborted(*boost::const_begin(statusHistory_.top().matchedRegion));
				}
				end();
			}
		}

		/**
		 * Appends the specified character to the end of the current search pattern.
		 * @param c The character to append
		 * @return true if the pattern is found
		 * @throw IllegalStateException The searcher is not running
		 */
		bool IncrementalSearcher::addCharacter(Char c) {
			checkRunning();
			pattern_ += c;
			operationHistory_.push(TYPE);
			return update();
		}

		/**
		 * Appends the specified character to the end of the current search pattern.
		 * @param c The character to append
		 * @return true if the pattern is found
		 * @throw IllegalStateException The searcher is not running
		 */
		bool IncrementalSearcher::addCharacter(CodePoint c) {
			checkRunning();
			if(c < 0x010000u)
				return addCharacter(static_cast<Char>(c & 0xffffu));
			Char surrogates[2];
			return addString(String(surrogates, text::utf::checkedEncode(c, surrogates)));
		}

		/**
		 * Appends the specified string to the end of the search pattern.
		 * @param text The string to append
		 * @return true if the pattern is found
		 * @throw IllegalStateException The searcher is not running
		 * @throw NotRunningException The searcher is not running
		 * @throw NullPointerException @a text is @c null
		 * @throw std#invalid_argument @a text is empty
		 * @throw ... Any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
		 */
		bool IncrementalSearcher::addString(const StringPiece& text) {
			if(text.cbegin() == nullptr)
				throw NullPointerException("text");
			checkRunning();
			if(text.empty())
				throw std::invalid_argument("Added string is empty.");
			pattern_.append(text.cbegin(), text.cend());
			for(const Char* p = text.cbegin(); p < text.cend(); ++p)
				operationHistory_.push(TYPE);
			return update();
		}

		/// @see kernel#BookmarkListener#bookmarkChanged
		void IncrementalSearcher::bookmarkChanged(Index) {
			abort();
		}

		/// @see kernel#BookmarkListener#bookmarkCleared
		void IncrementalSearcher::bookmarkCleared() {
			abort();
		}

		/// @internal Throws an @c IllegalStateException if not in running.
		void IncrementalSearcher::checkRunning() const {
			if(!isRunning())
				throw IllegalStateException("The incremental searcher is not running.");
		}

		/// @see kernel#DocumentListener#documentAboutToBeChanged
		void IncrementalSearcher::documentAboutToBeChanged(const kernel::Document&) {
			abort();
		}

		/// @see kernel#DocumentListener#documentChanged
		void IncrementalSearcher::documentChanged(const kernel::Document&, const kernel::DocumentChange&) {
		}

		/// Ends the search.
		void IncrementalSearcher::end() {
			if(isRunning()) {
				document_->removeListener(*this);
				document_->bookmarker().removeListener(*this);
				while(!statusHistory_.empty())
					statusHistory_.pop();
				if(callback_ != nullptr)
					callback_->incrementalSearchCompleted();
				if(!pattern_.empty())
					setPatternToSearcher(true);	// store to reuse
				searcher_ = nullptr;
				callback_ = nullptr;
				pattern_.erase();
			}
		}

		/**
		 * Search the next match. If the pattern is empty, this method uses the last used pattern.
		 * @param direction The new direction of the search
		 * @return true if matched after jump
		 * @throw IllegalStateException The searcher is not running
		 */
		bool IncrementalSearcher::next(Direction direction) {
			checkRunning();
			if(pattern_.empty()) {
				statusHistory_.top().direction = direction;
				if(searcher_->numberOfStoredPatterns() > 0)
					return addString(searcher_->pattern());	// use the most recent used
				else {
					if(callback_ != nullptr)
						callback_->incrementalSearchPatternChanged(IncrementalSearchCallback::EMPTY_PATTERN, IncrementalSearchCallback::NO_WRAPPED);
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
		 * @throw IllegalStateException The searcher is not running
		 */
		void IncrementalSearcher::reset() {
			checkRunning();
			while(!operationHistory_.empty())
				operationHistory_.pop();
			while(statusHistory_.size() > 1)
				statusHistory_.pop();
			pattern_.erase();
			if(callback_ != nullptr)
				callback_->incrementalSearchPatternChanged(IncrementalSearchCallback::EMPTY_PATTERN, IncrementalSearchCallback::NO_WRAPPED);
		}

		inline void IncrementalSearcher::setPatternToSearcher(bool pushToHistory) {
			if(pattern_.empty())
				throw IllegalStateException("the pattern is empty.");
			switch(type_) {
				case TextSearcher::LITERAL:
					// TODO: specify 'collator' parameter.
					searcher_->setPattern(
						std::unique_ptr<const LiteralPattern>(new LiteralPattern(pattern_, searcher_->isCaseSensitive())), pushToHistory);
					break;
#ifndef ASCENSION_NO_REGEX
				case TextSearcher::REGULAR_EXPRESSION:
					searcher_->setPattern(regex::Pattern::compile(pattern_,
						regex::Pattern::MULTILINE | (searcher_->isCaseSensitive() ? 0 : regex::Pattern::CASE_INSENSITIVE)), pushToHistory);
					break;
#ifndef ASCENSION_NO_MIGEMO
				case TextSearcher::MIGEMO:
					searcher_->setPattern(regex::MigemoPattern::compile(pattern_, searcher_->isCaseSensitive()), pushToHistory);
					break;
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
			}
		}

		/**
		 * Starts the search.
		 * @param document The document to search
		 * @param from The position at which the search starts
		 * @param searcher The text search object
		 * @param type The search type
		 * @param direction The initial search direction
		 * @param callback The callback object. can be @c null
		 */
		void IncrementalSearcher::start(kernel::Document& document, const kernel::Position& from, TextSearcher& searcher,
				TextSearcher::Type type, Direction direction, IncrementalSearchCallback* callback /* = nullptr */) {
			if(isRunning())
				end();
			const Status s = {kernel::Region(from, from), direction};
			assert(statusHistory_.empty() && pattern_.empty());
			statusHistory_.push(s);
			(document_ = &document)->addListener(*this);
			document_->bookmarker().addListener(*this);
			searcher_ = &searcher;
			type_ = type;
			matchedRegion_ = statusHistory_.top().matchedRegion;
			if(nullptr != (callback_ = callback)) {
				callback_->incrementalSearchStarted(document);
				callback_->incrementalSearchPatternChanged(IncrementalSearchCallback::EMPTY_PATTERN, IncrementalSearchCallback::NO_WRAPPED);
			}
		}

		/**
		 * Undoes the last search command. If the last command is typing, the end of the pattern is removed. Otherwise
		 * if researching, reverts to the last state.
		 * @return true if matched after the undo 
		 * @throw IllegalStateException The searcher is not running or the undo buffer is empty
		 */
		bool IncrementalSearcher::undo() {
			checkRunning();
			if(!canUndo())
				throw IllegalStateException("Undo buffer of incremental search is empty and not undoable.");

			const Operation lastOperation = operationHistory_.top();
			operationHistory_.pop();
			if(lastOperation == TYPE) {	// undo the last typing -> delete the last character in the pattern
				if(pattern_.length() > 1
						&& text::surrogates::isHighSurrogate(pattern_[pattern_.length() - 2])
						&& text::surrogates::isLowSurrogate(pattern_[pattern_.length() - 1])) {
					pattern_.erase(pattern_.length() - 2);
					operationHistory_.pop();
				} else
					pattern_.erase(pattern_.length() - 1);
				return update();
			} else if(lastOperation == JUMP) {	// undo the last jump -> revert to the last state
				matchedRegion_ = statusHistory_.top().matchedRegion;
				statusHistory_.pop();
				assert(!statusHistory_.empty());
				if(!matched_) {	// ... and should be matched state
					matched_ = true;
					if(callback_ != nullptr)
						callback_->incrementalSearchPatternChanged(IncrementalSearchCallback::FOUND, IncrementalSearchCallback::NO_WRAPPED);
				}
				return true;
			}
			ASCENSION_ASSERT_NOT_REACHED();
		}

		/**
		 * Re-searches using the current state.
		 * @return true if the pattern is found
		 * @throw ... Any exceptions specified by Boost.Regex will be thrown if the regular expression error occurred
		 */
		bool IncrementalSearcher::update() {
			const Status& lastStatus = statusHistory_.top();
			if(pattern_.empty()) {
				assert(statusHistory_.size() == 1);
				matchedRegion_ = lastStatus.matchedRegion;
				if(callback_ != nullptr)
					callback_->incrementalSearchPatternChanged(
						IncrementalSearchCallback::EMPTY_PATTERN, IncrementalSearchCallback::NO_WRAPPED);
				return true;
			}
			setPatternToSearcher(false);

			kernel::Region matchedRegion, scope(document_->accessibleRegion());
/*			if(statusHistory_.size() > 1 && lastStatus.matchedRegion.isEmpty()) {
				// handle the previous zero-width match
				if(lastStatus.direction == Direction::forward()) {
					DocumentCharacterIterator temp(*document_, scope.first);
					temp.next();
					scope.first = temp.tell();
				} else {
					DocumentCharacterIterator temp(*document_, scope.second);
					temp.previous();
					scope.second = temp.tell();
				}
			}
*/			matched_ = false;
#ifndef ASCENSION_NO_REGEX
			try {
#endif // !ASCENSION_NO_REGEX
				matched_ = searcher_->search(*document_,
					(lastStatus.direction == Direction::forward()) ? *boost::const_end(lastStatus.matchedRegion) : *boost::const_begin(lastStatus.matchedRegion),
					scope, lastStatus.direction, matchedRegion);
#ifndef ASCENSION_NO_REGEX
			} catch(boost::regex_error&) {
				if(callback_ != nullptr)
					callback_->incrementalSearchPatternChanged(IncrementalSearchCallback::BAD_REGEX, IncrementalSearchCallback::NO_WRAPPED);
				return false;
			} catch(std::runtime_error&) {
				if(callback_ != nullptr)
					callback_->incrementalSearchPatternChanged(IncrementalSearchCallback::COMPLEX_REGEX, IncrementalSearchCallback::NO_WRAPPED);
				return false;
			}
#endif // !ASCENSION_NO_REGEX

			if(matched_)
				matchedRegion_ = matchedRegion;
			if(callback_ != nullptr)
				callback_->incrementalSearchPatternChanged(matched_ ?
					IncrementalSearchCallback::FOUND : IncrementalSearchCallback::NOT_FOUND, IncrementalSearchCallback::NO_WRAPPED);
			return matched_;
		}
	}
}
