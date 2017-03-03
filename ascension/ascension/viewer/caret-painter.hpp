/**
 * @file caret-painter.hpp
 * Defines @c CaretPainter abstract class.
 * @author exeal
 * @date 2015-11-22 Separated from text-area.hpp.
 * @date 2015-11-23 Renamed from caret-blinker.hpp.
 * @date 2015-11-26 Reverted from ascension/viewer/detail/.
 */

#ifndef ASCENSION_CARET_PAINTER_HPP
#define ASCENSION_CARET_PAINTER_HPP
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

		/**
		 * An interface for object which paints the caret on the @c TextArea.
		 * @see Caret, StandardCaretPainter, TextArea
		 */
		class CaretPainter {
		public:
			/// Destructor.
			virtual ~CaretPainter() BOOST_NOEXCEPT {}
			/// Hides the cursor.
			virtual void hide() = 0;
			/**
			 * Installs this @c CaretPainterBase instance to the @c TextArea object.
			 * @param caret The caret
			 */
			virtual void install(Caret& caret) = 0;
			/**
			 * Paints the caret.
			 * @param context The graphics context
			 * @param layout The line layout
			 * @param alignmentPoint The 'alignment-point' of @a layout
			 */
			virtual void paintIfShows(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) = 0;
			/// Shows and begins blinking the caret.
			virtual void show() = 0;
			/// Returns @c true if the caret is shown (may be blinking off).
			virtual bool shows() const BOOST_NOEXCEPT = 0;
			/**
			 * Uninstalls this @c CaretPainterBase instance from the @c TextArea object.
			 * @param caret The caret
			 */
			virtual void uninstall(Caret& caret) = 0;
			friend class Caret;
		};
	}
}

#endif // !ASCENSION_CARET_PAINTER_HPP
