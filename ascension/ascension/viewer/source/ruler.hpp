/**
 * @file ruler.hpp
 * Defines @c Ruler interface.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-06-06 separated from viewer.hpp
 * @date 2011-2014 was viewer/ruler.hpp
 * @date 2015-01-12 Redesigned.
 */

#ifndef ASCENSION_RULER_HPP
#define ASCENSION_RULER_HPP
#include <ascension/graphics/geometry/common.hpp>

namespace ascension {
	namespace graphics {
		class PaintContext;
	}

	namespace viewers {
		class TextViewer;

		namespace source {
			class CompositeRuler;

			// TODO: Ruler is considered to be used with source.SourceViewer, not TextViewer. Fix later.

			/**
			 * This interface defines a visual representation of a ruler in @c SourceViewer. A ruler is placed on one
			 * side of the @c SourceViewer.
			 * @see SourceViewer#ruler, SourceViewer#setRuler
			 */
			class Ruler {
			public:
				/// Destructor.
				virtual ~Ruler() BOOST_NOEXCEPT {}
				/// Paints the content of this ruler.
				virtual void paint(graphics::PaintContext& context) = 0;
				/// Returns the width of this ruler in user units.
				virtual graphics::Scalar width() const BOOST_NOEXCEPT = 0;
			private:
				/**
				 * Installs this ruler to the specified text viewer.
				 * @param viewer The text viewer
				 */
				virtual void install(const TextViewer& viewer) = 0;
				/**
				 * Uninstalls this ruler from the specified text viewer.
				 * @param viewer The text viewer
				 */
				virtual void uninstall(const TextViewer& viewer) = 0;
				friend class CompositeRuler;
				friend class TextViewer;
			};
		}
	}
}

#endif // !ASCENSION_RULER_HPP
