/**
 * @file partition-presentation-reconstructor.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2014 was presentation.cpp
 * @date 2014-11-18 Separated from presentation.cpp
 */

#include <ascension/presentation/partition-presentation-reconstructor.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <boost/range/irange.hpp>


namespace ascension {
	namespace presentation {
		namespace {
			/// @internal
			class SingleStyledPartitionPresentationReconstructorIterator : public presentation::DeclaredStyledTextRunIterator {
			public:
				SingleStyledPartitionPresentationReconstructorIterator(const kernel::Region& region,
						std::shared_ptr<const DeclaredTextRunStyle> style) BOOST_NOEXCEPT : region_(region), style_(style), done_(region.isEmpty()) {
				}

			private:
				// StyledTextRunIterator
				bool isDone() const BOOST_NOEXCEPT override {
					return done_;
				}
				void next() override {
					if(done_)
						throw NoSuchElementException();
					done_ = true;
				}
				kernel::Position position() const BOOST_NOEXCEPT override {
					return !done_ ? region_.beginning() : region_.end();
				}
				std::shared_ptr<const DeclaredTextRunStyle> style() const override {
					if(done_)
						throw NoSuchElementException();
					return style_;
				}
			private:
				const kernel::Region region_;
				const std::shared_ptr<const DeclaredTextRunStyle> style_;
				bool done_;
			};
		}

		/**
		 * Constructor.
		 * @param style The style
		 */
		SingleStyledPartitionPresentationReconstructor::SingleStyledPartitionPresentationReconstructor(std::shared_ptr<const DeclaredTextRunStyle> style) BOOST_NOEXCEPT : style_(style) {
		}
		
		/// @see PartitionPresentationReconstructor#presentation
		std::unique_ptr<DeclaredStyledTextRunIterator> SingleStyledPartitionPresentationReconstructor::presentation(const kernel::Region& region) const {
			return std::unique_ptr<presentation::DeclaredStyledTextRunIterator>(new SingleStyledPartitionPresentationReconstructorIterator(region, style_));
		}
	}
}
