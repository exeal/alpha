/**
 * @file lexical-partition-presentation-reconstructor.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014
 */

#ifndef ASCENSION_LEXICAL_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP
#define ASCENSION_LEXICAL_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP

#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/rules/token.hpp>
#include <map>
#include <memory>

namespace ascension {
	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {
		class TokenScanner;

		/**
		 * Standard implementation of @c presentation#IPartitionPresentationReconstructor.
		 * This implementation performs rule based lexical tokenization using the given @c TokenScanner.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalPartitionPresentationReconstructor : public presentation::PartitionPresentationReconstructor {
		public:
			explicit LexicalPartitionPresentationReconstructor(
				const presentation::Presentation& presentation, std::unique_ptr<TokenScanner> tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>>& styles,
				std::shared_ptr<const presentation::TextRunStyle> defaultStyle = std::shared_ptr<const presentation::TextRunStyle>());

		private:
			// presentation.PartitionPresentationReconstructor
			std::unique_ptr<presentation::DeclaredStyledTextRunIterator> getPresentation(const kernel::Region& region) const BOOST_NOEXCEPT override;
		private:
			class StyledTextRunIterator;
			const presentation::Presentation& presentation_;
			std::unique_ptr<TokenScanner> tokenScanner_;
			std::shared_ptr<const presentation::TextRunStyle> defaultStyle_;
			const std::map<Token::Identifier, std::shared_ptr<const presentation::TextRunStyle>> styles_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_LEXICAL_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP
