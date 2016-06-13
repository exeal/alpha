/**
 * @file lexical-partition-presentation-reconstructor.hpp
 * @author exeal
 * @date 2004-2006 was Lexer.h
 * @date 2006-2014
 */

#ifndef ASCENSION_LEXICAL_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP
#define ASCENSION_LEXICAL_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP
#include <ascension/presentation/partition-presentation-reconstructor.hpp>
#include <ascension/rules/token.hpp>
#include <ascension/rules/token-scanner.hpp>
#include <map>
#include <memory>

namespace ascension {
	/// Provides a framework for rule based text scanning and document partitioning.
	namespace rules {
		/**
		 * Standard implementation of @c presentation#PartitionPresentationReconstructor.
		 * This implementation performs rule based lexical tokenization using the given @c TokenScanner.
		 * @note This class is not intended to be subclassed.
		 */
		class LexicalPartitionPresentationReconstructor : public presentation::PartitionPresentationReconstructor {
		public:
			explicit LexicalPartitionPresentationReconstructor(
				const presentation::Presentation& presentation, std::unique_ptr<TokenScanner> tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles,
				std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle = std::shared_ptr<const presentation::DeclaredTextRunStyle>());

		private:
			// presentation.PartitionPresentationReconstructor
			std::unique_ptr<presentation::DeclaredStyledTextRunIterator> presentation(const kernel::Region& region) const BOOST_NOEXCEPT override;
		private:
			const presentation::Presentation& presentation_;
			std::unique_ptr<TokenScanner> tokenScanner_;
			std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle_;
			const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>> styles_;
		};
	}
} // namespace ascension.rules

#endif // !ASCENSION_LEXICAL_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP
