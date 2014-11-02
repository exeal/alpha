/**
 * @file rules.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014
 */

#ifndef ASCENSION_RULES_HPP
#define ASCENSION_RULES_HPP

#include <ascension/config.hpp>	// ASCENSION_NO_REGEX
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/kernel/document-character-iterator.hpp>
#include <forward_list>
#include <set>
#include <boost/range/algorithm/for_each.hpp>

namespace ascension {
	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {
		namespace detail {
			class HashTable;
		}

		/**
		 * A @c URIDetector detects and searches URI. This class conforms to the syntaxes of
		 * <a href="http://www.ietf.org/rfc/rfc3986.txt">RFC 3986</a> and
		 * <a href="http://www.ietf.org/rfc/rfc3987.txt">RFC 3987</a>.
		 */
		class URIDetector : private boost::noncopyable {
		public:
			explicit URIDetector() BOOST_NOEXCEPT;
			~URIDetector() BOOST_NOEXCEPT;

			/// @name Parsing
			/// @{
			StringPiece::const_iterator detect(const StringPiece& text) const;
			StringPiece search(const StringPiece& text) const;
			/// @}

			/// @name Attribute
			/// @{
			URIDetector& setValidSchemes(const std::set<String>& schemes, bool caseSensitive = false);
			URIDetector& setValidSchemes(const String& schemes, Char separator, bool caseSensitive = false);
			/// @}

			/// @name Default Instances
			/// @{
			static const URIDetector& defaultGenericInstance() BOOST_NOEXCEPT;
			static const URIDetector& defaultIANAURIInstance() BOOST_NOEXCEPT;
			/// @}

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
			typedef std::uint16_t Identifier;
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
		public:
			/// Destructor.
			virtual ~Rule() BOOST_NOEXCEPT {}
			/**
			 * 
			 * @param scanner The scanner
			 * @param text The text to parse
			 * @return The found token or @c null
			 */
			virtual std::unique_ptr<Token> parse(
				const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT = 0;
			/// Returns the identifier of the token.
			Token::Identifier tokenID() const BOOST_NOEXCEPT {return id_;}
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
			std::unique_ptr<Token> parse(
				const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT;
		private:
			const String startSequence_, endSequence_;
			const Char escapeCharacter_;
			const bool caseSensitive_;
		};

		/// A concrete rule detects numeric tokens.
		class NumberRule : public Rule {
		public:
			explicit NumberRule(Token::Identifier id) BOOST_NOEXCEPT;
			std::unique_ptr<Token> parse(
				const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT;
		};

		/// A concrete rule detects URI strings.
		class URIRule : public Rule {
		public:
			URIRule(Token::Identifier id,
				std::shared_ptr<const URIDetector> uriDetector) BOOST_NOEXCEPT;
			std::unique_ptr<Token> parse(
				const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT;
		private:
			std::shared_ptr<const URIDetector> uriDetector_;
		};

		/// A concrete rule detects the registered words.
		class WordRule : protected Rule {
		public:
			WordRule(Token::Identifier id,
				const String* first, const String* last, bool caseSensitive = true);
			WordRule(Token::Identifier id,
				const StringPiece& words, Char separator, bool caseSensitive = true);
			std::unique_ptr<Token> parse(
				const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT;
		private:
			std::unique_ptr<detail::HashTable> words_;
		};

#ifndef ASCENSION_NO_REGEX
		/// A concrete rule detects tokens using regular expression match.
		class RegexRule : public Rule {
		public:
			RegexRule(Token::Identifier id, std::unique_ptr<const regex::Pattern> pattern);
			std::unique_ptr<Token> parse(
				const TokenScanner& scanner, const StringPiece& text) const BOOST_NOEXCEPT;
		private:
			std::unique_ptr<const regex::Pattern> pattern_;
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
			virtual ~TokenScanner() BOOST_NOEXCEPT {}
			/// Returns @c false if the scanning is done.
			virtual bool hasNext() const BOOST_NOEXCEPT = 0;
			/// Returns the identifier syntax.
			virtual const text::IdentifierSyntax& identifierSyntax() const BOOST_NOEXCEPT = 0;
			/**
			 * Moves to the next token and returns it.
			 * @return The token or @c null if the scanning was done.
			 */
			virtual std::unique_ptr<Token> nextToken() = 0;
			/**
			 * Starts the scan with the specified range. The current position of the scanner will
			 * be the top of the specified region.
			 * @param document The document
			 * @param region The region to be scanned
			 * @throw kernel#BadRegionException @a region intersects outside of the document
			 */
			virtual void parse(const kernel::Document& document, const kernel::Region& region) = 0;
			/// Returns the current position.
			virtual kernel::Position position() const = 0;
		};

		/**
		 * @c NullTokenScanner returns no tokens. @c NullTokenScanner#hasNext returns always
		 * @c false.
		 */
		class NullTokenScanner : public TokenScanner {
		public:
			bool hasNext() const BOOST_NOEXCEPT;
			const text::IdentifierSyntax& identifierSyntax() const BOOST_NOEXCEPT;
			std::unique_ptr<Token> nextToken();
			void parse(const kernel::Document& document, const kernel::Region& region);
			kernel::Position position() const;
		private:
			boost::optional<kernel::Position> position_;
		};

		/**
		 * A generic scanner which is programable with a sequence of rules. The rules must be
		 * registered before start of scanning. Otherwise @c RunningScannerException will be thrown.
		 * Note that the tokens this scanner returns are only single-line. Multi-line tokens are
		 * not supported by this class.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalTokenScanner : public TokenScanner, private boost::noncopyable {
		public:
			// constructors
			explicit LexicalTokenScanner(kernel::ContentType contentType) BOOST_NOEXCEPT;
			// attributes
			void addRule(std::unique_ptr<const Rule> rule);
			void addWordRule(std::unique_ptr<const WordRule> rule);
			// TokenScanner
			bool hasNext() const BOOST_NOEXCEPT;
			const text::IdentifierSyntax& identifierSyntax() const BOOST_NOEXCEPT;
			std::unique_ptr<Token> nextToken();
			void parse(const kernel::Document& document, const kernel::Region& region);
			kernel::Position position() const BOOST_NOEXCEPT;
		private:
			kernel::ContentType contentType_;
			std::forward_list<std::unique_ptr<const Rule>> rules_;
			std::forward_list<std::unique_ptr<const WordRule>> wordRules_;
			kernel::DocumentCharacterIterator current_;
		};

		/**
		 * A rule for detecting patterns which begin new partition in document.
		 * @see LexicalPartitioner
		 */
		class TransitionRule {
		public:
			virtual ~TransitionRule() BOOST_NOEXCEPT;
			virtual std::unique_ptr<TransitionRule> clone() const = 0;
			kernel::ContentType contentType() const BOOST_NOEXCEPT;
			kernel::ContentType destination() const BOOST_NOEXCEPT;
			virtual Index matches(const String& line, Index offsetInLine) const = 0;
		protected:
			TransitionRule(kernel::ContentType contentType, kernel::ContentType destination) BOOST_NOEXCEPT;
		private:
			const kernel::ContentType contentType_, destination_;
		};

		/// Implementation of @c TransitionRule uses literal string match.
		class LiteralTransitionRule : public TransitionRule {
		public:
			LiteralTransitionRule(kernel::ContentType contentType, kernel::ContentType destination,
				const String& pattern, Char escapeCharacter = text::NONCHARACTER, bool caseSensitive = true);
			std::unique_ptr<TransitionRule> clone() const;
			Index matches(const String& line, Index offsetInLine) const;
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
				kernel::ContentType destination, std::unique_ptr<const regex::Pattern> pattern);
			RegexTransitionRule(const RegexTransitionRule& other);
			std::unique_ptr<TransitionRule> clone() const;
			Index matches(const String& line, Index offsetInLine) const;
		private:
			std::unique_ptr<const regex::Pattern> pattern_;
		};
#endif // !ASCENSION_NO_REGEX

		/**
		 * @c LexicalPartitioner makes document partitions by using the specified lexical rules.
		 * @note This class is not derivable.
		 * @see kernel#Document
		 */
		class LexicalPartitioner : public kernel::DocumentPartitioner, private boost::noncopyable {
		public:
			// constructor
			LexicalPartitioner() BOOST_NOEXCEPT;
			~LexicalPartitioner() BOOST_NOEXCEPT;
			// attribute
			template<typename SinglePassReadableRange>
			void setRules(const SinglePassReadableRange& rules);
		private:
			struct Partition {
				kernel::ContentType contentType;
				kernel::Position start, tokenStart;
				Index tokenLength;
				Partition(kernel::ContentType type, const kernel::Position& p,
					const kernel::Position& startOfToken, Index lengthOfToken) BOOST_NOEXCEPT
					: contentType(type), start(p), tokenStart(startOfToken), tokenLength(lengthOfToken) {}
				kernel::Position getTokenEnd() const BOOST_NOEXCEPT {
					return kernel::Position(tokenStart.line, tokenStart.offsetInLine + tokenLength);
				}
			};
		private:
			void computePartitioning(const kernel::Position& start,
				const kernel::Position& minimalLast, kernel::Region& changedRegion);
//			static void deleteRules(std::list<const TransitionRule*>& rules) BOOST_NOEXCEPT;
			void dump() const;
			void erasePartitions(const kernel::Position& first, const kernel::Position& last);
			ascension::detail::GapVector<Partition*>::const_iterator partitionAt(const kernel::Position& at) const BOOST_NOEXCEPT;
			kernel::ContentType transitionStateAt(const kernel::Position& at) const BOOST_NOEXCEPT;
			Index tryTransition(const String& line, Index offsetInLine,
				kernel::ContentType contentType, kernel::ContentType& destination) const BOOST_NOEXCEPT;
			void verify() const;
			// DocumentPartitioner
			void documentAboutToBeChanged() BOOST_NOEXCEPT;
			void documentChanged(const kernel::DocumentChange& change) BOOST_NOEXCEPT;
			void doGetPartition(const kernel::Position& at, kernel::DocumentPartition& partition) const BOOST_NOEXCEPT;
			void doInstall() BOOST_NOEXCEPT;
		private:
			ascension::detail::GapVector<Partition*> partitions_;
			std::forward_list<std::unique_ptr<const TransitionRule>> rules_;
		};

		/**
		 * Standard implementation of @c presentation#IPartitionPresentationReconstructor. This
		 * implementation performs rule based lexical tokenization using the given @c TokenScanner.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalPartitionPresentationReconstructor : public presentation::PartitionPresentationReconstructor {
		public:
			explicit LexicalPartitionPresentationReconstructor(
				const presentation::Presentation& presentation, std::unique_ptr<TokenScanner> tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>& styles,
				std::shared_ptr<const presentation::TextRunStyle> defaultStyle = std::shared_ptr<const presentation::TextRunStyle>());
		private:
			// presentation.IPartitionPresentationReconstructor
			std::unique_ptr<presentation::DeclaredStyledTextRunIterator> getPresentation(const kernel::Region& region) const BOOST_NOEXCEPT;
		private:
			class StyledTextRunIterator;
			const presentation::Presentation& presentation_;
			std::unique_ptr<TokenScanner> tokenScanner_;
			std::shared_ptr<const presentation::TextRunStyle> defaultStyle_;
			const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>> styles_;
		};


		/// Returns the content type.
		inline kernel::ContentType TransitionRule::contentType() const BOOST_NOEXCEPT {return contentType_;}

		/// Returns the content type of the transition destination.
		inline kernel::ContentType TransitionRule::destination() const BOOST_NOEXCEPT {return destination_;}

		/**
		 * @tparam SinglePassReadableRange The type of @a rules
		 * @param rules The new rules to set
		 */
		template<typename SinglePassReadableRange>
		inline void LexicalPartitioner::setRules(const SinglePassReadableRange& rules) {
			if(document() != nullptr)
				throw IllegalStateException("The partitioner is already connected to document.");
//			std::forward_list<std::unique_ptr<const TransitionRule>> newRules(
//				std::make_move_iterator(std::begin(rules)), std::make_move_iterator(std::end(rules)));
			std::forward_list<std::unique_ptr<const TransitionRule>> newRules;
			boost::for_each(rules, [&newRules](const std::unique_ptr<const TransitionRule>& rule) mutable {
				newRules.push_front(std::move(rule->clone()));
			});
//			std::swap(rules_, newRules);
		}

	}
} // namespace ascension.rules

#endif // !ASCENSION_RULES_HPP
