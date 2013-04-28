/**
 * @file virtual-box.hpp
 * This header defines several visual presentation classes.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2013 was viewer.hpp
 * @date 2013-04-29 separated from viewer.hpp
 */

#ifndef ASCENSION_VIRTUAL_BOX_HPP
#define ASCENSION_VIRTUAL_BOX_HPP

#include <ascension/graphics/font/text-layout.hpp>
#include <array>
#include <utility>


namespace ascension {
	namespace viewers {
		class TextViewer;

		/**
		 * A virtual rectangle placed in the viewer.
		 * @note This feature is not fully available on bidirectional texts.
		 * @see Caret#boxForRectangleSelection
		 */
		class VirtualBox {
			ASCENSION_UNASSIGNABLE_TAG(VirtualBox);
		public:
			VirtualBox(const TextViewer& viewer, const kernel::Region& region) BOOST_NOEXCEPT;
			bool characterRangeInVisualLine(
				const graphics::font::VisualLine& line, boost::integer_range<Index>& range) const BOOST_NOEXCEPT;
			bool includes(const graphics::Point& p) const BOOST_NOEXCEPT;
			void update(const kernel::Region& region) BOOST_NOEXCEPT;
		private:
			struct Point {
				graphics::font::VisualLine line;
				graphics::Scalar ipd;	// distance from left/top-edge of content-area
			};
			std::array<Point, 2> points_;
			const TextViewer& viewer_;
			const Point& beginning() const BOOST_NOEXCEPT {
				return points_[(points_[0].line <= points_[1].line) ? 0 : 1];
			}
			const Point& end() const BOOST_NOEXCEPT {
				return points_[(&beginning() == &points_[0]) ? 1 : 0];
			}
			graphics::Scalar startEdge() const BOOST_NOEXCEPT {
				return std::min(points_[0].ipd, points_[1].ipd);
			}
			graphics::Scalar endEdge() const BOOST_NOEXCEPT {
				return std::max(points_[0].ipd, points_[1].ipd);
			}
		};
	}
} // namespace ascension.viewers

#endif // !ASCENSION_VIRTUAL_BOX_HPP
