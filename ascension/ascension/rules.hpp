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
#include <boost/range/algorithm/for_each.hpp>

namespace ascension {
	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {
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
	}
} // namespace ascension.rules

#endif // !ASCENSION_RULES_HPP
