/**
 * @file presentation-reconstructor.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2014 was presentation.cpp
 * @date 2014-11-18 Separated from presentation.cpp
 */

#include <ascension/presentation/partition-presentation-reconstructor.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/presentation-reconstructor.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/foreach.hpp>
#include <boost/range/irange.hpp>


namespace ascension {
	namespace presentation {
		namespace {
			/// @internal
			class PresentationReconstructorIterator : public presentation::DeclaredStyledTextRunIterator {
			public:
				PresentationReconstructorIterator(const Presentation& presentation,
					const std::map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors, Index line);

			private:
				void updateSubiterator();
				// StyledTextRunIterator
				bool isDone() const BOOST_NOEXCEPT override;
				void next() override;
				kernel::Position position() const BOOST_NOEXCEPT override;
				std::shared_ptr<const DeclaredTextRunStyle> style() const override;
			private:
				const Presentation& presentation_;
				const std::map<kernel::ContentType, PartitionPresentationReconstructor*> reconstructors_;
				const Index line_;
				kernel::DocumentPartition currentPartition_;
				std::unique_ptr<presentation::DeclaredStyledTextRunIterator> subiterator_;
				boost::integer_range<Index> currentRange_;
				std::shared_ptr<const DeclaredTextRunStyle> currentStyle_;
			};
		}

		/**
		 * Constructor.
		 * @param presentation
		 * @param reconstructors
		 * @param line
		 */
		PresentationReconstructorIterator::PresentationReconstructorIterator(
				const Presentation& presentation, const std::map<kernel::ContentType,
				PartitionPresentationReconstructor*> reconstructors, Index line)
				: presentation_(presentation), reconstructors_(reconstructors), line_(line), currentRange_(0, 0) {
			const kernel::DocumentPartitioner& partitioner = presentation_.document().partitioner();
			Index offsetInLine = 0;
			for(const Index lineLength = presentation_.document().lineLength(line);;) {
				partitioner.partition(kernel::Position(line, offsetInLine), currentPartition_);	// this may throw BadPositionException
				if(!boost::empty(currentPartition_.region))
					break;
				if(++offsetInLine >= lineLength) {	// rare case...
					currentPartition_.contentType = kernel::ContentType::DEFAULT_CONTENT;
					currentPartition_.region = kernel::Region::makeSingleLine(line, boost::irange(static_cast<Index>(0), lineLength));
					break;
				}
			}
			updateSubiterator();
		}

		/// @see StyledTextRunIterator#isDone
		bool PresentationReconstructorIterator::isDone() const BOOST_NOEXCEPT {
			return boost::empty(currentPartition_.region);
		}
		
		/// @see StyledTextRunIterator#next
		void PresentationReconstructorIterator::next() {
			if(subiterator_.get() != nullptr) {
				subiterator_->next();
				if(subiterator_->isDone())
					subiterator_.reset();
			}
			if(subiterator_.get() == nullptr) {
				const kernel::Document& document = presentation_.document();
				const Index lineLength = document.lineLength(line_);
				if(*boost::const_end(currentPartition_.region) >= kernel::Position(line_, lineLength)) {
					// done
					currentPartition_.region = kernel::Region::makeEmpty(*boost::const_end(currentPartition_.region));
					return;
				}
				// find the next partition
				const kernel::DocumentPartitioner& partitioner = document.partitioner();
				for(Index offsetInLine = kernel::offsetInLine(*boost::const_end(currentPartition_.region)); ; ) {
					partitioner.partition(kernel::Position(line_, offsetInLine), currentPartition_);
					if(!boost::empty(currentPartition_.region))
						break;
					if(++offsetInLine >= lineLength) {	// rare case...
						currentPartition_.contentType = kernel::ContentType::DEFAULT_CONTENT;
						currentPartition_.region = kernel::Region::makeSingleLine(line_,  boost::irange(offsetInLine, lineLength));
					}
				}
				updateSubiterator();
			}
		}
		
		/// @see StyledTextRunIterator#position
		kernel::Position PresentationReconstructorIterator::position() const BOOST_NOEXCEPT {
			if(subiterator_.get() != nullptr)
				return subiterator_->position();
			else if(!isDone())
				return kernel::Position(line_, currentRange_.front());
			return kernel::Position(line_, presentation_.document().lineLength(line_));
		}
		
		/// @see StyledTextRunIterator#style
		std::shared_ptr<const DeclaredTextRunStyle> PresentationReconstructorIterator::style() const {
			if(subiterator_.get() != nullptr)
				return subiterator_->style();
			else if(!isDone())
				return currentStyle_;
			throw NoSuchElementException();
		}

		inline void PresentationReconstructorIterator::updateSubiterator() {
			auto r(reconstructors_.find(currentPartition_.contentType));
			if(r != std::end(reconstructors_))
				subiterator_ = r->second->presentation(currentPartition_.region);
			else
				subiterator_.reset();
			if(subiterator_.get() == nullptr) {
				currentRange_ = boost::irange(kernel::offsetInLine(*boost::const_begin(currentPartition_.region)), kernel::offsetInLine(*boost::const_end(currentPartition_.region)));
				currentStyle_ = std::shared_ptr<const DeclaredTextRunStyle>(&DeclaredTextRunStyle::unsetInstance(), boost::null_deleter());
			}
		}


		// PresentationReconstructor //////////////////////////////////////////////////////////////////////////////////

		/**
		 * Constructor.
		 * @param presentation The presentation
		 */
		PresentationReconstructor::PresentationReconstructor(Presentation& presentation) : presentation_(presentation) {
			presentation_.setTextRunStyleDeclarator(std::shared_ptr<TextRunStyleDeclarator>(this));	// TODO: danger call (may delete this).
		}
		
		/// Destructor.
		PresentationReconstructor::~PresentationReconstructor() BOOST_NOEXCEPT {
		//	presentation_.setLineStyleDirector(ASCENSION_SHARED_POINTER<LineStyleDirector>());
			typedef std::pair<kernel::ContentType, PartitionPresentationReconstructor*> Temp;
			BOOST_FOREACH(const Temp& p, reconstructors_)
				delete p.second;
		}

		/// @see LineStyleDeclarator#declareTextRunStyle
		std::unique_ptr<DeclaredStyledTextRunIterator> PresentationReconstructor::declareTextRunStyle(Index line) const {
			return std::unique_ptr<DeclaredStyledTextRunIterator>(new PresentationReconstructorIterator(presentation_, reconstructors_, line));
		}
		
		/**
		 * Sets the partition presentation reconstructor for the specified content type.
		 * @param contentType The content type. If a reconstructor for this content type was already be
		 *                    set, the old will be deleted
		 * @param reconstructor The partition presentation reconstructor to set. Can't be @c null. The
		 *                      ownership will be transferred to the callee
		 * @throw NullPointerException @a reconstructor is @c null
		 */
		void PresentationReconstructor::setPartitionReconstructor(
				const kernel::ContentType& contentType, std::unique_ptr<PartitionPresentationReconstructor> reconstructor) {
			if(reconstructor.get() == nullptr)
				throw NullPointerException("reconstructor");
			const std::map<kernel::ContentType, PartitionPresentationReconstructor*>::iterator old(reconstructors_.find(contentType));
			if(old != std::end(reconstructors_)) {
				delete old->second;
				reconstructors_.erase(old);
			}
			reconstructors_.insert(std::make_pair(contentType, reconstructor.release()));
		}
	}
}
