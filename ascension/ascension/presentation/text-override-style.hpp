/**
 * @file text-override-style.hpp
 * Defines @c TextOverrideStyle class.
 * @author exeal
 * @date 2016-02-28 Created.
 */

#ifndef ASCENSION_TEXT_OVERRIDE_STYLE_HPP
#define ASCENSION_TEXT_OVERRIDE_STYLE_HPP
#include <ascension/graphics/color.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace presentation {
		struct TextOverrideStyle : std::enable_shared_from_this<TextOverrideStyle> {
			struct Part {
				/// Foreground color. If @c boost#none, the system-default value is used.
				boost::optional<graphics::Color> foreground;
				/// Background color. If @c boost#none, the system-default value is used.
				boost::optional<graphics::Color> background;
//				styles::TextShadow textShadow;
			};

			Part selection;			///< Active selected text.
			Part inactiveSelection;	///< Inactive selected text.
			Part restriction;		///< Inaccessible area.
		};
	}
}

#endif // !ASCENSION_TEXT_OVERRIDE_STYLE_HPP
