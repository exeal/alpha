/**
 * @file rules.hpp
 * @author exeal
 * @date 2004-2006 (was Lexer.h)
 * @date 2006-2011
 */

#ifndef ASCENSION_RULES_HPP
#define ASCENSION_RULES_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_REGEX
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <list>
#include <set>

namespace ascension {

	namespace detail {class HashTable;}

	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {

		/**
		 * A @c URIDetector detects and searches URI. This class conforms to the syntaxes of
		 * <a href="http://www.ietf.org/rfc/rfc3986.txt">RFC 3986</a> and
		 * <a href="http://www.ietf.org/rfc/rfc3987.txt">RFC 3987</a>.
		 */
		class URIDetector {
			ASCENSION_NONCOPYABLE_TAG(URIDetector);
		public:
			// constructors
			explicit URIDetector() /*throw()*/;
			~URIDetector() /*throw()*/;
			// parsing
			const Char* detect(const Char* first, const Char* last) const;
			bool search(const Char* first, const Char* last, Range<const Char*>& result) const;
			// attribute
			URIDetector& setValidSchemes(const std::set<String>& schemes, bool caseSensitive = false);
			URIDetector& setValidSchemes(const String& schemes, Char separator, bool caseSensitive = false);
			// singleton
			static const URIDetector& defaultGenericInstance() /*throw()*/;
			static const URIDetector& defaultIANAURIInstance() /*throw()*/;
		private:
			detail::HashTable* validSchemes_;
		};

		/**
		 * 
		 * @see TokenScanner
		 */
		class BadScannerStateException : public IllegalStateException {
		public:
			BadScannerStateException() : IllegalStateException("The scanner can't accept the requested operation in this state.") {}
		};

		/// A token is a text segment with identifier.
		struct Token : public FastArenaObject<Token> {
			typedef uint16_t Identifier;
			static const Identifier UNCALCULATED;
			Identifier id;
			kernel::Region region;
		};

		class TokenScanner;

		/**
		 * Base class for concrete rule classes.
		 * @see LexicalTokenScanner, RegionRule, NumberRule, WordRule, RegexRule
		 */
		class Rule {
			ASCENSION_UNASSIGNABLE_TAG(Rule);
		public:
			/// Destructor.
			virtual ~Rule() /*throw()*/ {}
			/**
			 * 
			 * @param scanner the scanner
			 * @param first the start of the text to parse
			 * @param last the end of the text to parse
			 * @return the found token or @c null
			 */
			virtual std::auto_ptr<Token> parse(
				const TokenScanner& scanner, const Char* first, const Char* last) const /*throw()*/ = 0;
			/// Returns the identifier of the token.
			Token::Identifier tokenID() const /*throw()*/ {return id_;}
		protected:
			explicit Rule(Token::Identifier tokenID);
		private:
			const Token::Identifier id_;
		};

		/***/
		class RegionRule : public Rule {
		public:
			RegionRule(Token::Identifier id,
				const String& startSequence, const String& endSequence,
				Char escapeCharacter = text::NONCHARACTER, bool caseSensitive = true);
			std::auto_ptr<Token> parse(const TokenScanner& scanner,
				const Char* first, const Char* last) const /*throw()*/;
		private:
			const String startSequence_, endSequence_;
			const Char escapeCharacter_;
			const bool caseSensitive_;
		};

		/// A concrete rule detects numeric tokens.
		class NumberRule : public Rule {
		public:
			explicit NumberRule(Token::Identifier id) /*throw()*/;
			std::auto_ptr<Token> parse(const TokenScanner& scanner,
				const Char* first, const Char* last) const /*throw()*/;
		};

		/// A concrete rule detects URI strings.
		class URIRule : public Rule {
		public:
			URIRule(Token::Identifier id,
				std::tr1::shared_ptr<const URIDetector> uriDetector) /*throw()*/;
			std::auto_ptr<Token> parse(const TokenScanner& scanner,
				const Char* first, const Char* last) const /*throw()*/;
		private:
			std::tr1::shared_ptr<const URIDetector> uriDetector_;
		};

		/// A concrete rule detects the registered words.
		class WordRule : protected Rule {
		public:
			WordRule(Token::Identifier id,
				const String* first, const String* last, bool caseSensitive = true);
			WordRule(Token::Identifier id,
				const StringPiece& words, Char separator, bool caseSensitive = true);
			~WordRule() /*throw()*/;
			std::auto_ptr<Token> parse(const TokenScanner& scanner,
				const Char* first, const Char* last) const /*throw()*/;
		private:
			detail::HashTable* words_;
		};

#ifndef ASCENSION_NO_REGEX
		/// A concrete rule detects tokens using regular expression match.
		class RegexRule : public Rule {
		public:
			RegexRule(Token::Identifier id, std::auto_ptr<const regex::Pattern> pattern);
			std::auto_ptr<Token> parse(const TokenScanner& scanner,
				const Char* first, const Char* last) const /*throw()*/;
		private:
			std::auto_ptr<const regex::Pattern> pattern_;
		};
#endif // !ASCENSION_NO_REGEX

		/**
		 * @c TokenScanner scans a range of document and returns the tokens it finds. To start
		 * scanning, call @c #parse method with a target document region. And then call
		 * @c #nextToken method repeatedly to get tokens. When reached the end of the scanning
		 * region, the scanning is end and @c #hasNext will return @c false.
		 */
		class TokenScanner {
		public:
			/// Destructor.
			virtual ~TokenScanner() /*throw()*/ {}
			/// Returns the identifier syntax.
			virtual const text::IdentifierSyntax& getIdentifierSyntax() const /*throw()*/ = 0;
			/// Returns the current position.
			virtual kernel::Position getPosition() const /*throw()*/ = 0;
			/// Returns @c false if the scanning is done.
			virtual bool hasNext() const /*throw()*/ = 0;
			/**
			 * Moves to the next token and returns it.
			 * @return the token or @c null if the scanning was done.
			 */
			virtual std::auto_ptr<Token> nextToken() = 0;
			/**
			 * Starts the scan with the specified range. The current position of the scanner will
			 * be the top of the specified region.
			 * @param document The document
			 * @param region The region to be scanned
			 * @throw kernel#BadRegionException @a region intersects outside of the document
			 */
			virtual void parse(const kernel::Document& document, const kernel::Region& region) = 0;
		};

		/**
		 * @c NullTokenScanner returns no tokens. @c NullTokenScanner#hasNext returns always
		 * @c false.
		 */
		class NullTokenScanner : public TokenScanner {
		public:
			const text::IdentifierSyntax& getIdentifierSyntax() const /*throw()*/;
			kernel::Position getPosition() const /*throw()*/;
			bool hasNext() const /*throw()*/;
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
		class LexicalTokenScanner : public TokenScanner {
			ASCENSION_NONCOPYABLE_TAG(LexicalTokenScanner);
		public:
			// constructors
			explicit LexicalTokenScanner(kernel::ContentType contentType) /*throw()*/;
			~LexicalTokenScanner() /*throw()*/;
			// attributes
			void addRule(std::auto_ptr<const Rule> rule);
			void addWordRule(std::auto_ptr<const WordRule> rule);
			// TokenScanner
			const text::IdentifierSyntax& getIdentifierSyntax() const /*throw()*/;
			kernel::Position getPosition() const /*throw()*/;
			bool hasNext() const /*throw()*/;
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
		 * @see LexicalPartitioner
		 */
		class TransitionRule {
			ASCENSION_UNASSIGNABLE_TAG(TransitionRule);
		public:
			virtual ~TransitionRule() /*throw()*/;
			virtual std::auto_ptr<TransitionRule> clone() const = 0;
			kernel::ContentType contentType() const /*throw()*/;
			kernel::ContentType destination() const /*throw()*/;
			virtual length_t matches(const String& line, length_t column) const = 0;
		protected:
			TransitionRule(kernel::ContentType contentType, kernel::ContentType destination) /*throw()*/;
		private:
			const kernel::ContentType contentType_, destination_;
		};

		/// Implementation of @c TransitionRule uses literal string match.
		class LiteralTransitionRule : public TransitionRule {
		public:
			LiteralTransitionRule(kernel::ContentType contentType, kernel::ContentType destination,
				const String& pattern, Char escapeCharacter = text::NONCHARACTER, bool caseSensitive = true);
			std::auto_ptr<TransitionRule> clone() const;
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
				kernel::ContentType destination, std::auto_ptr<const regex::Pattern> pattern);
			RegexTransitionRule(const RegexTransitionRule& other);
			std::auto_ptr<TransitionRule> clone() const;
			length_t matches(const String& line, length_t column) const;
		private:
			std::auto_ptr<const regex::Pattern> pattern_;
		};
#endif // !ASCENSION_NO_REGEX

		/**
		 * @c LexicalPartitioner makes document partitions by using the specified lexical rules.
		 * @note This class is not derivable.
		 * @see kernel#Document
		 */
		class LexicalPartitioner : public kernel::DocumentPartitioner {
			ASCENSION_NONCOPYABLE_TAG(LexicalPartitioner);
		public:
			// constructor
			LexicalPartitioner() /*throw()*/;
			~LexicalPartitioner() /*throw()*/;
			// attribute
			template<typename InputIterator>
			void setRules(InputIterator first, InputIterator last);
		private:
			struct Partition {
				kernel::ContentType contentType;
				kernel::Position start, tokenStart;
				length_t tokenLength;
				Partition(kernel::ContentType type, const kernel::Position& p,
					const kernel::Position& startOfToken, length_t lengthOfToken) /*throw()*/
					: contentType(type), start(p), tokenStart(startOfToken), tokenLength(lengthOfToken) {}
				kernel::Position getTokenEnd() const /*throw()*/ {return kernel::Position(tokenStart.line, tokenStart.column + tokenLength);}
			};
		private:
			void computePartitioning(const kernel::Position& start,
				const kernel::Position& minimalLast, kernel::Region& changedRegion);
			static void deleteRules(std::list<const TransitionRule*>& rules) /*throw()*/;
			void dump() const;
			void erasePartitions(const kernel::Position& first, const kernel::Position& last);
			detail::GapVector<Partition*>::const_iterator partitionAt(const kernel::Position& at) const /*throw()*/;
			kernel::ContentType transitionStateAt(const kernel::Position& at) const /*throw()*/;
			length_t tryTransition(const String& line, length_t column,
				kernel::ContentType contentType, kernel::ContentType& destination) const /*throw()*/;
			void verify() const;
			// DocumentPartitioner
			void documentAboutToBeChanged() /*throw()*/;
			void documentChanged(const kernel::DocumentChange& change) /*throw()*/;
			void doGetPartition(const kernel::Position& at, kernel::DocumentPartition& partition) const /*throw()*/;
			void doInstall() /*throw()*/;
		private:
			detail::GapVector<Partition*> partitions_;
			typedef std::list<const TransitionRule*> TransitionRules;
			TransitionRules rules_;
		};

		/**
		 * Standard implementation of @c presentation#IPartitionPresentationReconstructor. This
		 * implementation performs rule based lexical tokenization using the given @c TokenScanner.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalPartitionPresentationReconstructor : public presentation::PartitionPresentationReconstructor {
			ASCENSION_UNASSIGNABLE_TAG(LexicalPartitionPresentationReconstructor);
		public:
			explicit LexicalPartitionPresentationReconstructor(
				const presentation::Presentation& presentation, std::auto_ptr<TokenScanner> tokenScanner,
				const std::map<Token::Identifier, std::tr1::shared_ptr<const presentation::TextRunStyle> >& styles,
				std::tr1::shared_ptr<const presentation::TextRunStyle> defaultStyle = std::tr1::shared_ptr<const presentation::TextRunStyle>());
		private:
			// presentation.IPartitionPresentationReconstructor
			std::auto_ptr<presentation::StyledTextRunIterator> getPresentation(const kernel::Region& region) const /*throw()*/;
		private:
			class StyledTextRunIterator;
			const presentation::Presentation& presentation_;
			std::auto_ptr<TokenScanner> tokenScanner_;
			std::tr1::shared_ptr<const presentation::TextRunStyle> defaultStyle_;
			const std::map<Token::Identifier, std::tr1::shared_ptr<const presentation::TextRunStyle> > styles_;
		};


		/// Returns the content type.
		inline kernel::ContentType TransitionRule::contentType() const /*throw()*/ {return contentType_;}

		/// Returns the content type of the transition destination.
		inline kernel::ContentType TransitionRule::destination() const /*throw()*/ {return destination_;}

		template<typename InputIterator> inline void LexicalPartitioner::setRules(InputIterator first, InputIterator last) {
			if(document() != 0)
				throw IllegalStateException("The partitioner is already connected to document.");
			std::list<const TransitionRule*> temp;
			try {
				for(; first != last; ++first)
					temp.push_back((*first)->clone().release());
			} catch(...) {
				deleteRules(temp);
				throw;
			}
			std::swap(rules_, temp);
			deleteRules(temp);
		}

	}
} // namespace ascension.rules

#endif // !ASCENSION_RULES_HPP
