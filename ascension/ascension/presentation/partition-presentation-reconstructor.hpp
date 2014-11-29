/**
 * @file partition-presentation-reconstructor.hpp
 * @author exeal
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2011 was presentation.hpp
 * @date 2011-05-04 separated from presentation.hpp
 * @date 2012-2014 was presentation-reconstructor.hpp
 * @date 2014-11-18 Separated from presentation-reconstructor.hpp
 * @see presentation-reconstructor.hpp
 */

#ifndef ASCENSION_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP
#define ASCENSION_PARTITION_PRESENTATION_RECONSTRUCTOR_HPP

#include <memory>	// std.shared_ptr, std.unique_ptr
#include <boost/config.hpp>	// BOOST_NOEXCEPT

namespace ascension {
	namespace kernel {
		class Region;
	}

	namespace presentation {
		struct DeclaredStyledTextRunIterator;
		class DeclaredTextRunStyle;

		/**
		 * Creates (reconstructs) styles of the document region. This is used by
		 * @c PresentationReconstructor class to manage the styles in the specified content type.
		 * @see PresentationReconstructor#setPartitionReconstructor
		 */
		class PartitionPresentationReconstructor {
		public:
			/// Destructor.
			virtual ~PartitionPresentationReconstructor() BOOST_NOEXCEPT {}
			/**
			 * Returns the styled text segments for the specified document region.
			 * @param region The region to reconstruct the new presentation
			 * @return The presentation or @c null (filled by the presentation's default style)
			 */
			virtual std::unique_ptr<DeclaredStyledTextRunIterator>
				presentation(const kernel::Region& region) const BOOST_NOEXCEPT = 0;
		};

		/// Reconstructs document presentation with single text style.
		class SingleStyledPartitionPresentationReconstructor : public PartitionPresentationReconstructor {
		public:
			explicit SingleStyledPartitionPresentationReconstructor(std::shared_ptr<const DeclaredTextRunStyle> style) BOOST_NOEXCEPT;

		private:
			// PartitionPresentationReconstructor
			std::unique_ptr<DeclaredStyledTextRunIterator> presentation(const kernel::Region& region) const override;
		private:
			const std::shared_ptr<const DeclaredTextRunStyle> style_;
		};
	}
} // namespace ascension.presentation

#endif // !ASCENSION_PARTITION_PRESENTATION_CONSTRUCTOR_HPP
