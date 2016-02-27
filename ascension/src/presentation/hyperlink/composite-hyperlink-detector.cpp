/**
 * @file composite-hyperlink-detector.cpp
 * @author exeal
 * @date 2003-2007 (was LineLayout.cpp)
 * @date 2007-2014 was presentation.cpp
 * @date 2014-11-16 Separated from presentation.cpp
 */

#include <ascension/corelib/numeric-range-algorithm/encompasses.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/presentation/hyperlink/hyperlink.hpp>
#include <ascension/presentation/hyperlink/hyperlink-detector.hpp>
#include <boost/foreach.hpp>

namespace ascension {
	namespace presentation {
		namespace hyperlink {
			/// Destructor.
			CompositeHyperlinkDetector::~CompositeHyperlinkDetector() BOOST_NOEXCEPT {
				typedef std::pair<kernel::ContentType, HyperlinkDetector*> Temp;
				BOOST_FOREACH(const Temp& p, composites_)
					delete p.second;
			}
			
			/// @see HyperlinkDetector#nextHyperlink
			std::unique_ptr<Hyperlink> CompositeHyperlinkDetector::nextHyperlink(
					const kernel::Document& document, Index line, const boost::integer_range<Index>& range) const BOOST_NOEXCEPT {
				const kernel::DocumentPartitioner& partitioner = document.partitioner();
				kernel::DocumentPartition partition;
				for(kernel::Position p(line, *range.begin()), e(line, *range.end()); p < e;) {
					partitioner.partition(p, partition);
					assert(encompasses(partition.region, p));
					std::map<kernel::ContentType, HyperlinkDetector*>::const_iterator detector(composites_.find(partition.contentType));
					if(detector != composites_.end()) {
						std::unique_ptr<Hyperlink> found(detector->second->nextHyperlink(
							document, line, boost::irange(kernel::offsetInLine(p), kernel::offsetInLine(std::min(*boost::const_end(partition.region), e)))));
						if(found.get() != nullptr)
							return found;
					}
					p = *boost::const_end(partition.region);
				}
				return std::unique_ptr<Hyperlink>();
			}
			
			/**
			 * Sets the hyperlink detector for the specified content type.
			 * @param contentType The content type. if a detector for this content type was already be set, the
			 *                    old will be deleted
			 * @param detector The hyperlink detector to set. Can't be @c null. The ownership will be
			 *                 transferred to the callee
			 * @throw NullPointerException @a detector is @c null
			 */
			void CompositeHyperlinkDetector::setDetector(kernel::ContentType contentType, std::unique_ptr<HyperlinkDetector> detector) {
				if(detector.get() == nullptr)
					throw NullPointerException("detector");
				std::map<kernel::ContentType, HyperlinkDetector*>::iterator old(composites_.find(contentType));
				if(old != composites_.end()) {
					composites_.erase(old);
					delete old->second;
				}
				composites_.insert(std::make_pair(contentType, detector.release()));
			}
		}
	}
}
