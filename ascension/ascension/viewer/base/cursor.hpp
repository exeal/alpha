/**
 * @file cursor.hpp
 * @author exeal
 * @date 2011-06-25 created
 */

#ifndef ASCENSION_CURSOR_HPP
#define ASCENSION_CURSOR_HPP

#include <ascension/graphics/geometry.hpp>

namespace ascension {
	namespace viewers {
		namespace base {

			class Cursor {
			public:
				static void hide();
				static graphics::NativePoint position();
				static void setPosition(const graphics::NativePoint& p);
				static void show();
			};

		}
	}
}

#endif // !ASCENSION_CURSOR_HPP
