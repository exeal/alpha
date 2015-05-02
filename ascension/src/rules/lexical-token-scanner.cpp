/**
 * @file lexical-token-scanner.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Separated from scanner.cpp
 */

#include <ascension/kernel/document.hpp>
#include <ascension/rules/lexical-token-scanner.hpp>
#include <ascension/rules/token.hpp>
#include <ascension/rules/token-rules.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>


namespace ascension {
	namespace rules {		
		/**
		 * Constructor.
		 * @param contentType The content the scanner parses
		 */
		LexicalTokenScanner::LexicalTokenScanner(kernel::ContentType contentType) BOOST_NOEXCEPT : contentType_(contentType), current_() {
		}

		/**
		 * Adds the new rule to the scanner.
		 * @param rule The rule to be added
		 * @throw NullPointerException @a rule is @c null
		 * @throw std#invalid_argument @a rule is already registered
		 * @throw BadScannerStateException The scanner is running
		 */
		void LexicalTokenScanner::addRule(std::unique_ptr<const Rule> rule) {
			if(rule.get() == nullptr)
				throw NullPointerException("rule");
			else if(hasNext())
				throw BadScannerStateException();
			else if(boost::range::find(rules_, rule) != boost::end(rules_))
				throw std::invalid_argument("The rule is already registered.");
			rules_.push_front(std::move(rule));
		}
		
		/**
		 * Adds the new word rule to the scanner.
		 * @param rule The rule to be added
		 * @throw NullPointerException @a rule is @c null
		 * @throw std#invalid_argument @a rule is already registered
		 * @throw BadScannerStateException The scanner is running
		 */
		void LexicalTokenScanner::addWordRule(std::unique_ptr<const WordRule> rule) {
			if(rule.get() == nullptr)
				throw NullPointerException("rule");
			else if(hasNext())
				throw BadScannerStateException();
			else if(boost::range::find(wordRules_, rule) != boost::end(wordRules_))
				throw std::invalid_argument("The rule is already registered.");
			wordRules_.push_front(std::move(rule));
		}
		
		/// @see TokenScanner#hasNext
		bool LexicalTokenScanner::hasNext() const BOOST_NOEXCEPT {
			return current_.hasNext();
		}
		
		/// @see TokenScanner#identifierSyntax
		const text::IdentifierSyntax& LexicalTokenScanner::identifierSyntax() const BOOST_NOEXCEPT {
			return current_.document().contentTypeInformation().getIdentifierSyntax(contentType_);
		}
		
		/// @see TokenScanner#nextToken
		std::unique_ptr<Token> LexicalTokenScanner::nextToken() {
			// TODO: This code is not exception-safe.
			const text::IdentifierSyntax& ids = identifierSyntax();
			StringPiece line(current_.line());
			while(current_.hasNext()) {
				if(*current_ == text::LINE_SEPARATOR) {
					++current_;
					line = current_.line();
					if(!current_.hasNext())
						break;
				}
				const StringPiece::const_iterator p(line.cbegin() + current_.tell().offsetInLine);
				BOOST_FOREACH(const std::unique_ptr<const Rule>& rule, rules_) {
					const boost::optional<StringPiece::const_iterator> endOfToken(rule->parse(line, p, ids));
					if(endOfToken != boost::none) {
						const kernel::Position beginningOfToken(current_.tell());
						current_.seek(kernel::Position(beginningOfToken.line, boost::get(endOfToken) - line.cbegin()));
						return std::unique_ptr<Token>(new Token(rule->tokenID(), beginningOfToken));
					}
				}
				const StringPiece::const_iterator endOfWord(ids.eatIdentifier(p, line.cend()));
				if(endOfWord > p) {
					if(!wordRules_.empty()) {
						BOOST_FOREACH(const std::unique_ptr<const WordRule>& wordRule, wordRules_) {
							if(wordRule->parse(line, makeStringPiece(p, endOfWord), ids)) {
								const kernel::Position beginningOfToken(current_.tell());
								current_.seek(kernel::Position(beginningOfToken.line, endOfWord - line.cbegin()));
								return std::unique_ptr<Token>(new Token(wordRule->tokenID(), beginningOfToken));
							}
						}
					}
					current_.seek(kernel::Position(current_.tell().line, endOfWord - line.cbegin()));
				} else
					++current_;
			}
			return std::unique_ptr<Token>();
		}
		
		/// @see TokenScanner#parse
		void LexicalTokenScanner::parse(const kernel::Document& document, const kernel::Region& region) {
			current_ = kernel::DocumentCharacterIterator(document, region);
		}
		
		/// @see TokenScanner#position
		kernel::Position LexicalTokenScanner::position() const {
			if(current_ == kernel::DocumentCharacterIterator())
				throw BadScannerStateException();
			return current_.tell();
		}
	}
}
