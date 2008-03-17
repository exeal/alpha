/**
 * @file rules.hpp
 * @author exeal
 * @date 2004-2006 (was Lexer.h)
 * @date 2006-2008
 */

#ifndef ASCENSION_RULES_HPP
#define ASCENSION_RULES_HPP
#include "presentation.hpp"
#include "regex.hpp"
#include <set>
#include <list>

namespace ascension {

	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {

		namespace internal {class HashTable;}

		/**
		 * A @c URIDetector detects and searches URI. This class conforms to the syntaxes of
		 * <a href="http://www.ietf.org/rfc/rfc3986.txt">RFC 3986</a> and
		 * <a href="http://www.ietf.org/rfc/rfc3987.txt">RFC 3987</a>.
		 */
		class URIDetector {
			MANAH_NONCOPYABLE_TAG(URIDetector);
		public:
			// constructors
			explicit URIDetector() throw();
			~URIDetector() throw();
			// parsing
			const Char* detect(const Char* first, const Char* last) const;
			bool search(const Char* first, const Char* last, std::pair<const Char*, const Char*>& result) const;
			// attribute
			URIDetector& setValidSchemes(const std::set<String>& schemes, bool caseSensitive = false);
			URIDetector& setValidSchemes(const String& schemes, Char separator, bool caseSensitive = false);
			// singleton
			static const URIDetector& defaultGenericInstance() throw();
			static const URIDetector& defaultIANAURIInstance() throw();
		private:
			internal::HashTable* validSchemes_;
		};

		/**
		 * 
		 * @see token#TokenScanner, partition#PartitionScanner
		 */
		class BadScannerStateException : public IllegalStateException {
		public:
			BadScannerStateException() : IllegalStateException("The scanner can't accept the requested operation in this state.") {}
		};

		/// A token is a text segment with identifier.
		struct Token : public manah::FastArenaObject<Token> {
			typedef ushort ID;
			static const ID DEFAULT_TOKEN, UNCALCULATED;
			ID id;
			kernel::Region region;
		};

		class ITokenScanner;

		/**
		 * Base class for concrete rule classes.
		 * @see LexicalTokenScanner, RegionRule, NumberRule, WordRule, RegexRule
		 */
		class Rule {
			MANAH_UNASSIGNABLE_TAG(Rule);
		public:
			/// Returns the identifier of the token.
			Token::ID getTokenID() const throw() {return id_;}
			/// Returns true if the match is case sensitive.
			bool isCaseSensitive() const throw() {return caseSensitive_;}
			/**
			 * 
			 * @param scanner the scanner
			 * @param first the start of the text to parse
			 * @param last the end of the text to parse
			 * @return the found token or @c null
			 */
			virtual std::auto_ptr<Token> parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw() = 0;
		protected:
			explicit Rule(Token::ID tokenID, bool caseSensitive) throw();
		private:
			const Token::ID id_;
			const bool caseSensitive_;
		};

		/***/
		class RegionRule : public Rule {
		public:
			RegionRule(Token::ID id, const String& startSequence,
				const String& endSequence, Char escapeCharacter = NONCHARACTER, bool caseSensitive = true);
			std::auto_ptr<Token> parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw();
		private:
			const String startSequence_, endSequence_;
			const Char escapeCharacter_;
		};

		/// A concrete rule detects numeric tokens.
		class NumberRule : public Rule {
		public:
			explicit NumberRule(Token::ID id) throw();
			std::auto_ptr<Token> parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw();
		};

		/// A concrete rule detects URI strings.
		class URIRule : public Rule {
		public:
			URIRule(Token::ID id, const URIDetector& uriDetector, bool delegateOwnership) throw();
			std::auto_ptr<Token> parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw();
		private:
			ascension::internal::StrategyPointer<const URIDetector> uriDetector_;
		};

		/// A concrete rule detects the registered words.
		class WordRule : protected Rule {
		public:
			WordRule(Token::ID id, const String* first, const String* last, bool caseSensitive = true);
			WordRule(Token::ID id, const Char* first, const Char* last, Char separator, bool caseSensitive = true);
			~WordRule() throw();
			std::auto_ptr<Token> parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw();
		private:
			internal::HashTable* words_;
		};

#ifndef ASCENSION_NO_REGEX
		/// A concrete rule detects tokens using regular expression match.
		class RegexRule : public Rule {
		public:
			RegexRule(Token::ID id, const String& pattern, bool caseSensitive = true);
			std::auto_ptr<Token> parse(const ITokenScanner& scanner, const Char* first, const Char* last) const throw();
		private:
			std::auto_ptr<const regex::Pattern> pattern_;
		};
#endif /* !ASCENSION_NO_REGEX */

		/**
		 * @c ITokenScanner scans a range of document and returns the tokens it finds. To start
		 * scanning, call @c #parse method with a target document region. And then call
		 * @c #nextToken method repeatedly to get tokens. When reached the end of the scanning
		 * region, the scanning is end and @c #isDone will return true.
		 */
		class ITokenScanner {
		public:
			/// Destructor.
			virtual ~ITokenScanner() throw() {}
			/// Returns the identifier syntax.
			virtual const text::IdentifierSyntax& getIdentifierSyntax() const throw() = 0;
			/// Returns the current position.
			virtual kernel::Position getPosition() const throw() = 0;
			/// Returns true if the scanning is done.
			virtual bool isDone() const throw() = 0;
			/**
			 * Moves to the next token and returns it.
			 * @return the token or @c null if the scanning was done.
			 */
			virtual std::auto_ptr<Token> nextToken() = 0;
			/**
			 * Starts the scan with the specified range. The current position of the scanner will
			 * be the top of the specified region.
			 * @param document the document
			 * @param region the region to be scanned
			 * @throw kernel#BadRegionException @a region intersects outside of the document
			 */
			virtual void parse(const kernel::Document& document, const kernel::Region& region) = 0;
		};

		/// @c NullTokenScanner returns no tokens. @c NullTokenScanner#isDone returns always true.
		class NullTokenScanner : virtual public ITokenScanner {
		public:
			const text::IdentifierSyntax& getIdentifierSyntax() const throw();
			kernel::Position getPosition() const throw();
			bool isDone() const throw();
			std::auto_ptr<Token> nextToken();
			void parse(const kernel::Document& document, const kernel::Region& region);
		};

		/**
		 * A generic scanner which is programable with a sequence of rules. The rules must be
		 * registered before start of scanning. Otherwise @c RunningScannerException will be thrown.
		 * Note that the tokens this scanner returns are only single-line. Multi-line tokens are
		 * not supported by this class.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalTokenScanner : virtual public ITokenScanner {
			MANAH_NONCOPYABLE_TAG(LexicalTokenScanner);
		public:
			// constructors
			explicit LexicalTokenScanner(kernel::ContentType contentType) throw();
			~LexicalTokenScanner() throw();
			// attributes
			void addRule(std::auto_ptr<const Rule> rule);
			void addWordRule(std::auto_ptr<const WordRule> rule);
			// ITokenScanner
			const text::IdentifierSyntax& getIdentifierSyntax() const throw();
			kernel::Position getPosition() const throw();
			bool isDone() const throw();
			std::auto_ptr<Token> nextToken();
			void parse(const kernel::Document& document, const kernel::Region& region);
		private:
			kernel::ContentType contentType_;
			std::list<const Rule*> rules_;
			std::list<const WordRule*> wordRules_;
			kernel::DocumentCharacterIterator current_;
		};

		/**
		 * A rule for detecting patterns which begin new partition in document.
		 * @note This class is not dervable.
		 * @see LexicalPartitioner
		 */
		class TransitionRule {
			MANAH_UNASSIGNABLE_TAG(TransitionRule);
		public:
			virtual ~TransitionRule() throw();
			kernel::ContentType contentType() const throw();
			kernel::ContentType destination() const throw();
			virtual length_t matches(const String& line, length_t column) const = 0;
		protected:
			TransitionRule(kernel::ContentType contentType, kernel::ContentType destination) throw();
		private:
			const kernel::ContentType contentType_, destination_;
		};

		/// Implementation of @c TransitionRule uses literal string match.
		class LiteralTransitionRule : public TransitionRule {
		public:
			LiteralTransitionRule(kernel::ContentType contentType, kernel::ContentType destination,
				const String& pattern, Char escapeCharacter = NONCHARACTER, bool caseSensitive = true);
			length_t matches(const String& line, length_t column) const;
		private:
			const String pattern_;
			const Char escapeCharacter_;
			const bool caseSensitive_;
		};
		
#ifndef ASCENSION_NO_REGEX
		/// Implementation of @c TransitionRule uses regular expression match.
		class RegexTransitionRule : public TransitionRule {
		public:
			RegexTransitionRule(kernel::ContentType contentType,
				kernel::ContentType destination, const String& pattern, bool caseSensitive = true);
			length_t matches(const String& line, length_t column) const;
		private:
			std::auto_ptr<const regex::Pattern> pattern_;
		};
#endif /* !ASCENSION_NO_REGEX */

		/**
		 * @c LexicalPartitioner makes document partitions by using the specified lexical rules.
		 * @note This class is not derivable.
		 * @see kernel#Document
		 */
		class LexicalPartitioner : public kernel::DocumentPartitioner {
		public:
			// constructor
			LexicalPartitioner() throw();
			~LexicalPartitioner() throw();
			// attribute
			template<typename InputIterator>
			void setRules(InputIterator first, InputIterator last);
		private:
			void clearRules() throw();
			void computePartitioning(const kernel::Position& start,
				const kernel::Position& minimalLast, kernel::Region& changedRegion);
			void dump() const;
			void erasePartitions(const kernel::Position& first, const kernel::Position& last);
			std::size_t partitionAt(const kernel::Position& at) const throw();
			kernel::ContentType transitionStateAt(const kernel::Position& at) const throw();
			length_t tryTransition(const String& line, length_t column,
				kernel::ContentType contentType, kernel::ContentType& destination) const throw();
			void verify() const;
			// DocumentPartitioner
			void documentAboutToBeChanged() throw();
			void documentChanged(const kernel::DocumentChange& change) throw();
			void doGetPartition(const kernel::Position& at, kernel::DocumentPartition& partition) const throw();
			void doInstall() throw();
		private:
			struct Partition {
				kernel::ContentType contentType;
				kernel::Position start, tokenStart;
				length_t tokenLength;
				Partition(kernel::ContentType type, const kernel::Position& p, const kernel::Position& startOfToken,
					length_t lengthOfToken) throw() : contentType(type), start(p), tokenStart(startOfToken), tokenLength(lengthOfToken) {}
				kernel::Position getTokenEnd() const throw() {return kernel::Position(tokenStart.line, tokenStart.column + tokenLength);}
			};
			const kernel::Position& getPartitionStart(size_t partition) const throw() {return partitions_[partition]->start;}
			manah::GapBuffer<Partition*, manah::GapBuffer_DeletePointer<Partition*> > partitions_;
			typedef std::list<const TransitionRule*> TransitionRules;
			TransitionRules rules_;
		};

		/**
		 * Standard implementation of @c presentation#IPartitionPresentationReconstructor. This
		 * implementation performs rule based lexical tokenization using the given @c TokenScanner.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalPartitionPresentationReconstructor : virtual public presentation::IPartitionPresentationReconstructor {
			MANAH_UNASSIGNABLE_TAG(LexicalPartitionPresentationReconstructor);
		public:
			explicit LexicalPartitionPresentationReconstructor(const kernel::Document& document,
				std::auto_ptr<ITokenScanner> tokenScanner, const std::map<Token::ID, const presentation::TextStyle>& styles);
		private:
			// IPartitionPresentationReconstructor
			std::auto_ptr<presentation::LineStyle> getPresentation(const kernel::Region& region) const throw();
		private:
			const kernel::Document& document_;
			std::auto_ptr<ITokenScanner> tokenScanner_;
			const std::map<Token::ID, const presentation::TextStyle> styles_;
		};


		/// Returns the content type.
		inline kernel::ContentType TransitionRule::contentType() const throw() {return contentType_;}

		/// Returns the content type of the transition destination.
		inline kernel::ContentType TransitionRule::destination() const throw() {return destination_;}

		template<typename InputIterator> inline void LexicalPartitioner::setRules(InputIterator first, InputIterator last) {
			if(document() != 0)
				throw IllegalStateException("The partitioner is already connected to document.");
			clearRules();
			std::copy(first, last, std::back_inserter(rules_));
		}

}} // namespace ascension.rules

#endif /* !ASCENSION_RULES_HPP */
