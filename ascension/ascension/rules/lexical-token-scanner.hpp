/**
 * @file lexical-token-scanner.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 */

#ifndef ASCENSION_LEXICAL_TOKEN_SCANNER_HPP
#define ASCENSION_LEXICAL_TOKEN_SCANNER_HPP
#include <ascension/kernel/document-character-iterator.hpp>
#include <ascension/kernel/partition.hpp>
#include <ascension/rules/token-scanner.hpp>
#include <boost/core/noncopyable.hpp>
#include <forward_list>

namespace ascension {
	namespace rules {
		class TokenRule;
		class WordTokenRule;

		/**
		 * A generic scanner which is programable with a sequence of rules.
		 * The rules must be registered before start of scanning. Otherwise @c RunningScannerException will be thrown.
		 * Note that the tokens this scanner returns are only single-line. Multi-line tokens are not supported by this
		 * class.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalTokenScanner : public TokenScanner, private boost::noncopyable {
		public:
			// constructors
			explicit LexicalTokenScanner(kernel::ContentType contentType) BOOST_NOEXCEPT;
			// attributes
			void addRule(std::unique_ptr<const TokenRule> rule);
			void addWordRule(std::unique_ptr<const WordTokenRule> rule);
			// TokenScanner
			bool hasNext() const BOOST_NOEXCEPT override;
			const text::IdentifierSyntax& identifierSyntax() const BOOST_NOEXCEPT override;
			std::unique_ptr<Token> nextToken() override;
			void parse(const kernel::Document& document, const kernel::Region& region) override;
			kernel::Position position() const BOOST_NOEXCEPT override;

		private:
			kernel::ContentType contentType_;
			std::forward_list<std::unique_ptr<const TokenRule>> rules_;
			std::forward_list<std::unique_ptr<const WordTokenRule>> wordRules_;
			kernel::DocumentCharacterIterator current_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_LEXICAL_TOKEN_SCANNER_HPP
