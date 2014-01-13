/**
 * @file scanner.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 */

#include <ascension/rules.hpp>
//#include <ascension/corelib/ustring.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/find.hpp>


namespace ascension {
	namespace rules {
		// NullTokenScanner ///////////////////////////////////////////////////////////////////////////////////////////
		
		/// @see TokenScanner#hasNext
		bool NullTokenScanner::hasNext() const BOOST_NOEXCEPT {
			return false;
		}
		
		/// @see TokenScanner#identifierSyntax
		const text::IdentifierSyntax& NullTokenScanner::identifierSyntax() const BOOST_NOEXCEPT {
			return text::IdentifierSyntax::defaultInstance();
		}
		
		/// @see TokenScanner#nextToken
		std::unique_ptr<Token> NullTokenScanner::nextToken() {
			return std::unique_ptr<Token>();
		}
		
		/// @see TokenScanner#parse
		void NullTokenScanner::parse(const kernel::Document&, const kernel::Region&) {
		}
		
		/// @see TokenScanner#position
		kernel::Position NullTokenScanner::position() const {
			if(!position_)
				throw BadScannerStateException();
			return *position_;
		}


		// LexicalTokenScanner ////////////////////////////////////////////////////////////////////////////
		
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
			return current_.document()->contentTypeInformation().getIdentifierSyntax(contentType_);
		}
		
		/// @see TokenScanner#nextToken
		std::unique_ptr<Token> LexicalTokenScanner::nextToken() {
			const text::IdentifierSyntax& idSyntax = identifierSyntax();
			std::unique_ptr<Token> result;
			const String* line = &current_.line();
			while(current_.hasNext()) {
				if(current_.current() == text::LINE_SEPARATOR) {
					current_.next();
					line = &current_.line();
					if(!current_.hasNext())
						break;
				}
				const Char* const p = line->data() + current_.tell().offsetInLine;
				const Char* const last = line->data() + line->length();
				BOOST_FOREACH(const std::unique_ptr<const Rule>& rule, rules_) {
					result = rule->parse(*this, makeStringPiece(p, last));
					if(result.get() != nullptr) {
						current_.seek(result->region.end());
						return result;
					}
				}
				const Char* const wordEnd = idSyntax.eatIdentifier(p, last);
				if(wordEnd > p) {
					if(!wordRules_.empty()) {
						BOOST_FOREACH(const std::unique_ptr<const WordRule>& wordRule, wordRules_) {
							result = wordRule->parse(*this, makeStringPiece(p, wordEnd));
							if(result.get() != nullptr) {
								current_.seek(result->region.end());
								return result;
							}
						}
					}
					current_.seek(kernel::Position(current_.tell().line, wordEnd - line->data()));
				} else
					current_.next();
			}
			return result;
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
