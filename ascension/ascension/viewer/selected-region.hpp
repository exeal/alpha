/**
 * @file selected-region.hpp
 * Defines @c SelectedRegion class.
 * @author exeal
 * @date 2016-02-25 Created.
 */

#ifndef ASCENSION_SELECTED_REGION_HPP
#define ASCENSION_SELECTED_REGION_HPP
#include <ascension/kernel/region.hpp>
#include <ascension/viewer/text-hit.hpp>
#include <boost/parameter.hpp>

namespace ascension {
	namespace viewer {
#ifndef ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING
		BOOST_PARAMETER_NAME(anchor)
		BOOST_PARAMETER_NAME(caret)
		BOOST_PARAMETER_NAME(document)
#endif // !ASCENSION_DETAIL_DOXYGEN_IS_PREPROCESSING

		/// Base type of @c SelectedRegion class.
		class SelectedRegionBase : public kernel::Region {
		public:
			/// Constructor takes a named parameters.
			template<typename Arguments>
			explicit SelectedRegionBase(const Arguments& arguments) :
				kernel::Region(arguments[_anchor], insertionPosition(arguments[_document], arguments[_caret])), anchor_(&arguments[_anchor]), caret_(arguments[_caret]) {}
			/// Constructor takes a @c kernel#Region.
			explicit SelectedRegionBase(const kernel::Region& region) BOOST_NOEXCEPT : Region(region), caret_(TextHit::leading(*boost::const_end(*this))) {
				anchor_ = &*boost::const_begin(*this);
			}
			/// Returns const-reference to the position marked as the anchor.
			const kernel::Position& anchor() const BOOST_NOEXCEPT {
				return *anchor_;
			}
			/// Returns const-reference to the position marked as the caret.
			const TextHit& caret() const BOOST_NOEXCEPT {
				return caret_;
			}

		private:
			const kernel::Position* anchor_;
			TextHit caret_;
		};

		/**
		 * A region selected by a caret. A @c SelectedRegion marks which position is the caret.
		 * @see kernel#Region, Caret
		 */
		class SelectedRegion : public SelectedRegionBase {
		public:
			/// Constructor takes named parameters as initial values.
			BOOST_PARAMETER_CONSTRUCTOR(
				SelectedRegion, (SelectedRegionBase), tag,
				(required
					(document, (const kernel::Document&))
					(anchor, (const kernel::Position&))
					(caret, (const graphics::font::TextHit<kernel::Position>&))))
			/**
			 * Creates @c SelectedRegion from a region.
			 * @param region The region
			 * @post anchor() == *boost::const_begin(region)
			 * @post caret() == *boost::const_end(region)
			 */
			explicit SelectedRegion(const kernel::Region& region) BOOST_NOEXCEPT : SelectedRegionBase(region) {
			}
		};
	}
}

#endif // !ASCENSION_SELECTED_REGION_HPP
