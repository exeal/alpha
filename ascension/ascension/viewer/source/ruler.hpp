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
#include <ascension/viewer/text-viewer-component.hpp>

namespace ascension {
	namespace viewer {
		namespace source {
			class CompositeRuler;
			struct RulerAllocationWidthSink;
			class SourceViewer;

			/**
			 * This interface defines a visual representation of a ruler in @c SourceViewer. A ruler is placed on one
			 * side of the @c SourceViewer.
			 * @see SourceViewer#ruler, SourceViewer#setRuler
			 */
			class Ruler : public TextViewerComponent {
			public:
				/// Destructor.
				virtual ~Ruler() BOOST_NOEXCEPT {}
				/// Returns the width of this ruler in user units.
				virtual graphics::Scalar width() const BOOST_NOEXCEPT = 0;

			private:
				/**
				 * Installs this ruler to the specified source viewer.
				 * @param viewer The source viewer
				 * @param locator The @c Ruler locator which locates where this ruler is
				 * @param allocationWidthSink The event sink which receives the change of the allocation width
				 */
				virtual void install(SourceViewer& viewer,
					const Locator& locator, RulerAllocationWidthSink& allocationWidthSink) = 0;
				/**
				 * Uninstalls this ruler from the specified source viewer.
				 * @param viewer The source viewer
				 */
				virtual void uninstall(SourceViewer& viewer) = 0;

				using TextViewerComponent::install;
				using TextViewerComponent::uninstall;
				friend class CompositeRuler;
				friend class SourceViewer;
			};
		}
	}
}

#endif // !ASCENSION_RULER_HPP
