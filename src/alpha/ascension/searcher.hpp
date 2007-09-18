/**
 * @file searcher.hpp
 * @author exeal
 * @date 2004-2006 (was TextSearcher.h)
 * @date 2006-2007
 */

#ifndef ASCENSION_SEARCHER_HPP
#define ASCENSION_SEARCHER_HPP
#include "document.hpp"
#ifndef ASCENSION_NO_REGEX
#include "regex.hpp"
#endif /* !ASCENSION_NO_REGEX */
#include <list>


namespace ascension {

	namespace unicode {class BreakIterator;}

#ifndef ASCENSION_NO_REGEX
	namespace regex {template<typename CodePointIterator> class Matcher;};
#endif /* !ASCENSION_NO_REGEX */

	namespace searcher {

		/**
		 * @note This class is not derivable.
		 */
		class LiteralPattern {
		public:
			// constructor
			LiteralPattern(const Char* first, const Char* last,
				Direction direction, bool ignoreCase = false, const unicode::Collator* collator = 0);
			LiteralPattern(const String& pattern, Direction direction, bool ignoreCase = false, const unicode::Collator* collator = 0);
			~LiteralPattern() throw();
			// attributes
			Direction	getDirection() const throw();
			bool		isCaseSensitive() const throw();
			// operation
			void	compile(const Char* first, const Char* last,
						Direction direction, bool ignoreCase = false, const unicode::Collator* collator = 0);
			void	compile(const String& pattern, Direction direction, bool ignoreCase = false, const unicode::Collator* collator = 0);
			bool	matches(const unicode::CharacterIterator& target) const;
			bool	search(const unicode::CharacterIterator& target,
						std::auto_ptr<unicode::CharacterIterator>& matchedFirst,
						std::auto_ptr<unicode::CharacterIterator>& matchedLast) const;
		private:
			Direction direction_;
			bool caseSensitive_;
#ifndef ASCENSION_NO_UNICODE_COLLATION
			std::auto_ptr<unicode::Collator> collator_;
#endif /* !ASCENSION_NO_UNICODE_COLLATION */
			std::ptrdiff_t lastOccurences_[65536];
			int* first_;	// collation elements of the pattern
			int* last_;
		};

		/// Types of search.
		enum SearchType {
			LITERAL,			///< Literal search.
#ifndef ASCENSION_NO_REGEX
			REGULAR_EXPRESSION,	///< Regular expression search.
#ifndef ASCENSION_NO_MIGEMO
			MIGEMO				///< Migemo.
#endif /* !ASCENSION_NO_REGEX */
#endif /* !ASCENSION_NO_MIGEMO */
		};

		/// Options for search.
		struct SearchOptions {
			/// Constraint the edges of the matched region must satisfy.
			enum WholeMatch {
				NONE,				///< No constraint.
				GRAPHEME_CLUSTER,	///< The start and the end of the match region must be grapheme cluster boundaries.
				WORD				///< The start and the end of the match region must be word boundaries (called whole word match).
			};
			SearchType type;			///< Type the of search.
			bool caseSensitive;			///< For caseless match.
			bool canonicalEquivalents;	///< Set true to enable canonical equivalents (not implemented).
#ifndef ASCENSION_NO_UNICODE_COLLATION
			int collationWeight;		///< Collation weight level (not implemented).
#endif /* !ASCENSION_NO_UNICODE_COLLATION */
			WholeMatch wholeMatch;		///< Whole match constraint.
			bool wrapAround;			///< Wrap around when the scanning was reached the end/start of the target region.
			/// Constructor.
			SearchOptions() throw() : type(LITERAL), caseSensitive(true), canonicalEquivalents(false), wholeMatch(NONE) {}
			/// Equality operator.
			bool operator==(const SearchOptions& rhs) const throw() {
				return type == rhs.type
					&& caseSensitive == rhs.caseSensitive
					&& canonicalEquivalents == rhs.canonicalEquivalents
#ifndef ASCENSION_NO_UNICODE_COLLATION
					&& collationWeight == rhs.collationWeight
#endif /* !ASCENSION_NO_UNICODE_COLLATION */
					&& wholeMatch == rhs.wholeMatch
					&& wrapAround == rhs.wrapAround;
			}
			/// Unequality operator.
			bool operator!=(const SearchOptions& rhs) const throw() {return !operator==(rhs);}
		};

		/**
		 * A callback defines reactions about the interactive replacement.
		 * @see TextSearcher#replaceAll
		 */
		class IInteractiveReplacementCallback {
		protected:
			/// Reactions for @c #queryReplacementAction method.
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
			 * @param matchedRegion the region matched the pattern
			 * @param canUndo this call can return the value @c UNDO as an action
			 * @return the action the text searcher should perform
			 */
			virtual Action queryReplacementAction(const text::Region& matchedRegion, bool canUndo) = 0;
			/**
			 * The replacement exited or explicitly aborted.
			 * @param numberOfMatched
			 * @param numberOfReplacements
			 */
			virtual void replacementEnded(std::size_t numberOfMatches, std::size_t numberOfReplacements) = 0;
			/**
			 * The replacement started.
			 * @param document the document to search and replace
			 * @param scope the region to perform
			 */
			virtual void replacementStarted(const text::Document& document, const text::Region& scope) = 0;
			friend class TextSearcher;
		};

		/**
		 * Searches the specified pattern in the document.
		 *
		 * A session holds an instance of this class, while client can create instances.
		 *
		 * @c TextSearcher has the list of patterns used for search. The pattern which is given by
		 * #setPattern method is pushed into this list, and the client can reuse those patterns
		 * later. @c IncrementalSearcher uses this list to get the pattern used previously. To get
		 * this stored patterns, call #getPattern method. To get the length of the list, call
		 * #getNumberOfStoredPatterns method. The maximum length of the list can be changed by
		 * calling #setMaximumNumberOfStoredStrings method. Default length is 16 and the minimum
		 * is 4.
		 * @see texteditor#Session#getTextSearcher, texteditor#commands#FindAllCommand,
		 * texteditor#commands#FindNextCommand
		 */
		class TextSearcher : public manah::Noncopyable {
		public:
			// constructors.
			TextSearcher();
			virtual ~TextSearcher();
			// attributes
			std::size_t				getNumberOfStoredPatterns() const throw();
			std::size_t				getNumberOfStoredReplacements() const throw();
			const SearchOptions&	getOptions() const throw();
			const String&			getPattern(std::size_t index = 0) const;
			const String&			getReplacement(std::size_t index = 0) const;
			bool					isLastPatternMatched() const throw();
			bool					isMigemoAvailable() const throw();
			static bool				isRegexAvailable() throw();
			void					setMaximumNumberOfStoredStrings(std::size_t number) throw();
			void					setOptions(const SearchOptions& options) throw();
			void					setPattern(const String& pattern, bool dontRemember = false);
			void					setReplacement(const String& replacement);
			// operations
			void		abortInteractiveReplacement();
			std::size_t	replaceAll(text::Document& document,
							const text::Region& scope, IInteractiveReplacementCallback* callback) const;
			bool		search(const text::Document& document, const text::Position& from,
							const text::Region& scope, Direction direction, text::Region& matchedRegion) const;
			template<typename InputIterator>
			void		setStoredStrings(InputIterator first, InputIterator last, bool forReplacements);
		private:
			bool	checkBoundary(const text::DocumentCharacterIterator& first, const text::DocumentCharacterIterator& last) const;
			void	clearPatternCache();
			void	compilePattern(Direction direction) const;
			void	pushHistory(const String& s, bool forReplacements);
		private:
			std::auto_ptr<LiteralPattern> literalPattern_;
#ifndef ASCENSION_NO_REGEX
			std::auto_ptr<const regex::Pattern> regexPattern_;
			std::auto_ptr<regex::Matcher<text::DocumentCharacterIterator> > regexMatcher_;
#endif /* !ASCENSION_NO_REGEX */
			mutable struct LastResult {
				const text::Document* document;
				text::Region matchedRegion;
				Direction direction;
				ulong documentRevisionNumber;
				LastResult() throw() : document(0), matchedRegion(text::Position::INVALID_POSITION) {}
				~LastResult() throw() {reset();}
				bool checkDocumentRevision(const text::Document& current) const throw() {
					return document == &current && documentRevisionNumber == current.getRevisionNumber();}
				bool matched() const throw() {return matchedRegion.first != text::Position::INVALID_POSITION;}
				void reset() throw() {matchedRegion.first = matchedRegion.second = text::Position::INVALID_POSITION;}
				void updateDocumentRevision(const text::Document& document) throw() {
					this->document = &document; documentRevisionNumber = document.getRevisionNumber();}
			} lastResult_;
			SearchOptions options_;
			String temporaryPattern_;
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
			 * @param initialPosition the position at which the search started.
			 */
			virtual void incrementalSearchAborted(const text::Position& initialPosition) = 0;
			/// The search was completed successfully.
			virtual void incrementalSearchCompleted() = 0;
			/**
			 * The search pattern was changed.
			 * @param result the result on new pattern.
			 */
			virtual void incrementalSearchPatternChanged(Result result, const manah::Flags<WrappingStatus>& wrappingStatus) = 0;
			/**
			 * The search was started. @c incrementalSearchPatternChanged is also called with
			 * @c EMPTY_PATTERN after this.
			 * @param document the document to search
			 */
			virtual void incrementalSearchStarted(const text::Document& document) = 0;
			friend class IncrementalSearcher;
		};


		/**
		 * @c IncrementalSearcher performs incremental search on a document. A session holds an
		 * instance of this class, while client can create instances.
		 * The search will abort automatically when the document was changed.
		 * @see TextSearcher, texteditor#Session#getIncrementalSearcher,
		 * texteditor#commands#IncrementalSearchCommand
		 */
		class IncrementalSearcher : virtual public text::IDocumentListener, public manah::Noncopyable {
		public:
			// constructor
			IncrementalSearcher() throw();
			// attributes
			bool				canUndo() const throw();
			Direction			getDirection() const;
			const text::Region&	getMatchedRegion() const;
			const String&		getPattern() const;
			bool				isRunning() const throw();
			// operations
			void	abort();
			bool	addCharacter(Char c);
			bool	addCharacter(CodePoint c);
			bool	addString(const Char* first, const Char* last);
			bool	addString(const String& text);
			void	end();
			bool	next(Direction direction);
			void	reset();
			void	start(text::Document& document, const text::Position& from,
						TextSearcher& searcher, Direction direction, IIncrementalSearchCallback* callback = 0);
			bool	undo();
		private:
			bool	update();
			// text.IDocumentListener
			void	documentAboutToBeChanged(const text::Document& document);
			void	documentChanged(const text::Document& document, const text::DocumentChange& change);

		private:
			void checkRunning() const;
			enum Operation {TYPE, JUMP};
			struct Status {
				text::Region matchedRegion;
				Direction direction;
			};
			text::Document* document_;
			TextSearcher* searcher_;
			IIncrementalSearchCallback* callback_;
			text::Region matchedRegion_;
			std::stack<Operation> operationHistory_;
			std::stack<Status> statusHistory_;
			String pattern_;
			bool matched_;	// true if matched in the last update() call
		};


// inline ///////////////////////////////////////////////////////////////////

	/**
	 * Compiles the pattern.
	 * @param pattern the search pattern
	 * @param direction the direction to search
	 * @param ignoreCase set true to perform case-insensitive search
	 * @param collator the collator or @c null if not needed
	 * @throw std#invalid_argument @a first and/or @a last are invalid
	 */
	inline void LiteralPattern::compile(const String& pattern, Direction direction, bool ignoreCase /* = false */,
		const unicode::Collator* collator /* = 0 */) {compile(pattern.data(), pattern.data() + pattern.length(), direction, ignoreCase, collator);}
	/// Returns the direction to search.
	inline Direction LiteralPattern::getDirection() const throw() {return direction_;}
	/// Returns true if the pattern performs case-sensitive match.
	inline bool LiteralPattern::isCaseSensitive() const throw() {return caseSensitive_;}
	/// Returns the number of the stored patterns.
	inline std::size_t TextSearcher::getNumberOfStoredPatterns() const throw() {return storedPatterns_.size();}
	/// Returns the number of the stored replacements.
	inline std::size_t TextSearcher::getNumberOfStoredReplacements() const throw() {return storedReplacements_.size();}
	/// Returns the search options.
	inline const SearchOptions& TextSearcher::getOptions() const throw() {return options_;}
	/// Returns the pattern string.
	inline const String& TextSearcher::getPattern(std::size_t index /* = 0 */) const {
		if(index >= storedPatterns_.size()) throw IndexOutOfBoundsException();
		std::list<String>::const_iterator i(storedPatterns_.begin()); std::advance(i, index); return *i;}
	/// Returns the replacement string.
	inline const String& TextSearcher::getReplacement(std::size_t index /* = 0 */) const {
		if(index >= storedReplacements_.size()) throw IndexOutOfBoundsException();
		std::list<String>::const_iterator i(storedReplacements_.begin()); std::advance(i, index); return *i;}
	/// Returns true if the search using regular expression is available.
	inline bool TextSearcher::isRegexAvailable() throw() {
#ifdef ASCENSION_NO_REGEX
		return false;
#else
		return true;
#endif /* ASCENSION_NO_REGEX */
	}
	/**
	 * Sets the stored list.
	 * @param first the first string of the list
	 * @param last the end string of the list
	 * @param forReplacements set true to set the replacements list
	 */
	template<typename InputIterator>
	inline void TextSearcher::setStoredStrings(InputIterator first, InputIterator last, bool forReplacements) {
		(forReplacements ? storedReplacements_ : storedPatterns_).assign(first, last);}
	/**
	 * Appends the specified text to the end of the current pattern.
	 * @param text the string to append
	 * @return true if the pattern is found
	 * @throw NotRunningException the searcher is not running
	 * @throw std#invalid_argument the string is empty
	 * @throw ... any exceptions specified by Boost.Regex will be thrown if the regular expression error occured
	 */
	inline bool IncrementalSearcher::addString(const String& text) {return addString(text.data(), text.data() + text.length());}
	/// Returns if the previous command is undoable.
	inline bool IncrementalSearcher::canUndo() const throw() {return !operationHistory_.empty();}
	/**
	 * Returns the direction of the search.
	 * @return the direction
	 * @throw IllegalStateException the searcher is not running
	 */
	inline Direction IncrementalSearcher::getDirection() const {checkRunning(); return statusHistory_.top().direction;}
	/**
	 * Returns the current search pattern.
	 * @throw NotRunningException the searcher is not running
	 */
	inline const String& IncrementalSearcher::getPattern() const {checkRunning(); return pattern_;}
	/// Returns true if the search is active.
	inline bool IncrementalSearcher::isRunning() const throw() {return !statusHistory_.empty();}
	/**
	 * Returns the matched region.
	 * @throw NotRunningException the searcher is not running
	 */
	inline const text::Region& IncrementalSearcher::getMatchedRegion() const {checkRunning(); return matchedRegion_;}

}} // namespace ascension.searcher

#endif /* !ASCENSION_SEARCHER_HPP */
