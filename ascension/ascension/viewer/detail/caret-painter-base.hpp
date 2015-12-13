/**
 * @file caret-painter-base.hpp
 * Defines @c CaretPainterBase internal class.
 * @author exeal
 * @date 2015-11-22 Separated from text-area.hpp.
 * @date 2015-11-23 Renamed from caret-blinker.hpp.
 * @date 2015-11-26 Reverted from ascension/viewer/detail/.
 * @date 2015-11-27 Separated from caret-painter.hpp.
 */

#ifndef ASCENSION_CARET_PAINTER_BASE_HPP
#define ASCENSION_CARET_PAINTER_BASE_HPP
#include <ascension/graphics/geometry/point.hpp>

namespace ascension {
	namespace graphics {
		class PaintContext;

		namespace font {
			class TextLayout;
		}
	}

	namespace viewer {
		class Caret;
		class TextArea;

		namespace detail {
			class CaretPainterBase {
			public:
				/// Destructor.
				virtual ~CaretPainterBase() BOOST_NOEXCEPT {}

			private:
				/// Hides the cursor.
				virtual void hide() = 0;
				/**
				 * Installs this @c CaretPainterBase instance to the @c TextArea object.
				 * @param caret The caret
				 */
				virtual void install(Caret& caret) = 0;
				/// Returns @c true if the caret is visible.
				virtual bool isVisible() const BOOST_NOEXCEPT = 0;
				/**
				 * Paints the caret.
				 * @param context The graphics context
				 * @param layout The line layout
				 * @param alignmentPoint The 'alignment-point' of @a layout
				 */
				virtual void paintIfShows(graphics::PaintContext& context,
					const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) = 0;
				/// Pends blinking of the caret(s).
				virtual void pend() = 0;
				/// 
				virtual void resetTimer() = 0;
				/// Shows and begins blinking the caret.
				virtual void show() = 0;
				/// Returns @c true if the caret is shown (may be blinking off).
				virtual bool shows() const BOOST_NOEXCEPT = 0;
				/**
				 * Uninstalls this @c CaretPainterBase instance from the @c TextArea object.
				 * @param caret The caret
				 */
				virtual void uninstall(Caret& caret) = 0;
				/// Checks and updates state of blinking of the caret.
				virtual void update() = 0;
				friend class TextArea;
			};
		}
	}
}

#endif // !ASCENSION_CARET_PAINTER_BASE_HPP
