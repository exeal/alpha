/**
* @file stacked-widget.cpp
* Implements @c win32#StackedWidget class.
* @author exeal
* @date 2017-02-01 Created.
*/

#include "win32/stacked-widget.hpp"
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>

namespace alpha {
	namespace win32 {
		/// Creates a @c StackedWidget instance.
		StackedWidget::StackedWidget() : horizontallyHomogeneous_(true), verticallyHomogeneous_(true) {
		}

		/// Destructor.
		StackedWidget::~StackedWidget() BOOST_NOEXCEPT {
		}

		/**
		 * Appends the specified widget to this @c StackedWidget.
		 * @param widget The widget to append
		 * @throw std#invalid_argument @a widget is already added
		 * @see #removeWidget
		 */
		void StackedWidget::addWidget(ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> widget) {
			if(ascension::win32::boole(::IsChild(handle().get(), widget->handle().get())))
				throw std::invalid_argument("widget");	// already added
			ascension::viewer::widgetapi::setParentWidget(widget, *this);
			if(numberOfWidgets() > 0)
				setCurrentWidget(widget);
		}

		/**
		 * Returns the current widget, or @c null if there are no child widgets.
		 * @see #setCurrentWidget
		 */
		ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> StackedWidget::currentWidget() const {
			ascension::win32::Handle<HWND> current;
			foreachChildren([&current](ascension::win32::Handle<HWND> child) {
				if(ascension::viewer::widgetapi::isVisible(ascension::win32::Window(child))) {
					current = child;
					return false;
				}
				return true;
			});
			return ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget>(ascension::win32::Window(current));
		}

		/// Returns the number of widgets contained by this @c StackedWidget.
		std::size_t StackedWidget::numberOfWidgets() const {
			std::size_t n = 0;
			foreachChildren([&n](ascension::win32::Handle<HWND>) {
				return ++n, true;
			});
			return n;
		}

		/// @internal
		BOOL CALLBACK StackedWidget::processChildWindow(HWND window, LPARAM lp) {
			auto f = reinterpret_cast<std::function<bool(ascension::win32::Handle<HWND>)>*>(lp);
			if(!(*f)(ascension::win32::borrowed(window)))
				return FALSE;
			return TRUE;
		}
	
		/// @see ascension#win32#CustomControl#processMessage
		LRESULT StackedWidget::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
			switch(message) {
				case WM_SETFOCUS:
					if(auto child = currentWidget()) {
						ascension::viewer::widgetapi::setFocus(child);
						consumed = true;
						return 0l;
					}
					break;
				case WM_SIZE:
					if(isHorizontallyHomogeneous() || isVerticallyHomogeneous()) {
						if(auto child = currentWidget()) {
							if(ascension::viewer::widgetapi::isRealized(child)) {
								auto bounds(ascension::viewer::widgetapi::bounds(child, false));
								const auto thisBounds(ascension::viewer::widgetapi::bounds(*this, false));
								if(isHorizontallyHomogeneous())
									ascension::graphics::geometry::range<0>(bounds) = ascension::graphics::geometry::range<0>(thisBounds);
								if(isVerticallyHomogeneous())
									ascension::graphics::geometry::range<1>(bounds) = ascension::graphics::geometry::range<1>(thisBounds);
								ascension::viewer::widgetapi::setBounds(child, bounds);
								consumed = true;
								return 0l;
							}
						}
					}
					break;
			}

			return ascension::win32::CustomControl<StackedWidget>::processMessage(message, wp, lp, consumed);
		}

		/**
		 * Removed the specified widget from this @c StackedWidget.
		 * @param widget The widget to remove
		 * @throw std#invalid_argument @a widget is not a child
		 * @see #addWidget
		 */
		void StackedWidget::removeWidget(ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> widget) {
			if(!ascension::win32::boole(::IsChild(handle().get(), widget->handle().get())))
				throw std::invalid_argument("widget");
			::SetParent(widget->handle().get(), nullptr);
		}

		/**
		 * Sets the current widget to be the specified widget.
		 * @param widget The widget to select
		 * @throw std#invalid_argument @a widget is not a child
		 * @see #currentWidget
		 */
		void StackedWidget::setCurrentWidget(ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> widget) {
			if(!ascension::win32::boole(::IsChild(handle().get(), widget->handle().get())))
				throw std::invalid_argument("widget");

			ascension::win32::Window target(widget->handle());
			if(ascension::viewer::widgetapi::isVisible(target))
				return;

			// resize this and target
			if(!isHorizontallyHomogeneous() || !isVerticallyHomogeneous()) {
				const auto childBounds(ascension::viewer::widgetapi::bounds(target, false));
				std::remove_const<decltype(childBounds)>::type newBounds;
				if(isHorizontallyHomogeneous() || isVerticallyHomogeneous())
					newBounds = ascension::viewer::widgetapi::bounds(*this, false);
				if(!isHorizontallyHomogeneous())
					ascension::graphics::geometry::range<0>(newBounds) = ascension::graphics::geometry::range<0>(childBounds);
				if(!isVerticallyHomogeneous())
					ascension::graphics::geometry::range<1>(newBounds) = ascension::graphics::geometry::range<1>(childBounds);
				ascension::viewer::widgetapi::setBounds(*this, newBounds);
			}
			ascension::viewer::widgetapi::show(target);

			// hide other widgets
			foreachChildren([&target](ascension::win32::Handle<HWND> child) {
				if(child != target.handle())
					ascension::viewer::widgetapi::hide(ascension::win32::Window(child));
				return true;
			});
		}

		/// @see ascension#win32#CustomControl#windowClass
		void StackedWidget::windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT {
			out.name = L"alpha.win32.StackedWidget";
			out.styles = CS_HREDRAW | CS_VREDRAW;
		}
	}
}
