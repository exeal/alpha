/**
 * @file transition-rule.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014 was rules.hpp
 * @date 2014-11-16 Separated from rules.hpp
 * @date 2016-08-13 Renamed from transition-rules.hpp.
 */

#ifndef ASCENSION_TRANSITION_RULE_HPP
#define ASCENSION_TRANSITION_RULE_HPP
#include <ascension/corelib/string-piece.hpp>
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
			/// Destructor.
			virtual ~TransitionRule() BOOST_NOEXCEPT {}
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
			/**
			 * Creates a @c TransitionRule instance.
			 * @param contentType The content type of the transition source
			 * @param destination The content type of the transition destination
			 */
			TransitionRule(kernel::ContentType contentType, kernel::ContentType destination) BOOST_NOEXCEPT
				: contentType_(contentType), destination_(destination) {}
		private:
			const kernel::ContentType contentType_, destination_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_TRANSITION_RULE_HPP
