/**
 * @file viewer-gtk.cpp
 * @author exeal
 * @date 2013-11-10 Created.
 * @date 2013-2014
 */

#include <ascension/viewer/viewer.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)

#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/viewer/widgetapi/drag-and-drop.hpp>

namespace ascension {
	namespace viewers {
		void TextViewer::doBeep() BOOST_NOEXCEPT {
#if 1
			::gdk_beep();
#else
			get_display()->beep();
#endif
		}

		/// Implements @c Gtk#Widget#get_preferred_height_for_width_vfunc.
		void TextViewer::get_preferred_height_for_width_vfunc(int, int& minimumHeight, int& naturalHeight) const {
			return get_preferred_height_vfunc(minimumHeight, naturalHeight);
		}

		/// Implements @c Gtk#Widget#get_preferred_height_vfunc.
		void TextViewer::get_preferred_height_vfunc(int& minimumHeight, int& naturalHeight) const {
			// TODO: Not implemented.
		}

		/// Implements @c Gtk#Widget#get_preferred_width_for_height_vfunc.
		void TextViewer::get_preferred_width_for_height_vfunc(int, int& minimumWidth, int& naturalWidth) const {
			return get_preferred_width_vfunc(minimumWidth, naturalWidth);
		}

		/// Implements @c Gtk#Widget#get_preferred_width_vfunc.
		void TextViewer::get_preferred_width_vfunc(int& minimumWidth, int& naturalWidth) const {
			// TODO: Not implemented.
		}

		/// Implements @c Gtk#Widget#get_request_mode_vfunc.
		Gtk::SizeRequestMode TextViewer::get_request_mode_vfunc() const {
			return Gtk::Widget::get_request_mode_vfunc();
		}

		void TextViewer::hideToolTip() {
			// TODO: Not implemented.
		}

		void TextViewer::initializeNativeObjects() {
			set_can_focus(true);
			set_redraw_on_allocate(false);
//			drag_dest_set_target_list();

			get_hadjustment()->signal_value_changed().connect([this]() {
				if(const std::shared_ptr<graphics::font::TextViewport> viewport = this->textRenderer().viewport())
					viewport->scroll(graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset>(
						graphics::_x = static_cast<graphics::font::TextViewportSignedScrollOffset>(
							this->property_hadjustment().get_value()->get_value() - this->scrollPositionsBeforeChanged_.x()),
						graphics::_y = 0));
//				this->scrollPositionsBeforeChanged_.x() = this->get_hadjustment()->get_value();
//				this->scrollPositionsBeforeChanged_.y() = this->get_vadjustment()->get_value();
			});
			get_vadjustment()->signal_value_changed().connect([this]() {
				if(const std::shared_ptr<graphics::font::TextViewport> viewport = this->textRenderer().viewport())
					viewport->scroll(graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset>(
						graphics::_x = 0,
						graphics::_y = static_cast<graphics::font::TextViewportSignedScrollOffset>(
							this->property_vadjustment().get_value()->get_value() - this->scrollPositionsBeforeChanged_.y())));
//				this->scrollPositionsBeforeChanged_.x() = this->get_hadjustment()->get_value();
//				this->scrollPositionsBeforeChanged_.y() = this->get_vadjustment()->get_value();
			});
		}

		namespace {
			static const int NATIVE_BUTTON_MASK = GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK;
			static const int NATIVE_KEYBOARD_MASK = ~NATIVE_BUTTON_MASK;

			template<typename NativeEvent>
			inline std::tuple<
				graphics::Point,
				widgetapi::event::LocatedUserInput::MouseButton,
				widgetapi::event::UserInput::KeyboardModifier
			> makeLocatedUserInput(const NativeEvent& event) {
				return std::make_tuple(
					graphics::geometry::make<graphics::Point>((graphics::geometry::_x = event.x, graphics::geometry::_y = event.y)),
					static_cast<widgetapi::event::LocatedUserInput::MouseButton>(event.state & NATIVE_BUTTON_MASK),
					static_cast<widgetapi::event::UserInput::KeyboardModifier>(event.state & NATIVE_KEYBOARD_MASK));
			}

			widgetapi::event::MouseButtonInput makeMouseButtonInput(const GdkEventButton& event) {
				static const widgetapi::event::LocatedUserInput::MouseButton NATIVE_BUTTON_VALUES[] = {
					widgetapi::event::LocatedUserInput::BUTTON1_DOWN, widgetapi::event::LocatedUserInput::BUTTON2_DOWN,
					widgetapi::event::LocatedUserInput::BUTTON3_DOWN, widgetapi::event::LocatedUserInput::BUTTON4_DOWN,
					widgetapi::event::LocatedUserInput::BUTTON5_DOWN
				};
				const auto a(makeLocatedUserInput(event));
				const widgetapi::event::LocatedUserInput::MouseButton button =
					(event.button >= 1 && event.button <= 5) ? NATIVE_BUTTON_VALUES[event.button - 1] : widgetapi::event::LocatedUserInput::NO_BUTTON;
				return widgetapi::event::MouseButtonInput(std::get<0>(a), button, std::get<1>(a), std::get<2>(a));
			}
		}

		/**
		 * Invokes @c #mousePressed, @c #mouseDoubleClicked and @c #mouseTripleClicked methods.
		 * @see Widget#on_button_press_event
		 */
		bool TextViewer::on_button_press_event(GdkEventButton* event) {
			widgetapi::setFocus(*this);
			if(::gdk_event_triggers_context_menu(reinterpret_cast<GdkEvent*>(event)) != 0) {
				doShowContextMenu(event);
				return true;
			}

			widgetapi::event::MouseButtonInput input(makeMouseButtonInput(*event));
			if(input.button() != widgetapi::event::LocatedUserInput::NO_BUTTON) {
				switch(event->type) {
					case Gdk::BUTTON_PRESS:
						mousePressed(input);
						break;
					case Gdk::DOUBLE_BUTTON_PRESS:
						mouseDoubleClicked(input);
						break;
					case Gdk::TRIPLE_BUTTON_PRESS:
						mouseTripleClicked(input);
						break;
				}
			}
			return input.isConsumed() || Gtk::Widget::on_button_press_event(event);
		}

		/**
		 * Invokes @c #mouseReleased method.
		 * @see Widget#on_button_release_event
		 */
		bool TextViewer::on_button_release_event(GdkEventButton* event) {
			widgetapi::event::MouseButtonInput input(makeMouseButtonInput(*event));
			if(input.button() != widgetapi::event::LocatedUserInput::NO_BUTTON && event->type == GDK_BUTTON_RELEASE)
				mouseReleased(input);
			return input.isConsumed() || Gtk::Widget::on_button_release_event(event);
		}

		/**
		 * Invokes @c #resized method.
		 * @see Gtk#Widget#on_configure_event
		 */
		bool TextViewer::on_configure_event(GdkEventConfigure* event) {
			resized(graphics::Dimension(
				graphics::geometry::_dx = static_cast<graphics::Scalar>(event->width),
				graphics::geometry::_dy = static_cast<graphics::Scalar>(event->height)));
			return false;
		}

		/// @see Gtk#Widget#on_drag_drop
		bool TextViewer::on_drag_drop(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) {
			if(mouseInputStrategy_.get() != nullptr) {
				if(const std::shared_ptr<widgetapi::DropTarget> dropTarget = mouseInputStrategy_->handleDropTarget()) {
					detail::DragEventAdapter adapter(*dropTarget);
					return adapter.adaptDropEvent(context, x, y, time);
				}
			}
			return false;
		}

		/// @see Gtk#Widget#on_drag_leave
		void TextViewer::on_drag_leave(const Glib::RefPtr<Gdk::DragContext>& context, guint time) {
			if(mouseInputStrategy_.get() != nullptr) {
				if(const std::shared_ptr<widgetapi::DropTarget> dropTarget = mouseInputStrategy_->handleDropTarget()) {
					detail::DragEventAdapter adapter(*dropTarget);
					return adapter.adaptDragLeaveEvent(context, time);
				}
			}
		}

		/// @see Gtk#Widget#on_drag_motion
		bool TextViewer::on_drag_motion(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time) {
			if(mouseInputStrategy_.get() != nullptr) {
				if(const std::shared_ptr<widgetapi::DropTarget> dropTarget = mouseInputStrategy_->handleDropTarget()) {
					detail::DragEventAdapter adapter(*dropTarget);
					return adapter.adaptDragMoveEvent(context, x, y, time);
				}
			}
			return false;
		}

		/**
		 * Invokes @c #paint method.
		 * @see Gtk#Widget#on_draw
		 */
		bool TextViewer::on_draw(const Cairo::RefPtr<Cairo::Context>& context) {
			double x1, y1, x2, y2;
			context->get_clip_extents(x1, y1, x2, y2);
			const graphics::Rectangle boundsToPaint(std::make_pair(
				graphics::Point(graphics::geometry::_x = static_cast<graphics::Scalar>(x1), graphics::geometry::_y = static_cast<graphics::Scalar>(y1)),
				graphics::Point(graphics::geometry::_x = static_cast<graphics::Scalar>(x2), graphics::geometry::_y = static_cast<graphics::Scalar>(y2))));
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
			graphics::PaintContext pc(graphics::RenderingContext2D(context), boundsToPaint);
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
			graphics::PaintContext pc(widgetapi::createRenderingContext(*this), boundsToPaint);
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			paint(pc);
			return true;
		}

		/**
		 * Invokes @c #focusGained method.
		 * @see Gtk#Widget#on_focus_in_event
		 */
		bool TextViewer::on_focus_in_event(GdkEventFocus*) {
			widgetapi::event::Event e;
			focusGained(e);
			return e.isConsumed();
		}

		/**
		 * Invokes @c #focusAboutToBeLost method.
		 * @see Gtk#Widget#on_focus_out_event
		 */
		bool TextViewer::on_focus_out_event(GdkEventFocus*) {
			widgetapi::event::Event e;
			focusAboutToBeLost(e);
			return e.isConsumed();
		}

		void TextViewer::on_grab_focus() {
			// TODO: Not implemented.
		}

		/**
		 * Invokes @c #keyPressed method.
		 * @see Gtk#Widget#on_key_press_event
		 */
		bool TextViewer::on_key_press_event(GdkEventKey* event) {
			widgetapi::event::KeyInput input(event->keyval, static_cast<widgetapi::event::UserInput::KeyboardModifier>(event->state));
			keyPressed(input);
			return input.isConsumed();
		}

		/**
		 * Invokes @c #keyReleased method.
		 * @see Gtk#Widget#on_key_release_event
		 */
		bool TextViewer::on_key_release_event(GdkEventKey* event) {
			widgetapi::event::KeyInput input(event->keyval, static_cast<widgetapi::event::UserInput::KeyboardModifier>(event->state));
			keyReleased(input);
			return input.isConsumed();
		}

		/**
		 * Invokes @c #mouseMoved method.
		 * @see Widget#on_motion_notify_event
		 */
		bool TextViewer::on_motion_notify_event(GdkEventMotion* event) {
			const auto a(makeLocatedUserInput(*event));
			widgetapi::event::LocatedUserInput input(std::get<0>(a), std::get<1>(a), std::get<2>(a));
			mouseMoved(input);
			return input.isConsumed();
		}

		/// @see Gtk#Widget#on_realize
		void TextViewer::on_realize() {
			set_realized();

			const Gtk::Allocation allocation(get_allocation());
			GdkWindowAttr attributes;
			std::memset(&attributes, 0, sizeof(decltype(attributes)));
			static const gint attributesMask = GDK_WA_X | GDK_WA_Y/* | GDK_WA_VISUAL*/;
			attributes.x = allocation.get_x();
			attributes.y = allocation.get_y();
			attributes.width = allocation.get_width();
			attributes.height = allocation.get_height();
			attributes.event_mask = get_events()
				| Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK
				| Gdk::EXPOSURE_MASK | Gdk::FOCUS_CHANGE_MASK
				| Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK
				| Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK
				| Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK;
//			attributes.visual = ::gtk_widget_get_visual(gobj());
			attributes.window_type = GDK_WINDOW_CHILD;
			attributes.wclass = GDK_INPUT_OUTPUT;
			window_ = Gdk::Window::create(get_parent_window(), &attributes, attributesMask);
			assert(window_);
			set_window(window_);
#if 0
			register_window(textAreaWindow_);
#else
			window_->set_user_data(Gtk::Widget::gobj());
#endif
			initializeGraphics();
		}

		/**
		 * Invokes @c #mouseWheelChanged method.
		 * @see Gtk#Widget#on_scroll_event
		 */
		bool TextViewer::on_scroll_event(GdkEventScroll* event) {
			const graphics::geometry::BasicDimension<unsigned int> scrollAmount(graphics::geometry::_dx = 3, graphics::geometry::_dy = 3);
//			static const double NOTCH_RESOLUTION = 120;
			graphics::geometry::BasicDimension<double> wheelRotation(graphics::geometry::_dx = 0, graphics::geometry::_dy = 0);
			switch(event->direction) {
				case GDK_SCROLL_UP:
					graphics::geometry::dy(wheelRotation) = +1;
					break;
				case GDK_SCROLL_DOWN:
					graphics::geometry::dy(wheelRotation) = -1;
					break;
				case GDK_SCROLL_LEFT:
					graphics::geometry::dx(wheelRotation) = +1;
					break;
				case GDK_SCROLL_RIGHT:
					graphics::geometry::dx(wheelRotation) = -1;
					break;
				case GDK_SCROLL_SMOOTH:
					graphics::geometry::dx(wheelRotation) = event->delta_x;
					graphics::geometry::dy(wheelRotation) = event->delta_y;
					break;
			}
			widgetapi::event::MouseWheelInput input(
				graphics::geometry::make<graphics::Point>((graphics::geometry::_x = event->x, graphics::geometry::_y = event->y)),
				event->state & NATIVE_BUTTON_MASK, event->state & NATIVE_KEYBOARD_MASK, scrollAmount, wheelRotation);
			mouseWheelChanged(input);
			return input.isConsumed();
		}

		/// @see Gtk#Widget#on_size_allocate
		void TextViewer::on_size_allocate(Gtk::Allocation& allocation) {
			set_allocation(allocation);
#if 0
			if(textAreaWindow_ && get_realized()) {
				graphics::Rectangle r(textAreaAllocationRectangle());
				textAreaWindow_->move_resize(
					static_cast<int>(graphics::geometry::left(r)), static_cast<int>(graphics::geometry::top(r)),
					static_cast<int>(graphics::geometry::dx(r)), static_cast<int>(graphics::geometry::dy(r)));

				const RulerStyles& rulerStyles = rulerPainter_->declaredStyles();
				boost::geometry::assign_zero<graphics::Rectangle>(r);
				if(indicatorMargin(rulerStyles)->visible)
					r = graphics::geometry::joined(r, rulerPainter_->indicatorMarginAllocationRectangle());
				if(lineNumbers(rulerStyles)->visible)
					r = graphics::geometry::joined(r, rulerPainter_->lineNumbersAllocationRectangle());
				textAreaWindow_->move_resize(
					static_cast<int>(graphics::geometry::left(r)), static_cast<int>(graphics::geometry::top(r)),
					static_cast<int>(graphics::geometry::dx(r)), static_cast<int>(graphics::geometry::dy(r)));
			}
#else
			if(window_)
				window_->move_resize(allocation.get_x(), allocation.get_y(), allocation.get_width(), allocation.get_height());
#endif
		}

		/// @see Gtk#Widget#on_unrealize
		void TextViewer::on_unrealize() {
			window_.reset();
			Gtk::Widget::on_unrealize();
		}

		void TextViewer::showContextMenu(const widgetapi::event::LocatedUserInput& input, void* nativeEvent) {
			// TODO: Not implemented.
		}
	}
}

#endif	// ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
