/**
 * @file lexical-partition-presentation-reconstructor.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Renamed from lexical-partitioning.cpp
 */

#include <ascension/corelib/ustring.hpp>
#include <ascension/corelib/utility.hpp>	// detail.searchBound
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/rules/lexical-partition-presentation-reconstructor.hpp>
#include <boost/foreach.hpp>
#if defined(_DEBUG)
#	include <boost/log/trivial.hpp>
#endif


namespace ascension {
	namespace rules {

		/// @internal
		class LexicalPartitionPresentationReconstructor::StyledTextRunIterator : public presentation::DeclaredStyledTextRunIterator {
		public:
			StyledTextRunIterator(const kernel::Document& document, TokenScanner& tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles,
				std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle, const kernel::Region& region);

		private:
			void nextRun();
			// presentation.DeclaredStyledTextRunIterator
			boost::integer_range<Index> currentRange() const override;
			std::shared_ptr<const presentation::DeclaredTextRunStyle> currentStyle() const override;
			bool isDone() const BOOST_NOEXCEPT override;
			void next() override;
		private:
//			const LexicalPartitionPresentationReconstructor& reconstructor_;
			TokenScanner& tokenScanner_;
			const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles_;
			std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle_;
			const kernel::Region region_;
			kernel::Region currentRegion_;
			std::shared_ptr<const presentation::DeclaredTextRunStyle> currentStyle_;
			std::unique_ptr<Token> next_;
		};

		LexicalPartitionPresentationReconstructor::StyledTextRunIterator::StyledTextRunIterator(
				const kernel::Document& document, TokenScanner& tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles,
				std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle, const kernel::Region& region) :
				tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), currentRegion_(region.beginning()) {
			tokenScanner_.parse(document, region);
			nextRun();
		}
		
		/// @see StyledTextRunIterator#currentRange
		boost::integer_range<Index> LexicalPartitionPresentationReconstructor::StyledTextRunIterator::currentRange() const {
			if(isDone())
				throw NoSuchElementException();
			if(currentRegion_.lines().size() == 1)
				return boost::irange(currentRegion_.beginning().offsetInLine, currentRegion_.end().offsetInLine);
			return boost::irange(currentRegion_.beginning().offsetInLine, currentRegion_.end().offsetInLine);
		}
		
		/// @see StyledTextRunIterator#current
		std::shared_ptr<const presentation::DeclaredTextRunStyle> LexicalPartitionPresentationReconstructor::StyledTextRunIterator::currentStyle() const {
			if(isDone())
				throw NoSuchElementException();
			return currentStyle_;
		}
		
		/// @see StyledTextRunIterator#isDone
		bool LexicalPartitionPresentationReconstructor::StyledTextRunIterator::isDone() const {
			return currentRegion_.end() == region_.end();
		}
		
		/// @see StyledTextRunIterator#next
		void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::next() {
			if(isDone())
				throw NoSuchElementException();
			nextRun();
		}

		inline void LexicalPartitionPresentationReconstructor::StyledTextRunIterator::nextRun() {
			if(next_.get() != nullptr) {
				const auto style(styles_.find(next_->id));
				currentRegion_ = next_->region;
				currentStyle_ = (style != styles_.end()) ? style->second : defaultStyle_;
				next_.reset();
			} else if(tokenScanner_.hasNext()) {
				next_ = move(tokenScanner_.nextToken());
				assert(next_.get() != nullptr);
				assert(next_->region.beginning() >= currentRegion_.end());
				if(next_->region.beginning() != currentRegion_.end()) {
					// 
					currentRegion_ = kernel::Region(currentRegion_.end(), next_->region.beginning());
					currentStyle_ = defaultStyle_;
				} else {
					const auto style(styles_.find(next_->id));
					currentRegion_ = next_->region;
					currentStyle_ = (style != styles_.end()) ? style->second : defaultStyle_;
					next_.reset();
				}
			} else if(currentRegion_.end() != region_.end()) {
				//
				currentRegion_ = kernel::Region(currentRegion_.end(), region_.end());
				currentStyle_ = defaultStyle_;
			}
		}

		/**
		 * Constructor.
		 * @param presentation The presentation which gives the document and default text run style
		 * @param tokenScanner The token scanner to use for tokenization
		 * @param styles Token identifier to its text style map
		 * @param defaultStyle The style for the regions out of tokens. can be @c null
		 * @throw NullPointerException @a tokenScanner is @c null
		 */
		LexicalPartitionPresentationReconstructor::LexicalPartitionPresentationReconstructor(
				const presentation::Presentation& presentation, std::unique_ptr<TokenScanner> tokenScanner,
				const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles,
				std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle /* = shared_ptr<const presentation::DeclaredTextRunStyle>() */)
				: presentation_(presentation), tokenScanner_(std::move(tokenScanner)), styles_(styles) {
			if(tokenScanner_.get() == nullptr)
				throw NullPointerException("tokenScanner");
		}

		/// @see presentation#PartitionPresentationReconstructor#presentation
		std::unique_ptr<presentation::DeclaredStyledTextRunIterator> LexicalPartitionPresentationReconstructor::presentation(const kernel::Region& region) const BOOST_NOEXCEPT {
			return std::unique_ptr<presentation::DeclaredStyledTextRunIterator>(
				new StyledTextRunIterator(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
		}
	}
}
