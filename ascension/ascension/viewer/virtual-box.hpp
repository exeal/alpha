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
			boost::optional<boost::integer_range<Index>>
				characterRangeInVisualLine(const graphics::font::VisualLine& line) const BOOST_NOEXCEPT;
			bool includes(const graphics::Point& p) const BOOST_NOEXCEPT;
			void update(const kernel::Region& region) BOOST_NOEXCEPT;
		private:
			const TextViewer& viewer_;
			boost::integer_range<graphics::font::VisualLine> lines_;
			boost::integer_range<graphics::Scalar> ipds_;	// inline-progression-dimension in TextRenderer coordinates
		};
	}
} // namespace ascension.viewers

#endif // !ASCENSION_VIRTUAL_BOX_HPP