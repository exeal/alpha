/**
 * @file selected-region.hpp
 * Defines @c SelectedRegion class.
 * @author exeal
 * @date 2016-02-25 Created.
 */

#ifndef ASCENSION_SELECTED_REGION_HPP
#define ASCENSION_SELECTED_REGION_HPP
#include <ascension/kernel/region.hpp>
#include <ascension/graphics/font/text-hit.hpp>
#include <boost/parameter.hpp>

namespace ascension {
	namespace viewer {
#ifndef ASCENSION_DOXYGEN_SHOULD_SKIP_THIS
		BOOST_PARAMETER_NAME(anchor)
		BOOST_PARAMETER_NAME(caret)
#endif // !ASCENSION_DOXYGEN_SHOULD_SKIP_THIS

		/// Base type of @c SelectedRegion class.
		class SelectedRegionBase : public kernel::Region {
		public:
			/// Constructor takes a named parameters.
			template<typename Arguments>
			explicit SelectedRegionBase(const Arguments& arguments) :
				kernel::Region(arguments[_anchor], arguments[_caret].characterIndex()),
				normal_(arguments[_anchor] <= arguments[_caret].characterIndex()),
				caretIsTrailing_(!arguments[_caret].isLeadingEdge()) {}
			/// Constructor takes a @c kernel#Region.
			explicit SelectedRegionBase(const kernel::Region& region) BOOST_NOEXCEPT : Region(region), normal_(true), caretIsTrailing_(false) {}
			/// Returns const-reference to the position marked as the anchor.
			kernel::Position anchor() const BOOST_NOEXCEPT {
				return normal_ ? *boost::const_begin(*this) : *boost::const_end(*this);
			}
			/// Returns const-reference to the position marked as the caret.
			graphics::font::TextHit<kernel::Position> caret() const BOOST_NOEXCEPT {
				const auto p(normal_ ? *boost::const_end(*this) : *boost::const_begin(*this));
				return caretIsTrailing_ ? graphics::font::makeLeadingTextHit(p) : graphics::font::makeTrailingTextHit(p);
			}

		private:
			bool normal_;
			bool caretIsTrailing_;	// true if the caret is marked as trailing
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
