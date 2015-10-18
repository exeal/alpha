/**
 * @file text-viewport-base.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2012
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 * @date 2012-02-18 separated from text-renderer.hpp
 * @date 2015-10-17 Separated from text-viewport.hpp
 */

#ifndef ASCENSION_TEXT_VIEWPORT_BASE_HPP
#define ASCENSION_TEXT_VIEWPORT_BASE_HPP
#include <ascension/corelib/basic-types.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			/// Base class of @c TextViewport which defines the integer types.
			struct TextViewportBase {
				typedef Index ScrollOffset;
				typedef SignedIndex SignedScrollOffset;
			};
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_VIEWPORT_HPP
