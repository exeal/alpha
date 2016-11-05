/**
 * @file lexical-partition-presentation-reconstructor.cpp
 * @author exeal
 * @date 2004-2006 (was Lexer.cpp)
 * @date 2006-2014 was rules.cpp
 * @date 2014-01-13 separated from rules.cpp
 * @date 2014-11-16 Renamed from lexical-partitioning.cpp
 */

#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/rules/lexical-partition-presentation-reconstructor.hpp>

namespace ascension {
	namespace rules {
		namespace {
			class StyledTextRunIteratorImpl : public presentation::DeclaredStyledTextRunIterator {
			public:
				StyledTextRunIteratorImpl(const kernel::Document& document, TokenScanner& tokenScanner,
					const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles,
					std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle, const kernel::Region& region);

			private:
				// presentation.DeclaredStyledTextRunIterator
				bool isDone() const BOOST_NOEXCEPT override;
				void next() override;
				kernel::Position position() const BOOST_NOEXCEPT override;
				std::shared_ptr<const presentation::DeclaredTextRunStyle> style() const override;
			private:
//				const LexicalPartitionPresentationReconstructor& reconstructor_;
				TokenScanner& tokenScanner_;
				const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles_;
				std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle_;
				const kernel::Region region_;
				kernel::Position position_;
				std::shared_ptr<const presentation::DeclaredTextRunStyle> style_;
				std::unique_ptr<Token> next_;
			};

			StyledTextRunIteratorImpl::StyledTextRunIteratorImpl(
					const kernel::Document& document, TokenScanner& tokenScanner,
					const std::map<Token::Identifier, std::shared_ptr<const presentation::DeclaredTextRunStyle>>& styles,
					std::shared_ptr<const presentation::DeclaredTextRunStyle> defaultStyle, const kernel::Region& region) :
					tokenScanner_(tokenScanner), styles_(styles), defaultStyle_(defaultStyle), region_(region), position_(*boost::const_begin(region)) {
				tokenScanner_.parse(document, region);
				do {
					if(!tokenScanner_.hasNext()
							|| (next_ = std::move(tokenScanner_.nextToken())).get() == nullptr)	// tokenScanner_ didn't give a token
						style_ = defaultStyle_;
					else if(next_->position >= *boost::const_end(region_)) {	// the first token is far beyond region_
						style_ = defaultStyle_;
						next_.reset();
					} else if(next_->position > *boost::const_begin(region_))	// the first token didn't start at the beginning of region_
						style_ = defaultStyle_;
					else if(next_->position == *boost::const_begin(region_)) {	// the first token starts at the beginning of region_ (usual case)
						const auto style(styles_.find(next_->identifier));
						style_ = (style != std::end(styles_)) ? style->second : defaultStyle_;
						next_.reset();
					} else
						continue;
				} while(false);
				assert(next_.get() == nullptr || next_->position > position_);
			}
		
			/// @see StyledTextRunIterator#isDone
			bool StyledTextRunIteratorImpl::isDone() const BOOST_NOEXCEPT {
				return position_ == *boost::const_end(region_);
			}
		
			/// @see StyledTextRunIterator#next
			void StyledTextRunIteratorImpl::next() {
				if(isDone())
					throw NoSuchElementException();
				if(next_.get() != nullptr) {	// the next element is already in hand
					const auto style(styles_.find(next_->identifier));
					position_ = next_->position;
					style_ = (style != std::end(styles_)) ? style->second : defaultStyle_;
					next_.reset();	// next_ is consumed
					return;
				}

				do {
					if(!tokenScanner_.hasNext()
							|| (next_ = std::move(tokenScanner_.nextToken())).get() == nullptr)	// tokenScanner_ didn't the next token
						position_ = *boost::const_end(region_);	// done
					else if(next_->position >= *boost::const_end(region_)) {	// the next token is far beyond region_
						position_ = *boost::const_end(region_);	// done
						next_.reset();
					} else if(next_->position > position_) {	// found the next valid token (usual case)
						position_ = next_->position;
						const auto style(styles_.find(next_->identifier));
						style_ = (style != std::end(styles_)) ? style->second : defaultStyle_;
						next_.reset();
					} else
						continue;
				} while(false);
			}
		
			/// @see StyledTextRunIterator#position
			kernel::Position StyledTextRunIteratorImpl::position() const BOOST_NOEXCEPT {
				return !isDone() ? position_ : *boost::const_end(region_);
			}
		
			/// @see StyledTextRunIterator#style
			std::shared_ptr<const presentation::DeclaredTextRunStyle> StyledTextRunIteratorImpl::style() const {
				if(isDone())
					throw NoSuchElementException();
				return style_;
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
				new StyledTextRunIteratorImpl(presentation_.document(), *tokenScanner_, styles_, defaultStyle_, region));
		}
	}
}
