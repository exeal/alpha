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
#include <ascension/corelib/basic-types.hpp>
#include <ascension/corelib/string-piece.hpp>
#include <ascension/kernel/content-type.hpp>
#include <memory>

namespace ascension {
	namespace rules {
		/**
		 * A rule for detecting patterns which begin new partition in document.
		 * @see LexicalPartitioner
		 */
		class TransitionRule {
		public:
			/**
			 * Flags to specify which edge of the transition token begins the new partition.
			 * @see #MatchResult, #matches
			 */
			enum TokenBias {
				/// The new partition begins at the beginning of the transition token.
				NEW_PARTITION_BEGINS_AT_BEGINNING_OF_TOKEN,
				/// The new partition begins at the end of the transition token.
				NEW_PARTITION_BEGINS_AT_END_OF_TOKEN
			};

			/// A transition token which is the result of @c #matches method.
			struct TransitionToken {
				Index length;	///< The length of the transition token.
				TokenBias bias;	///< The @c TokenBias.
			};

			/// Destructor.
			virtual ~TransitionRule() BOOST_NOEXCEPT {}
			/**
			 * Creates and returns a copy of the object.
			 * @return A copy of the object
			 */
			virtual std::unique_ptr<TransitionRule> clone() const = 0;
			/// Returns the content type.
			const kernel::ContentType& contentType() const BOOST_NOEXCEPT {return contentType_;}
			/// Returns the content type of the transition destination.
			const kernel::ContentType& destination() const BOOST_NOEXCEPT {return destination_;}
			/**
			 * Returns @c true if the rule matches the specified text.
			 * @param lineString The text string of the line
			 * @param at The start position of the token in @a lineString
			 * @return The found transition token, or @c boost::none if not found
			 * @note @a lineString is neither @c null nor empty
			 */
			virtual boost::optional<TransitionToken> matches(
				const StringPiece& lineString, StringPiece::const_iterator at) const = 0;

		protected:
			/**
			 * Creates a @c TransitionRule instance.
			 * @param contentType The content type of the transition source
			 * @param destination The content type of the transition destination
			 */
			TransitionRule(const kernel::ContentType& contentType, const kernel::ContentType& destination) BOOST_NOEXCEPT
				: contentType_(contentType), destination_(destination) {}
		private:
			const kernel::ContentType contentType_, destination_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_TRANSITION_RULE_HPP
