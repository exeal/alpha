/**
 * @file transition-rules.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 */

#ifndef ASCENSION_TRANSITION_RULES_HPP
#define ASCENSION_TRANSITION_RULES_HPP
#include <ascension/config.hpp>	// ASCENSION_NO_REGEX
#include <ascension/corelib/regex.hpp>
#include <ascension/kernel/partition.hpp>
#include <memory>

namespace ascension {
	namespace rules {
		/**
		 * A rule for detecting patterns which begin new partition in document.
		 * @see LexicalPartitioner
		 */
		class TransitionRule {
		public:
			virtual ~TransitionRule() BOOST_NOEXCEPT;
			/**
			 * Creates and returns a copy of the object.
			 * @return A copy of the object
			 */
			virtual std::unique_ptr<TransitionRule> clone() const = 0;
			/// Returns the content type.
			kernel::ContentType contentType() const BOOST_NOEXCEPT {return contentType_;}
			/// Returns the content type of the transition destination.
			kernel::ContentType destination() const BOOST_NOEXCEPT {return destination_;}
			/**
			 * Returns @c true if the rule matches the specified text. Note that an implementation can't use the
			 * partitioning of the document to generate the new partition.
			 * @param line The target line text
			 * @param offsetInLine The offset in the line at which match starts
			 * @return The length of the matched pattern. If and only if the match failed, returns 0. If matched zero
			 *         width text, returns 1
			 * @todo This documentation is confusable.
			 */
			virtual Index matches(const StringPiece& line, Index offsetInLine) const = 0;

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
			std::unique_ptr<TransitionRule> clone() const override;
			Index matches(const StringPiece& line, Index offsetInLine) const override;
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
			std::unique_ptr<TransitionRule> clone() const override;
			Index matches(const StringPiece& line, Index offsetInLine) const override;
		private:
			std::unique_ptr<const regex::Pattern> pattern_;
		};
#endif // !ASCENSION_NO_REGEX
	}
} // namespace ascension.rules

#endif // !ASCENSION_TRANSITION_RULES_HPP
