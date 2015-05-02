/**
 * @file font-face.hpp
 * @author exeal
 * @date 2010-11-06 created
 * @date 2015-04-26 Separated from font.hpp.
 */

#ifndef ASCENSION_FONT_FACE_HPP
#define ASCENSION_FONT_FACE_HPP
#include <ascension/graphics/font/font-description.hpp>

namespace ascension {
	namespace graphics {
		namespace font {
			class FontCollection;
			class FontFamily;

			/// Used to represent a group of fonts with the same family, slant, weight, width, but varying sizes.
			class FontFace {
			public:
				const FontDescription& describe() const;
				/// Returns the face name.
				const String& name() const;
			private:
				const FontDescription description_;
				const String name_;
			};

			class FontFaceIterator {};
			class FontSizeIterator {};

			FontFaceIterator availableFaces(const FontCollection& collection, const FontFamily& family);
			FontSizeIterator availablePointSizes(const FontFace& fontFace);
		}
	}
}

#endif // !ASCENSION_FONT_FACE_HPP
