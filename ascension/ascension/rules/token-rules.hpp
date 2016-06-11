/**
 * @file token-rules.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014
 */

#ifndef ASCENSION_TOKEN_RULES_HPP
#define ASCENSION_TOKEN_RULES_HPP
#include <ascension/config.hpp>	// ASCENSION_NO_REGEX
#include <ascension/corelib/regex.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/rules/token.hpp>
#include <ascension/rules/uri-detector.hpp>
#include <memory>

namespace ascension {
	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {
		/// Base class of the two abstract rule classes.
		class RuleBase {
		public:
			/// Destructor.
			virtual ~RuleBase() BOOST_NOEXCEPT {}
			/// Returns the identifier of the token.
			Token::Identifier tokenID() const BOOST_NOEXCEPT {return identifier_;}
		protected:
			explicit RuleBase(Token::Identifier tokenID);
		private:
			const Token::Identifier identifier_;
		};

		/**
		 * Base class of non-word rule classes.
		 * @see LexicalTokenScanner, RegionRule, NumberRule, RegexRule
		 */
		class Rule : public RuleBase {
		public:
			/// Destructor.
			virtual ~Rule() BOOST_NOEXCEPT {}
			/**
			 * Parses and finds a token at the beginning of the given text string.
			 * @param text The text string to parse. This is a while line in the document
			 * @param start The start position of the token
			 * @param identifierSyntax The identifier syntax
			 * @return The end position of the found token, or @c boost::none if not found
			 */
			virtual boost::optional<StringPiece::const_iterator> parse(
				const StringPiece& text, StringPiece::const_iterator start,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT = 0;
		protected:
			explicit Rule(Token::Identifier tokenID) : RuleBase(tokenID) {}
		};

		/**
		 * Base class of word rule classes.
		 * @see LexicalTokenScanner, WordSetRule
		 */
		class WordRule : public RuleBase {
		public:
			/// Destructor.
			virtual ~WordRule() BOOST_NOEXCEPT {}
			/**
			 * Parses and finds a token at the beginning of the given text string.
			 * @param text The text string to parse. This is a while line in the document
			 * @param word The word to check if is a token
			 * @param identifierSyntax The identifier syntax
			 * @return @c true if @a word is a token
			 */
			virtual bool parse(const StringPiece& text, const StringPiece& word,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT = 0;
		protected:
			explicit WordRule(Token::Identifier tokenID) : RuleBase(tokenID) {}
		};

		/***/
		class RegionRule : public Rule {
		public:
			RegionRule(Token::Identifier identifier,
				const String& startSequence, const String& endSequence,
				Char escapeCharacter = text::NONCHARACTER, bool caseSensitive = true);
			boost::optional<StringPiece::const_iterator> parse(
				const StringPiece& text, StringPiece::const_iterator start,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;
		private:
			const String startSequence_, endSequence_;
			const Char escapeCharacter_;
			const bool caseSensitive_;
		};

		/// A concrete rule detects numeric tokens.
		class NumberRule : public Rule {
		public:
			explicit NumberRule(Token::Identifier identifier) BOOST_NOEXCEPT;
			boost::optional<StringPiece::const_iterator> parse(
				const StringPiece& text, StringPiece::const_iterator start,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;
		};

		/// A concrete rule detects URI strings.
		class URIRule : public Rule {
		public:
			URIRule(Token::Identifier identifier,
				std::shared_ptr<const URIDetector> uriDetector) BOOST_NOEXCEPT;
			boost::optional<StringPiece::const_iterator> parse(
				const StringPiece& text, StringPiece::const_iterator start,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;
		private:
			std::shared_ptr<const URIDetector> uriDetector_;
		};

		/// A concrete rule detects the registered words.
		class WordSetRule : protected WordRule {
		public:
			WordSetRule(Token::Identifier identifier,
				const String* first, const String* last, bool caseSensitive = true);
			WordSetRule(Token::Identifier identifier,
				const StringPiece& words, Char separator, bool caseSensitive = true);
			bool parse(const StringPiece& text, const StringPiece& word,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;
		private:
			std::unique_ptr<detail::HashTable> words_;
		};

#ifndef ASCENSION_NO_REGEX
		/// A concrete rule detects tokens using regular expression match.
		class RegexRule : public Rule {
		public:
			RegexRule(Token::Identifier identifier, std::unique_ptr<const regex::Pattern> pattern);
			boost::optional<StringPiece::const_iterator> parse(
				const StringPiece& text, StringPiece::const_iterator start,
				const text::IdentifierSyntax& identifierSyntax) const BOOST_NOEXCEPT override;
		private:
			std::unique_ptr<const regex::Pattern> pattern_;
		};
#endif // !ASCENSION_NO_REGEX
	}
} // namespace ascension.rules

#endif // !ASCENSION_TOKEN_RULES_HPP
