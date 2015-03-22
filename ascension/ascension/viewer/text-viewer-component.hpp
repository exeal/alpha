/**
 * @file text-viewer-component.hpp
 * Defines @c TextViewerComponent interface.
 * @author exeal
 * @date 2015-03-17 Created
 */

#ifndef ASCENSION_TEXT_VIEWER_COMPONENT_HPP
#define ASCENSION_TEXT_VIEWER_COMPONENT_HPP
#include <ascension/graphics/geometry/rectangle.hpp>
#include <memory>

namespace ascension {
	namespace graphics {
		class PaintContext;
	}

	namespace viewer {
		class MouseInputStrategy;
		class TextViewer;

		/**
		 * A @c TextViewerComponent is rectangular portion inside the @c TextViewer.
		 * @see TextArea, source#Ruler
		 */
		class TextViewerComponent {
		public:
			/// Interface of the object which locates where the specified component is.
			struct Locator {
				/**
				 * Returns the allocation-rectangle of the specified component, in viewer-local coordinates.
				 * @param component The component to locate
				 * @return The allocation-rectangle of @a component
				 * @throw std#invalid_argument @a component was not found
				 */
				virtual graphics::Rectangle locateComponent(const TextViewerComponent& component) const = 0;
			};

		public:
			/// Destructor.
			virtual ~TextViewerComponent() {}
			/**
			 * Returns @c MouseInputStrategy object which handles the mouse input.
			 * @return The @c MouseInputStrategy object, or @c null if this component ignores the mouse input
			 * @note The default implementation returns @c null.
			 */
			virtual std::weak_ptr<MouseInputStrategy> mouseInputStrategy() const {
				return std::weak_ptr<MouseInputStrategy>();
			}
			/**
			 * Paints the content of this component.
			 * @param context The paint context
			 */
			virtual void paint(graphics::PaintContext& context) = 0;
			/**
			 * The 'allocation-rectangle' of this component was changed.
			 * @see #Locator
			 */
			virtual void relocated() {}

		protected:
			/**
			 * Installs this component to the specified text viewer.
			 * @param viewer The text viewer
			 * @param locator The @c locator which locates where this component is
			 * @see #uninstall
			 */
			virtual void install(TextViewer& viewer, const Locator& locator) = 0;
			/**
			 * Uninstalls this component from the specified text viewer.
			 * @param viewer The text viewer
			 * @see #install
			 */
			virtual void uninstall(TextViewer& viewer) = 0;

			friend class TextViewer;
		};
	}
}

#endif // !ASCENSION_TEXT_VIEWER_COMPONENT_HPP
