/**
 * @file searcher.hpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.h)
 * @date 2006-2010
 */

#ifndef ASCENSION_SEARCHER_HPP
#define ASCENSION_SEARCHER_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_COLLATION, ASCENSION_NO_REGEX, ...
#include <ascension/kernel/document-character-iterator.hpp>
#ifndef ASCENSION_NO_REGEX
#	include <ascension/corelib/regex.hpp>
#endif // !ASCENSION_NO_REGEX
#include <list>
#include <stack>

namespace ascension {

	namespace text {class BreakIterator;}

#ifndef ASCENSION_NO_REGEX
	namespace regex {template<typename CodePointIterator> class Matcher;};
#endif // !ASCENSION_NO_REGEX

	namespace searcher {

		/**
		 * @note This class is not intended to be subclassed.
		 */
		class LiteralPattern {
			ASCENSION_NONCOPYABLE_TAG(LiteralPattern);
		public:
			// constructors
			LiteralPattern(const String& pattern, bool caseSensitive = true,
				std::auto_ptr<const text::Collator> collator = std::auto_ptr<const text::Collator>(0));
			~LiteralPattern() /*throw()*/;
			// attributes
			bool isCaseSensitive() const /*throw()*/;
			const String& pattern() const /*throw()*/;
			// operations
			bool matches(const text::CharacterIterator& target) const;
			bool search(const text::CharacterIterator& target, Direction direction,
				std::auto_ptr<text::CharacterIterator>& matchedFirst, std::auto_ptr<text::CharacterIterator>& matchedLast) const;
		private:
			void makeShiftTable(Direction direction) /*throw()*/;
		private:
			const String pattern_;
			const bool caseSensitive_;
#ifndef ASCENSION_NO_UNICODE_COLLATION
			const std::auto_ptr<const text::Collator> collator_;
#endif // !ASCENSION_NO_UNICODE_COLLATION
			std::ptrdiff_t lastOccurences_[65536];	// for forward search
			std::ptrdiff_t firstOccurences_[65536];	// for backward search
			int* first_;	// collation elements of the pattern
			int* last_;
		};

		/**
		 * A callback defines reactions about the interactive replacement.
		 * @see TextSearcher#replaceAll
		 */
		class IInteractiveReplacementCallback {
		protected:
			/// Reactions for @c IInteractiveReplacementCallback#queryReplacementAction method.
			enum Action {
				REPLACE,			///< Replaces the matched region with the replacement, and continues.
				SKIP,				///< Skips to the next without replacing.
				REPLACE_ALL,		///< Replaces all remaining matches without queries.
				REPLACE_AND_EXIT,	///< Replaces the matched region and then exits without searching.
				UNDO,				///< Does undo the last replacement.
				EXIT				///< Exits the replacements.
			};
		private:
			/**
			 * Returns how the text searcher should perform about matched text.
			 * @param matchedRegion The region matched the pattern
			 * @param canUndo This call can return the value @c UNDO as an action
			 * @return The action the text searcher should perform
			 */
			virtual Action queryReplacementAction(const kernel::Region& matchedRegion, bool canUndo) = 0;
			/**
			 * The replacement exited or explicitly aborted.
			 * @param numberOfMatched
			 * @param numberOfReplacements
			 */
			virtual void replacementEnded(std::size_t numberOfMatches, std::size_t numberOfReplacements) = 0;
			/**
			 * The replacement started.
			 * @param document The document to search and replace
			 * @param scope The region to perform
			 */
			virtual void replacementStarted(const kernel::Document& document, const kernel::Region& scope) = 0;
			friend class TextSearcher;
		};

		// the documentation is searcher.cpp
		template<typename SourceException>
		class ReplacementInterruptedException : public SourceException {
		public:
			explicit ReplacementInterruptedException(std::size_t numberOfReplacements);
			ReplacementInterruptedException(const char* message, std::size_t numberOfReplacements);
			std::size_t numberOfReplacements() const /*throw()*/;
		private:
			const std::size_t numberOfReplacements_;
		};

		// the documentation is searcher.cpp
		class TextSearcher {
			ASCENSION_NONCOPYABLE_TAG(TextSearcher);
		public:
			/// Types of search.
			enum Type {
				LITERAL,			///< Literal search.
#ifndef ASCENSION_NO_REGEX
				REGULAR_EXPRESSION,	///< Regular expression search.
#ifndef ASCENSION_NO_MIGEMO
				MIGEMO				///< Migemo.
#endif // !ASCENSION_NO_REGEX
#endif // !ASCENSION_NO_MIGEMO
			};
			/// Constraint the edges of the matched region must satisfy.
			enum WholeMatch {
				/// No constraint.
				MATCH_UTF32_CODE_UNIT,
				/// The start and the end of the match region must be grapheme cluster boundaries.
				MATCH_GRAPHEME_CLUSTER,
				/// The start and the end of the match region must be word boundaries (called whole word match).
				MATCH_WORD
			};
		public:
			// constructor
			TextSearcher();
			// pattern/replacement
			bool hasPattern() const /*throw()*/;
			std::size_t numberOfStoredPatterns() const /*throw()*/;
			std::size_t numberOfStoredReplacements() const /*throw()*/;
			const String& pattern(std::size_t index = 0) const;
			const String& replacement(std::size_t index = 0) const;
			void setMaximumNumberOfStoredStrings(std::size_t number) /*throw()*/;
			template<typename PatternType>
			TextSearcher& setPattern(std::auto_ptr<const PatternType> pattern, bool dontRemember = false);
			// search conditions
			int collationWeight() const /*throw()*/;
			bool isCaseSensitive() const /*throw()*/;
			TextSearcher& setWholeMatch(WholeMatch newValue);
			Type type() const /*throw()*/;
			bool usesCanonicalEquivalents() const /*throw()*/;
			WholeMatch wholeMatch() const /*throw()*/;
//			TextSearcher& wrapAround(bool wrap) /*throw()*/;
//			bool wrapsAround() const /*throw()*/;
			// result
			bool isLastPatternMatched() const /*throw()*/;
			// services
			bool isMigemoAvailable() const /*throw()*/;
			static bool isRegexAvailable() /*throw()*/;
			// operations
			void abortInteractiveReplacement();
			std::size_t replaceAll(
				kernel::Document& document, const kernel::Region& scope,
				const String& replacement, IInteractiveReplacementCallback* callback);
			bool search(const kernel::Document& document, const kernel::Position& from,
				const kernel::Region& scope, Direction direction, kernel::Region& matchedRegion) const;
			template<typename InputIterator>
			void setStoredStrings(InputIterator first, InputIterator last, bool forReplacements);
		private:
			void pushHistory(const String& s, bool forReplacements);
		private:
			std::auto_ptr<const LiteralPattern> literalPattern_;
#ifndef ASCENSION_NO_REGEX
			std::auto_ptr<const regex::Pattern> regexPattern_;
			std::auto_ptr<const regex::MigemoPattern> migemoPattern_;
			std::auto_ptr<regex::Matcher<kernel::DocumentCharacterIterator> > regexMatcher_;
#endif // !ASCENSION_NO_REGEX
			mutable struct LastResult {
				const kernel::Document* document;
				kernel::Region matchedRegion;
				Direction direction;
				std::size_t documentRevisionNumber;
				LastResult() /*throw()*/ : document(0), matchedRegion(), direction(Direction::FORWARD) {}
				~LastResult() /*throw()*/ {reset();}
				bool checkDocumentRevision(const kernel::Document& current) const /*throw()*/ {
					return document == &current && documentRevisionNumber == current.revisionNumber();}
				bool matched() const /*throw()*/ {return matchedRegion.first != kernel::Position();}
				void reset() /*throw()*/ {matchedRegion.first = matchedRegion.second = kernel::Position();}
				void updateDocumentRevision(const kernel::Document& document) /*throw()*/ {
					this->document = &document; documentRevisionNumber = document.revisionNumber();}
			} lastResult_;
			Type searchType_;
			WholeMatch wholeMatch_;
//			bool wrapsAround_;
			std::list<String> storedPatterns_, storedReplacements_;
			std::size_t maximumNumberOfStoredStrings_;
			bool abortedInteractiveReplacement_;
			enum {DEFAULT_NUMBER_OF_STORED_STRINGS = 16, MINIMUM_NUMBER_OF_STORED_STRINGS = 4};
		};

		/**
		 * Represents a callback object observes the state of the incremental searcher.
		 * @see IncrementalSearcher
		 */
		class IIncrementalSearchCallback {
		protected:
			/// Temporary result of an incremental search.
			enum Result {
				EMPTY_PATTERN,	///< The pattern is empty.
				FOUND,			///< The pattern is found.
				NOT_FOUND,		///< The pattern is not found.
				COMPLEX_REGEX,	///< The regular expression is too complex.
				BAD_REGEX		///< The regular expression is invalid.
			};
			/// Wrapping status of an incremental search.
			enum WrappingStatus {
				NO_WRAPPED,		///< A wrapping is not happened.
				WRAPPED_AROUND,	///< The scanning was over the end/start of the target region.
				OVERWRAPPED		///< The scanning reached the position where the search started.
			};
		private:
			/**
			 * The search was aborted.
			 * @param initialPosition The position at which the search started.
			 */
			virtual void incrementalSearchAborted(const kernel::Position& initialPosition) = 0;
			/// The search was completed successfully.
			virtual void incrementalSearchCompleted() = 0;
			/**
			 * The search pattern was changed.
			 * @param result the result on new pattern.
			 */
			virtual void incrementalSearchPatternChanged(Result result, const Flags<WrappingStatus>& wrappingStatus) = 0;
			/**
			 * The search was started. @c incrementalSearchPatternChanged is also called with
			 * @c EMPTY_PATTERN after this.
			 * @param document The document to search
			 */
			virtual void incrementalSearchStarted(const kernel::Document& document) = 0;
			friend class IncrementalSearcher;
		};


		/**
		 * @c IncrementalSearcher performs incremental search on a document. A session holds an
		 * instance of this class, while client can create instances.
		 * The search will abort automatically when the content or the bookmark of the document
		 * was changed.
		 * @see TextSearcher, texteditor#Session#getIncrementalSearcher,
		 * texteditor#commands#IncrementalSearchCommand
		 */
		class IncrementalSearcher : public kernel::IDocumentListener, public kernel::IBookmarkListener {
			ASCENSION_NONCOPYABLE_TAG(IncrementalSearcher);
		public:
			// constructor
			IncrementalSearcher() /*throw()*/;
			// attributes
			bool canUndo() const /*throw()*/;
			Direction direction() const;
			bool isRunning() const /*throw()*/;
			const kernel::Region& matchedRegion() const;
			const String& pattern() const;
			TextSearcher::Type type() const;
			// operations
			void abort();
			bool addCharacter(Char c);
			bool addCharacter(CodePoint c);
			bool addString(const StringPiece& text);
			void end();
			bool next(Direction direction);
			void reset();
			void start(kernel::Document& document, const kernel::Position& from,
				TextSearcher& searcher, TextSearcher::Type type, Direction direction,
				IIncrementalSearchCallback* callback = 0);
			bool undo();
		private:
			void setPatternToSearcher(bool pushToHistory);
			bool update();
			// kernel.IDocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// kernel.IBookmarkListener
			void bookmarkChanged(length_t line);
			void bookmarkCleared();

		private:
			void checkRunning() const;
			enum Operation {TYPE, JUMP};
			struct Status {
				kernel::Region matchedRegion;
				Direction direction;
			};
			kernel::Document* document_;
			TextSearcher* searcher_;
			IIncrementalSearchCallback* callback_;
			kernel::Region matchedRegion_;
			std::stack<Operation> operationHistory_;
			std::stack<Status> statusHistory_;
			String pattern_;
			TextSearcher::Type type_;
			bool matched_;	// true if matched in the last update() call
		};


// inline ///////////////////////////////////////////////////////////////////

	/// Returns @c true if the pattern performs case-sensitive match.
	inline bool LiteralPattern::isCaseSensitive() const /*throw()*/ {return caseSensitive_;}
	/// Returns the pattern string.
	inline const String& LiteralPattern::pattern() const /*throw()*/ {return pattern_;}
	/**
	 * Constructor.
	 * @param numberOfReplacements The number of the replacements the object done successfully
	 */
	template<typename SourceException>
	inline ReplacementInterruptedException<SourceException>::ReplacementInterruptedException(
		std::size_t numberOfReplacements) : numberOfReplacements_(numberOfReplacements) {}
	/**
	 * Constructor.
	 * @param message The message string passed to the @a SourceException's constructor
	 * @param numberOfReplacements The number of the replacements the object done successfully
	 */
	template<typename SourceException>
	inline ReplacementInterruptedException<SourceException>::ReplacementInterruptedException(
		const char* message, std::size_t numberOfReplacements)
		: SourceException(message), numberOfReplacements_(numberOfReplacements) {}
	/// Returns the number of the replacements the object done successfully.
	template<typename SourceException>
	inline std::size_t ReplacementInterruptedException<SourceException>::numberOfReplacements() const /*throw()*/ {return numberOfReplacements_;}
	/// Returns @c true if any pattern is set on the searcher.
	inline bool TextSearcher::hasPattern() const /*throw()*/ {
		return literalPattern_.get() != 0
#ifndef ASCENSION_NO_REGEX
			|| regexPattern_.get() != 0
#ifndef ASCENSION_NO_MIGEMO
			|| migemoPattern_.get() != 0
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
			;
	}
	/// Returns @c true if the search using regular expression is available.
	inline bool TextSearcher::isRegexAvailable() /*throw()*/ {
#ifdef ASCENSION_NO_REGEX
		return false;
#else
		return true;
#endif // ASCENSION_NO_REGEX
	}
	/// Returns the number of the stored patterns.
	inline std::size_t TextSearcher::numberOfStoredPatterns() const /*throw()*/ {return storedPatterns_.size();}
	/// Returns the number of the stored replacements.
	inline std::size_t TextSearcher::numberOfStoredReplacements() const /*throw()*/ {return storedReplacements_.size();}
	/// Returns the pattern string.
	inline const String& TextSearcher::pattern(std::size_t index /* = 0 */) const {
		if(index >= storedPatterns_.size()) throw IndexOutOfBoundsException();
		std::list<String>::const_iterator i(storedPatterns_.begin()); std::advance(i, index); return *i;}
	/// Returns the replacement string.
	inline const String& TextSearcher::replacement(std::size_t index /* = 0 */) const {
		if(index >= storedReplacements_.size()) throw IndexOutOfBoundsException();
		std::list<String>::const_iterator i(storedReplacements_.begin()); std::advance(i, index); return *i;}
	template<> inline TextSearcher& TextSearcher::setPattern<LiteralPattern>(std::auto_ptr<const LiteralPattern> pattern, bool dontRemember /* = false */) {
		if(!dontRemember && (storedPatterns_.empty() || pattern->pattern() != storedPatterns_.front()))
			pushHistory(pattern->pattern(), false);
		literalPattern_ = pattern;
#ifndef ASCENSION_NO_REGEX
		regexPattern_.reset();
		regexMatcher_.reset();
#ifndef ASCENSION_NO_MIGEMO
		migemoPattern_.reset();
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
		return *this;
	}
#ifndef ASCENSION_NO_REGEX
	template<> inline TextSearcher& TextSearcher::setPattern<regex::Pattern>(std::auto_ptr<const regex::Pattern> pattern, bool dontRemember /* = false */) {
		if(!dontRemember && (storedPatterns_.empty() || pattern->pattern() != storedPatterns_.front()))
			pushHistory(pattern->pattern(), false);
		literalPattern_.reset();
		regexPattern_ = pattern;
		regexMatcher_.reset();
#ifndef ASCENSION_NO_MIGEMO
		migemoPattern_.reset();
#endif // !ASCENSION_NO_MIGEMO
		return *this;
	}
#ifndef ASCENSION_NO_MIGEMO
	template<> inline TextSearcher& TextSearcher::setPattern<regex::MigemoPattern>(std::auto_ptr<const regex::MigemoPattern> pattern, bool dontRemember /* = false */) {
		if(!dontRemember && (storedPatterns_.empty() || pattern->pattern() != storedPatterns_.front()))
			pushHistory(pattern->pattern(), false);
		literalPattern_.reset();
		regexPattern_.reset();
		migemoPattern_ = pattern;
		regexMatcher_.reset();
		return *this;
	}
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX

	/**
	 * Sets the stored list.
	 * @param first The first string of the list
	 * @param last The end string of the list
	 * @param forReplacements Set @c true to set the replacements list
	 */
	template<typename InputIterator>
	inline void TextSearcher::setStoredStrings(InputIterator first, InputIterator last, bool forReplacements) {
		(forReplacements ? storedReplacements_ : storedPatterns_).assign(first, last);}
	/// Returns if the previous command is undoable.
	inline bool IncrementalSearcher::canUndo() const /*throw()*/ {return !operationHistory_.empty();}
	/**
	 * Returns the direction of the search.
	 * @return The direction
	 * @throw IllegalStateException The searcher is not running
	 */
	inline Direction IncrementalSearcher::direction() const {checkRunning(); return statusHistory_.top().direction;}
	/// Returns true if the search is active.
	inline bool IncrementalSearcher::isRunning() const /*throw()*/ {return !statusHistory_.empty();}
	/**
	 * Returns the matched region.
	 * @throw NotRunningException the searcher is not running
	 */
	inline const kernel::Region& IncrementalSearcher::matchedRegion() const {checkRunning(); return matchedRegion_;}
	/**
	 * Returns the current search pattern.
	 * @throw NotRunningException The searcher is not running
	 */
	inline const String& IncrementalSearcher::pattern() const {checkRunning(); return pattern_;}
	/**
	 * Returns the current search type.
	 * @throw NotRunningException The searcher is not running
	 */
	inline TextSearcher::Type IncrementalSearcher::type() const {checkRunning(); return type_;}

}} // namespace ascension.searcher

#endif // !ASCENSION_SEARCHER_HPP
