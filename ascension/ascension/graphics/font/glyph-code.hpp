/**
 * @file glyph-code.hpp
 * @author exeal
 * @date 2010-11-06 created as font.hpp
 * @date 2010-2012 was font.hpp
 * @date 2012-08-26 separated from font.hpp
 * @date 2012-09-16 separated from text-run.hpp
 * @date 2015-04-26 Separated from glyph-vector.hpp.
 */

#ifndef ASCENSION_GLYPH_CODE_HPP
#define ASCENSION_GLYPH_CODE_HPP
#include <cstdint>

namespace ascension {
	namespace graphics {
		namespace font {
			/// 16-bit glyph index value.
			typedef std::uint16_t GlyphCode;
		}
	}
}

#endif // !ASCENSION_GLYPH_CODE_HPP
