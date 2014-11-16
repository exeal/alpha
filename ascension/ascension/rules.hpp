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
