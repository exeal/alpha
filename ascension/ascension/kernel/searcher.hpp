/**
 * @file searcher.hpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.h)
 * @date 2006-2014
 */

#ifndef ASCENSION_SEARCHER_HPP
#define ASCENSION_SEARCHER_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_UNICODE_COLLATION, ASCENSION_NO_REGEX, ...
#include <ascension/direction.hpp>
#ifndef ASCENSION_NO_UNICODE_COLLATION
#	include <ascension/corelib/text/collator.hpp>
#endif // !ASCENSION_NO_UNICODE_COLLATION
#include <ascension/kernel/bookmarker.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#ifndef ASCENSION_NO_REGEX
#	include <ascension/corelib/regex.hpp>
#endif // !ASCENSION_NO_REGEX
#include <ascension/kernel/document-observers.hpp>
#include <array>
#include <list>
#include <stack>
#include <vector>

namespace ascension {
	namespace text {
		class BreakIterator;
	}

#ifndef ASCENSION_NO_REGEX
	namespace regex {
		template<typename CodePointIterator> class Matcher;
	};
#endif // !ASCENSION_NO_REGEX

	namespace searcher {
		/**
		 * @note This class is not intended to be subclassed.
		 */
		class LiteralPattern : private boost::noncopyable {
		public:
			LiteralPattern(const String& pattern, bool caseSensitive = true
#ifndef ASCENSION_NO_UNICODE_COLLATION
				, std::unique_ptr<const text::Collator> collator = std::unique_ptr<const text::Collator>()
#endif // !ASCENSION_NO_UNICODE_COLLATION
);
			/// @name Attributes
			/// @{
			bool isCaseSensitive() const BOOST_NOEXCEPT;
			const String& pattern() const BOOST_NOEXCEPT;
			/// @}

			/// @name Operations
			/// @{
			template<typename CharacterIterator>
			bool matches(const CharacterIterator& target) const;
			template<typename CharacterIterator>
			bool search(const CharacterIterator& target, Direction direction,
				CharacterIterator& matchedFirst, CharacterIterator& matchedLast) const;
			/// @}

		private:
			void makeShiftTable(Direction direction) BOOST_NOEXCEPT;
			bool matches(const text::detail::CharacterIterator& target) const;
			bool search(const text::detail::CharacterIterator& target, Direction direction,
				text::detail::CharacterIterator& matchedFirst, text::detail::CharacterIterator& matchedLast) const;
		private:
			const String pattern_;
			const bool caseSensitive_;
#ifndef ASCENSION_NO_UNICODE_COLLATION
			const std::unique_ptr<const text::Collator> collator_;
#endif // !ASCENSION_NO_UNICODE_COLLATION
			std::array<std::ptrdiff_t, 65536> lastOccurences_;	// for forward search
			std::array<std::ptrdiff_t, 65536> firstOccurences_;	// for backward search
			std::vector<int> collationElements_;	// collation elements of the pattern
		};

		/**
		 * A callback defines reactions about the interactive replacement.
		 * @see TextSearcher#replaceAll
		 */
		class InteractiveReplacementCallback {
		protected:
			/// Reactions for @c InteractiveReplacementCallback#queryReplacementAction method.
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
			 * @param numberOfMatches
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
			/**
			 * Constructor.
			 * @param numberOfReplacements The number of the replacements the object done successfully
			 */
			explicit ReplacementInterruptedException(std::size_t numberOfReplacements)
				: numberOfReplacements_(numberOfReplacements) {}
			/**
			 * Constructor.
			 * @param message The message string passed to the @a SourceException's constructor
			 * @param numberOfReplacements The number of the replacements the object done successfully
			 */
			ReplacementInterruptedException(const char* message, std::size_t numberOfReplacements)
				: SourceException(message), numberOfReplacements_(numberOfReplacements) {}
			/// Returns the number of the replacements the object done successfully.
			std::size_t numberOfReplacements() const BOOST_NOEXCEPT {return numberOfReplacements_;}
		private:
			const std::size_t numberOfReplacements_;
		};

		// the documentation is searcher.cpp
		class TextSearcher : private boost::noncopyable {
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
			TextSearcher();

			/// @name Pattern and Replacement
			/// @{
			bool hasPattern() const BOOST_NOEXCEPT;
			std::size_t numberOfStoredPatterns() const BOOST_NOEXCEPT;
			std::size_t numberOfStoredReplacements() const BOOST_NOEXCEPT;
			const String& pattern(std::size_t index = 0) const;
			const String& replacement(std::size_t index = 0) const;
			void setMaximumNumberOfStoredStrings(std::size_t number) BOOST_NOEXCEPT;
			template<typename PatternType>
			TextSearcher& setPattern(std::unique_ptr<const PatternType> pattern, bool dontRemember = false);
			/// @}

			/// @a name Search Conditions
			/// @{
			int collationWeight() const BOOST_NOEXCEPT;
			bool isCaseSensitive() const BOOST_NOEXCEPT;
			TextSearcher& setWholeMatch(WholeMatch newValue);
			Type type() const BOOST_NOEXCEPT;
			bool usesCanonicalEquivalents() const BOOST_NOEXCEPT;
			WholeMatch wholeMatch() const BOOST_NOEXCEPT;
//			TextSearcher& wrapAround(bool wrap) BOOST_NOEXCEPT;
//			bool wrapsAround() const BOOST_NOEXCEPT;
			/// @}

			/// @name Search Result
			/// @{
			bool isLastPatternMatched() const BOOST_NOEXCEPT;
			/// @}

			/// @name Services
			/// @{
			bool isMigemoAvailable() const BOOST_NOEXCEPT;
			static bool isRegexAvailable() BOOST_NOEXCEPT;
			/// @}

			/// @name Operations
			/// @{
			void abortInteractiveReplacement();
			std::size_t replaceAll(
				kernel::Document& document, const kernel::Region& scope,
				const String& replacement, InteractiveReplacementCallback* callback);
			bool search(const kernel::Document& document, const kernel::Position& from,
				const kernel::Region& scope, Direction direction, kernel::Region& matchedRegion) const;
			template<typename InputIterator>
			void setStoredStrings(InputIterator first, InputIterator last, bool forReplacements);
			/// @}

		private:
			void pushHistory(const String& s, bool forReplacements);
		private:
			std::unique_ptr<const LiteralPattern> literalPattern_;
#ifndef ASCENSION_NO_REGEX
			std::unique_ptr<const regex::Pattern> regexPattern_;
			std::unique_ptr<const regex::MigemoPattern> migemoPattern_;
			std::unique_ptr<regex::Matcher<kernel::DocumentCharacterIterator>> regexMatcher_;
#endif // !ASCENSION_NO_REGEX
			mutable struct LastResult {
				const kernel::Document* document;
				boost::optional<kernel::Region> matchedRegion;
				Direction direction;
				std::size_t documentRevisionNumber;
				LastResult() BOOST_NOEXCEPT : document(nullptr), direction(Direction::FORWARD) {}
				~LastResult() BOOST_NOEXCEPT {reset();}
				bool checkDocumentRevision(const kernel::Document& current) const BOOST_NOEXCEPT;
				bool matched() const BOOST_NOEXCEPT {return matchedRegion;}
				void reset() BOOST_NOEXCEPT {matchedRegion = boost::none;}
				void updateDocumentRevision(const kernel::Document& document) BOOST_NOEXCEPT;
			} lastResult_;
			Type searchType_;
			WholeMatch wholeMatch_;
//			bool wrapsAround_;
			std::list<String> storedPatterns_, storedReplacements_;
			std::size_t maximumNumberOfStoredStrings_;
			bool abortedInteractiveReplacement_;
			static const std::size_t DEFAULT_NUMBER_OF_STORED_STRINGS, MINIMUM_NUMBER_OF_STORED_STRINGS;
		};

		/**
		 * Represents a callback object observes the state of the incremental searcher.
		 * @see IncrementalSearcher
		 */
		class IncrementalSearchCallback {
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
			 * @param result The result on new pattern. See @c #WrappingStatus.
			 * @param wrappingStatus
			 */
			virtual void incrementalSearchPatternChanged(Result result, int wrappingStatus) = 0;
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
		class IncrementalSearcher : public kernel::DocumentListener,
			public kernel::BookmarkListener, private boost::noncopyable {
		public:
			IncrementalSearcher() BOOST_NOEXCEPT;

			/// @name Attributes
			/// @{
			bool canUndo() const BOOST_NOEXCEPT;
			Direction direction() const;
			bool isRunning() const BOOST_NOEXCEPT;
			const kernel::Region& matchedRegion() const;
			const String& pattern() const;
			TextSearcher::Type type() const;
			/// @}

			/// @name Operations
			/// @{
			void abort();
			bool addCharacter(Char c);
			bool addCharacter(CodePoint c);
			bool addString(const StringPiece& text);
			void end();
			bool next(Direction direction);
			void reset();
			void start(kernel::Document& document, const kernel::Position& from,
				TextSearcher& searcher, TextSearcher::Type type, Direction direction,
				IncrementalSearchCallback* callback = nullptr);
			bool undo();
			/// @}
		private:
			void setPatternToSearcher(bool pushToHistory);
			bool update();
			// kernel.DocumentListener
			void documentAboutToBeChanged(const kernel::Document& document);
			void documentChanged(const kernel::Document& document, const kernel::DocumentChange& change);
			// kernel.BookmarkListener
			void bookmarkChanged(Index line);
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
			IncrementalSearchCallback* callback_;
			kernel::Region matchedRegion_;
			std::stack<Operation> operationHistory_;
			std::stack<Status> statusHistory_;
			String pattern_;
			TextSearcher::Type type_;
			bool matched_;	// true if matched in the last update() call
		};


		// inline implementation //////////////////////////////////////////////////////////////////////////////////////

		/// Returns @c true if the pattern performs case-sensitive match.
		inline bool LiteralPattern::isCaseSensitive() const BOOST_NOEXCEPT {return caseSensitive_;}

		/**
		 * Returns @c true if the pattern matches the specified character sequence.
		 * @tparam CharacterIterator The type of @a target, which satisfies @c text#detail#CharacterIteratorConcepts
		 * @param target The character sequence to match
		 * @return true if matched
		 */
		template<typename CharacterIterator>
		inline bool LiteralPattern::matches(const CharacterIterator& target) const {
			return matches(text::detail::CharacterIterator(target));
		}

		/// Returns the pattern string.
		inline const String& LiteralPattern::pattern() const BOOST_NOEXCEPT {return pattern_;}

		/**
		 * Searches in the specified character sequence.
		 * @tparam CharacterIterator The type of @a target, which satisfies @c text#detail#CharacterIteratorConcepts
		 * @param target The target character sequence
		 * @param direction The direction to search. If this is @c Direction#FORWARD, the method finds the first
		 *                  occurence of the pattern in @a target. Otherwise finds the last one
		 * @param[out] matchedFirst 
		 * @param[out] matchedLast 
		 * @return true if the pattern was found
		 */
		template<typename CharacterIterator>
		inline bool LiteralPattern::search(const CharacterIterator& target, Direction direction,
				CharacterIterator& matchedFirst, CharacterIterator& matchedLast) const {
			text::detail::CharacterIterator b, e;
			const bool found = search(text::detail::CharacterIterator(target), direction, b, e);
			matchedFirst = boost::type_erasure::any_cast<CharacterIterator>(b);
			matchedLast = boost::type_erasure::any_cast<CharacterIterator>(e);
			return found;
		}

		/// Returns @c true if any pattern is set on the searcher.
		inline bool TextSearcher::hasPattern() const BOOST_NOEXCEPT {
			return literalPattern_.get() != nullptr
#ifndef ASCENSION_NO_REGEX
				|| regexPattern_.get() != nullptr
#ifndef ASCENSION_NO_MIGEMO
				|| migemoPattern_.get() != nullptr
#endif // !ASCENSION_NO_MIGEMO
#endif // !ASCENSION_NO_REGEX
				;
		}

		/// Returns @c true if the search using regular expression is available.
		inline bool TextSearcher::isRegexAvailable() BOOST_NOEXCEPT {
#ifdef ASCENSION_NO_REGEX
			return false;
#else
			return true;
#endif // ASCENSION_NO_REGEX
		}

		/// Returns the number of the stored patterns.
		inline std::size_t TextSearcher::numberOfStoredPatterns() const BOOST_NOEXCEPT {
			return storedPatterns_.size();
		}

		/// Returns the number of the stored replacements.
		inline std::size_t TextSearcher::numberOfStoredReplacements() const BOOST_NOEXCEPT {
			return storedReplacements_.size();
		}
	
		/// Returns the pattern string.
		inline const String& TextSearcher::pattern(std::size_t index /* = nullptr */) const {
			if(index >= storedPatterns_.size())
				throw IndexOutOfBoundsException();
			std::list<String>::const_iterator i(storedPatterns_.begin());
			std::advance(i, index);
			return *i;
		}

		/// Returns the replacement string.
		inline const String& TextSearcher::replacement(std::size_t index /* = nullptr */) const {
			if(index >= storedReplacements_.size())
				throw IndexOutOfBoundsException();
			std::list<String>::const_iterator i(storedReplacements_.begin());
			std::advance(i, index);
			return *i;
		}

		template<> inline TextSearcher& TextSearcher::setPattern<LiteralPattern>(
				std::unique_ptr<const LiteralPattern> pattern, bool dontRemember /* = false */) {
			if(!dontRemember && (storedPatterns_.empty() || pattern->pattern() != storedPatterns_.front()))
				pushHistory(pattern->pattern(), false);
			literalPattern_ = move(pattern);
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
		template<> inline TextSearcher& TextSearcher::setPattern<regex::Pattern>(
				std::unique_ptr<const regex::Pattern> pattern, bool dontRemember /* = false */) {
			if(!dontRemember && (storedPatterns_.empty() || pattern->pattern() != storedPatterns_.front()))
				pushHistory(pattern->pattern(), false);
			literalPattern_.reset();
			regexPattern_ = move(pattern);
			regexMatcher_.reset();
#ifndef ASCENSION_NO_MIGEMO
			migemoPattern_.reset();
#endif // !ASCENSION_NO_MIGEMO
			return *this;
		}

#ifndef ASCENSION_NO_MIGEMO
		template<> inline TextSearcher& TextSearcher::setPattern<regex::MigemoPattern>(
				std::unique_ptr<const regex::MigemoPattern> pattern, bool dontRemember /* = false */) {
			if(!dontRemember && (storedPatterns_.empty() || pattern->pattern() != storedPatterns_.front()))
				pushHistory(pattern->pattern(), false);
			literalPattern_.reset();
			regexPattern_.reset();
			migemoPattern_ = move(pattern);
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
			(forReplacements ? storedReplacements_ : storedPatterns_).assign(first, last);
		}

		/// Returns if the previous command is undoable.
		inline bool IncrementalSearcher::canUndo() const BOOST_NOEXCEPT {
			return !operationHistory_.empty();
		}

		/**
		 * Returns the direction of the search.
		 * @return The direction
		 * @throw IllegalStateException The searcher is not running
		 */
		inline Direction IncrementalSearcher::direction() const {
			checkRunning();
			return statusHistory_.top().direction;
		}

		/// Returns true if the search is active.
		inline bool IncrementalSearcher::isRunning() const BOOST_NOEXCEPT {
			return !statusHistory_.empty();
		}

		/**
		 * Returns the matched region.
		 * @throw NotRunningException the searcher is not running
		 */
		inline const kernel::Region& IncrementalSearcher::matchedRegion() const {
			checkRunning();
			return matchedRegion_;
		}

		/**
		 * Returns the current search pattern.
		 * @throw NotRunningException The searcher is not running
		 */
		inline const String& IncrementalSearcher::pattern() const {
			checkRunning();
			return pattern_;
		}

		/**
		 * Returns the current search type.
		 * @throw NotRunningException The searcher is not running
		 */
		inline TextSearcher::Type IncrementalSearcher::type() const {
			checkRunning();
			return type_;
		}
	}
} // namespace ascension.searcher

#endif // !ASCENSION_SEARCHER_HPP
