/**
 * @file paned-widget.cpp
 * Implements @c win32#PanedWidget class.
 * @author exeal
 * @date 2017-02-01 Created.
 */

#include "win32/paned-widget.hpp"
#include <ascension/corelib/numeric-range.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-range.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>

namespace alpha {
	namespace win32 {
		/// Creates a @c PanedWidget instance.
		PanedWidget::PanedWidget() : horizontal_(true), firstChildSizeRatio_(0.5f) {
		}

		/// Destructor.
		PanedWidget::~PanedWidget() BOOST_NOEXCEPT {
		}

		namespace {
			BOOL CALLBACK enumerateDescendants(HWND window, LPARAM lp) {
				auto& f = *reinterpret_cast<std::function<bool(ascension::win32::Handle<HWND>)>*>(lp);
				if(!f(ascension::win32::borrowed(window)))
					return FALSE;
				::EnumChildWindows(window, &enumerateDescendants, lp);
				return TRUE;
			}

			template<typename Function>
			void foreachDescendants(ascension::win32::Handle<HWND> window, Function function) {
				std::function<bool(ascension::win32::Handle<HWND>)> f(function);
				enumerateDescendants(window.get(), reinterpret_cast<LPARAM>(&f));
			}
		}

		/**
		 * Handles @c WM_LBUTTONDBLCLK window message.
		 * @param modifiers Indicates whether virtual keys are down
		 * @param location The cursor location in the client area, in user units
		 * @param[out] consumed
		 */
		void PanedWidget::onLButtonDblClk(UINT modifiers, const POINT& location, bool& consumed) {
			// unsplit when the gap was double-clicked
			if(numberOfChildren() == 2) {
				// check if any widget in descendants of the second child widget has focus
				auto activeWindow(ascension::win32::borrowed(::GetActiveWindow()));
				auto focusedWindow(ascension::win32::borrowed(::GetFocus()));
				if(focusedWindow.get() != nullptr) {
					bool secondIsActive = false;
					foreachDescendants(std::get<1>(children_)->handle(), [&activeWindow, &focusedWindow, &secondIsActive](ascension::win32::Handle<HWND> window) {
						if(window == activeWindow)
							activeWindow.reset();
						if(window == focusedWindow)
							focusedWindow.reset();
						if(activeWindow.get() == nullptr || focusedWindow.get() == nullptr) {
							secondIsActive = true;
							return false;
						}
						return true;
					});

					if(secondIsActive) {
						std::get<1>(children_) = nullptr;
						std::get<1>(typedChildren_).clear();
						if(focusedWindow.get() == nullptr)
							ascension::viewer::widgetapi::setFocus(*std::get<0>(children_));
						updateChildrenPlacement();
						return;
					}
				}

				std::get<0>(typedChildren_) = std::move(std::get<1>(typedChildren_));
				std::get<0>(children_) = std::move(std::get<1>(children_));
				assert(std::get<1>(children_) == nullptr);
				assert(std::get<1>(typedChildren_).empty());
				updateChildrenPlacement();
			}
		}

		/**
		 * Handles @c WM_SETCURSOR window message.
		 * @param window A handle to the window that contains the cursor
		 * @param hit The hit-test code
		 * @param message The identifier of the mouse message
		 * @param[out] consumed
		 */
		void PanedWidget::onSetCursor(HWND, UINT hit, UINT, bool& consumed) {
			if(consumed = (numberOfChildren() == 2 && hit == HTCLIENT))
				::SetCursor(static_cast<HCURSOR>(::LoadImageW(nullptr, isHorizontal() ? IDC_SIZEWE : IDC_SIZENS, IMAGE_CURSOR, LR_DEFAULTSIZE, LR_DEFAULTSIZE, LR_DEFAULTCOLOR | LR_SHARED)));
		}

		/**
		 * Handles @c WM_SIZE window message.
		 * @param type The type of resizing requested
		 * @param width The new width of the client area in user units
		 * @param height The new height of the client area in user units
		 * @param[out] consumed
		 */
		void PanedWidget::onSize(UINT, int, int, bool& consumed) {
			const ascension::win32::Window* widget = this;
			while(widget != nullptr) {
				if(ascension::viewer::widgetapi::isMinimized(*widget))
					return;	// ignore if the window is minimized
				widget = ascension::viewer::widgetapi::parentWidget(*widget);
			}
			updateChildrenPlacement();
		}

		/// @see ascension#win32#CustomControl#processMessage
		LRESULT PanedWidget::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
			switch(message) {
				case WM_SETCURSOR:
					onSetCursor(reinterpret_cast<HWND>(wp), LOWORD(lp), HIWORD(lp), consumed);
					if(consumed)
						return TRUE;
					break;
				case WM_SIZE:
					onSize(wp, LOWORD(lp), HIWORD(lp), consumed);
					if(consumed)
						return 0l;
					break;
			}
			return ascension::win32::CustomControl<PanedWidget>::processMessage(message, wp, lp, consumed);
		}

		/**
		 * Sets the gap.
		 * @param newGap The gap to set in user units
		 * @see #gap
		 */
		void PanedWidget::setGap(unsigned int newGap) BOOST_NOEXCEPT {
			if(newGap != gap()) {
				gap_ = newGap;
				updateChildrenPlacement();
			}
		}

		/**
		 * Sets the orientation to horizontal.
		 * @see #isHorizontal, #setVertical
		 */
		void PanedWidget::setHorizontal() BOOST_NOEXCEPT {
			if(!isHorizontal()) {
				horizontal_ = true;
				updateChildrenPlacement();
			}
		}

		/**
		 * Sets the orientation to vertical.
		 * @see #isVertical, #setHorizontal
		 */
		void PanedWidget::setVertical() BOOST_NOEXCEPT {
			if(!isVertical()) {
				horizontal_ = false;
				updateChildrenPlacement();
			}
		}

		/// @internal Updates placement of the child widgets.
		void PanedWidget::updateChildrenPlacement() {
			const auto n = numberOfChildren();
			if(n == 0)
				return;
			const auto thisBounds(ascension::viewer::widgetapi::bounds(*this, false));
			if(n == 1) {
				auto& child = hasChild<0>() ? *std::get<0>(children_) : *std::get<1>(children_);
				ascension::viewer::widgetapi::setBounds(child, thisBounds);
				return;
			}
			assert(n == 2);

			const ascension::graphics::Scalar firstChildSize = (isHorizontal() ? ascension::graphics::geometry::dx(thisBounds) : ascension::graphics::geometry::dy(thisBounds)) * firstChildSizeRatio_;
			ascension::NumericRange<ascension::graphics::Scalar> xrange[2], yrange[2];
			if(isHorizontal()) {
				const auto styles = ascension::win32::getWindowLong(handle().get(), GWL_EXSTYLE);
				bool rtl = (styles & WS_EX_LAYOUTRTL) != 0;
				rtl = (styles & WS_EX_RTLREADING) != 0 ? !rtl : rtl;
				xrange[0] = ascension::nrange(ascension::graphics::geometry::left(thisBounds), ascension::graphics::geometry::left(thisBounds) + firstChildSize);
				xrange[1] = ascension::nrange(*boost::const_begin(xrange[0]) + gap(), ascension::graphics::geometry::right(thisBounds));
				if(rtl)
					std::swap(xrange[0], xrange[1]);
				yrange[0] = yrange[1] = ascension::graphics::geometry::range<1>(thisBounds);
			} else {
				xrange[0] = xrange[1] = ascension::graphics::geometry::range<0>(thisBounds);
				yrange[0] = ascension::nrange(ascension::graphics::geometry::top(thisBounds), ascension::graphics::geometry::top(thisBounds) + firstChildSize);
				yrange[1] = ascension::nrange(*boost::const_end(yrange[0]) + gap(), ascension::graphics::geometry::bottom(thisBounds));
			}
			ascension::viewer::widgetapi::setBounds(*std::get<0>(children_), ascension::graphics::geometry::make<ascension::graphics::Rectangle>(std::make_pair(xrange[0], yrange[0])));
			ascension::viewer::widgetapi::setBounds(*std::get<1>(children_), ascension::graphics::geometry::make<ascension::graphics::Rectangle>(std::make_pair(xrange[1], yrange[1])));
		}

		/// @see ascension#win32#CustomControl#windowClass
		void PanedWidget::windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT {
			out.name = L"alpha.win32.PanedWidget";
			out.styles = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		}
	}
}
