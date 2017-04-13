/**
 * @file main-window-win32.cpp
 * Implements alpha#ui#MainWindow class.
 * @author exeal
 * @date 2014-06-04 Created.
 * @date 2017-01-21 Renamed from main-window.cpp.
 */

#include "ui/main-window.hpp"
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include "buffer-list.hpp"
#	include "editor-pane.hpp"
#	include "editor-view.hpp"
#	include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#	include <ascension/graphics/geometry/rectangle-range.hpp>
#	include <ascension/graphics/geometry/algorithms/make.hpp>
#	include <ascension/viewer/widgetapi/cursor.hpp>
#	include <ascension/viewer/widgetapi/widget.hpp>
#	include <boost/filesystem/operations.hpp>

namespace alpha {
	namespace ui {
		/// Default constructor.
		MainWindow::MainWindow() {
		}

		/***/
		void MainWindow::onCopyData(ascension::win32::Handle<HWND> window, const COPYDATASTRUCT& data) {}

		/// Handles @c WM_DESTROY window message.
		void MainWindow::onDestroy() {
//			killTimer(ID_TIMER_QUERYCOMMAND);
//			setMenu(nullptr);
			::PostQuitMessage(0);
		}

		/**
		 * Handles @c WM_DRAWITEM window message.
		 * @param id The identifier of the control that sent the @c WM_DRAWITEM message
		 * @param item The item to be drawn and the type of drawing required
		 */
		void MainWindow::onDrawItem(UINT, const DRAWITEMSTRUCT& item) {
			if(item.CtlType == ODT_MENU) {
				// TODO: Not implemented.
			}
		}

		/**
		 * Handles @c WM_DROPFILES window message.
		 * @param droppedFiles A handle to an internal structure describing the dropped files
		 */
		void MainWindow::onDropFiles(ascension::win32::Handle<HDROP> droppedFiles) {
			if(droppedFiles.get() != nullptr) {
				const auto n = ::DragQueryFileW(droppedFiles.get(), 0xffffffffu, nullptr, 0);
				if(n != 0) {
					decltype(n) fileNameLength = MAX_PATH;
					std::unique_ptr<WCHAR[]> fileName(new WCHAR[fileNameLength]);
					for(std::decay<decltype(n)>::type i = 0; i < n; ++i) {
						auto c = ::DragQueryFileW(droppedFiles.get(), i, nullptr, 0);
						if(c != 0) {
							if(c > fileNameLength)
								fileName.reset(new WCHAR[fileNameLength]);
							if((c = ::DragQueryFileW(droppedFiles.get(), i, fileName.get(), fileNameLength)) != 0) {
								if(!boost::filesystem::is_directory(fileName.get()))
									BufferList::instance().addNew(fileName.get());
								else
									BufferList::instance().addNewDialog(fileName.get());
							}
						}
					}
				}
				::DragFinish(droppedFiles.get());

				auto& activeView = editorPanes().activePane().selectedView();
				if(ascension::viewer::widgetapi::isRealized(activeView))
					ascension::viewer::widgetapi::setFocus(activeView);
			}
		}

		/**
		 * Handles @c WM_ENTERMENULOOP window message.
		 * @param byTrackPopupMenu Indicates whether the window menu was entered using the @c TrackPopupMenu function
		 */
		void MainWindow::onEnterMenuLoop(bool byTrackPopupMenu) {
			statusBar().setSimple(true);
		}

		/**
		 * Handles @c WM_EXITMENULOOP window message.
		 * @param shortcutMenu Indicates whether the menu is a shortcut menu
		 */
		void MainWindow::onExitMenuLoop(bool shortcutMenu) {
			statusBar().setSimple(false);
		}

		/**
		 * Handles @c WM_MEASUREITEM window message.
		 * @param id The identifier of the control that sent the @c WM_MEASUREITEM message
		 * @param item The dimension of the owner-drawn control or menu item
		 */
		void MainWindow::onMeasureItem(UINT, MEASUREITEMSTRUCT& item) {
			if(item.CtlType == ODT_MENU) {
				// TODO: Not implemented.
			}
		}

		/**
		 * Handles @c WM_MENUCHAR window message.
		 * @param c The character code that corresponds	to the key the user pressed
		 * @param type The active menu type
		 * @param menu A handle to the active menu
		 * @return
		 */
		LRESULT MainWindow::onMenuChar(WCHAR c, UINT type, ascension::win32::Handle<HMENU> menu) {
			// TODO: Not implemented.
			return 0;
		}

		/**
		 * Handles @c WM_NOTIFY window message.
		 * @param id The identifier of the common control sending the message
		 * @param nmhdr An @c NMHDR structure that contains the notification code and additional information
		 * @param[out] consumed
		 */
		void MainWindow::onNotify(UINT_PTR id, NMHDR& nmhdr, bool& consumed) {
#if 0
			if(id == IDC_BUFFERBAR)
				return BufferList::instance().handleBufferBarNotification(*reinterpret_cast<NMTOOLBARW*>(&nmhdr), consumed);
			else if(id == IDC_BUFFERBARPAGER)
				return BufferList::instance().handleBufferBarPagerNotification(nmhdr, consumed);

			switch(nmhdr.code) {
				case RBN_HEIGHTCHANGE:
					return (consumed = true), onSize(0, -1, -1);
				case RBN_CHEVRONPUSHED:
					return (consumed = true), onRebarChevronPushed(*reinterpret_cast<LPNMREBARCHEVRON>(&nmhdr));
				case TBN_DROPDOWN:
					break;
				case TBN_GETOBJECT:
					onCommand(reinterpret_cast<LPNMOBJECTNOTIFY>(&nmhdr)->iItem, 0, nullptr);
				case TTN_GETDISPINFOW:
					break;
				case TBN_HOTITEMCHANGE:
					break;
			}
#endif
		}

		/// @internal
		void MainWindow::onRebarChevronPushed(const NMREBARCHEVRON& nmRebarChevron) {
			// TODO: Not implemented.
		}

		/**
		 * Handles @c WM_SETCURSOR window message.
		 * @param window A handle to the window that contains the cursor
		 * @param hitTest The hit-test code
		 * @param message The identifier of the mouse message
		 * @param[out] consumed
		 */
		void MainWindow::onSetCursor(ascension::win32::Handle<HWND> window, UINT hitTest, UINT message, bool& consumed) {
			// TODO: This code supports only horizontal layout window.
			const auto y(boost::geometry::get<1>(ascension::viewer::widgetapi::Cursor::position(*this)));
			const auto bottom(*boost::const_end(ascension::graphics::geometry::range<1>(ascension::viewer::widgetapi::bounds(*this, false))));
			ascension::graphics::Scalar statusBarSize;
			if(ascension::viewer::widgetapi::isVisible(statusBar()))
				statusBarSize = boost::size(ascension::graphics::geometry::range<1>(ascension::viewer::widgetapi::bounds(statusBar(), true)));
			else
				statusBarSize = 0;

			if(consumed = (y >= bottom - statusBarSize - 3 && y <= bottom - statusBarSize))
				::SetCursor(static_cast<HCURSOR>(::LoadImageW(nullptr, IDC_SIZENS, IMAGE_CURSOR, LR_DEFAULTSIZE, LR_DEFAULTSIZE, LR_DEFAULTCOLOR | LR_SHARED)));
		}

		/**
		 * Handles @c WM_SETFOCUS window message.
		 * @param oldWindow A handle to the window that has lost the keyboard focus.
		 */
		void MainWindow::onSetFocus(ascension::win32::Handle<HWND>) {
			ascension::viewer::widgetapi::setFocus(editorPanes());
		}

		/**
		 * Handles @c WM_SIZE window message.
		 * @param type The type of resizing requested
		 * @param width The new width of the client area
		 * @param height The new height of the client area
		 */
		void MainWindow::onSize(UINT type, int width, int height) {
			if(width != -1 && height != -1) {
				const auto temp(ascension::viewer::widgetapi::bounds(*this, false));
				width = static_cast<int>(ascension::graphics::geometry::dx(temp));
				height = static_cast<int>(ascension::graphics::geometry::dy(temp));
			}

			ascension::graphics::Rectangle statusBarBounds;
			if(ascension::viewer::widgetapi::isVisible(statusBar())) {
				::SendMessageW(statusBar().handle().get(), WM_SIZE, type, MAKELPARAM(width, height));
				statusBarBounds = ascension::viewer::widgetapi::bounds(statusBar(), true);
//				statusBar().adjustPaneWidths();
			} else
				boost::geometry::assign_zero(statusBarBounds);

			ascension::graphics::Rectangle rebarBounds;
#if 0
			if(ascension::viewer::widgetapi::isVisible(rebar())) {
				::SendMessageW(rebar().handle().get(), WM_SIZE, type, MAKELPARAM(width, height));
				statusBarBounds = ascension::viewer::widgetapi::bounds(rebar(), true);
//				toolbar_.sendMessage(WM_SIZE, type, MAKELPARAM(width, ascension::geometry::dy(rebarBounds) - 2));
			} else
#endif
				boost::geometry::assign_zero(rebarBounds);

			if(ascension::viewer::widgetapi::isRealized(editorPanes())) {
				const auto editorBounds(ascension::graphics::geometry::make<ascension::graphics::Rectangle>((
					ascension::graphics::geometry::_left = static_cast<ascension::graphics::Scalar>(0),
					ascension::graphics::geometry::_top = ascension::graphics::geometry::dy(rebarBounds),
					ascension::graphics::geometry::_right = static_cast<ascension::graphics::Scalar>(width),
					ascension::graphics::geometry::_bottom = height - ascension::graphics::geometry::dy(statusBarBounds))));
				ascension::viewer::widgetapi::setBounds(editorPanes(), editorBounds);
			}
		}

		/**
		 * Handles @c WM_TIMER window message.
		 * @param timerID The timer identifier
		 * @param procedure A pointer to an application-defined callback function
		 */
		void MainWindow::onTimer(UINT_PTR timerID, TIMERPROC procedure) {
			// TODO: Not implemented.
		}

		/// @see ascension#win32#CustomControl#processMessage
		LRESULT MainWindow::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
			switch(message) {
				case WM_COPYDATA:
					if(lp != 0l) {
						onCopyData(ascension::win32::borrowed(reinterpret_cast<HWND>(wp)), *reinterpret_cast<const COPYDATASTRUCT*>(lp));
						return (consumed = true), TRUE;
					}
					break;
				case WM_DESTROY:
					onDestroy();
					return (consumed = true), 0;
				case WM_DRAWITEM:
					onDrawItem(wp, *reinterpret_cast<const DRAWITEMSTRUCT*>(lp));
					return (consumed = true), TRUE;
				case WM_DROPFILES:
					onDropFiles(ascension::win32::borrowed(reinterpret_cast<HDROP>(wp)));
					return (consumed = true), 0;
				case WM_ENTERMENULOOP:
					onEnterMenuLoop(ascension::win32::boole(wp));
					return (consumed = true), 0;
				case WM_EXITMENULOOP:
					onExitMenuLoop(ascension::win32::boole(wp));
					return (consumed = true), 0;
				case WM_MEASUREITEM:
					onMeasureItem(wp, *reinterpret_cast<MEASUREITEMSTRUCT*>(lp));
					return (consumed = true), TRUE;
				case WM_MENUCHAR:
					return (consumed = true), onMenuChar(LOWORD(wp), HIWORD(wp), ascension::win32::borrowed(reinterpret_cast<HMENU>(lp)));
				case WM_SETCURSOR:
					onSetCursor(ascension::win32::borrowed(reinterpret_cast<HWND>(wp)), LOWORD(lp), HIWORD(lp), consumed);
					if(consumed)
						return TRUE;
					break;
				case WM_SETFOCUS:
					onSetFocus(ascension::win32::borrowed(reinterpret_cast<HWND>(wp)));
					return (consumed = true), 0;
				case WM_SIZE:
					onSize(wp, LOWORD(lp), HIWORD(lp));
					return (consumed = true), 0;
				case WM_TIMER:
					onTimer(wp, reinterpret_cast<TIMERPROC>(lp));
					return (consumed = true), 0;
			}

			return ascension::win32::CustomControl<MainWindow>::processMessage(message, wp, lp, consumed);
		}

		/// @see ascension#win32#CustomControl#realized
		void MainWindow::realized(const Type& type) {
			CustomControl<MainWindow>::realized(type);
			ascension::win32::realize(editorPanes(), ascension::win32::Window::Type::widget(handle()));
			statusBar_.reset(new StatusBar(ascension::win32::Window::Type::widget(handle())));
			bufferSelectionChangedConnection_ = editorPanes_.bufferSelectionChangedSignal().connect([this](EditorPanes&) {
				this->updateTitle();
			});
			ascension::viewer::widgetapi::show(editorPanes());
		}

		/// Updates the text string of the title bar.
		void MainWindow::updateTitle() {
//			if(isWindow()) {
				// show the display name of the selected buffer and application credit
				static PlatformString titleCache;
				const auto& buffer = editorPanes().selectedBuffer();
#if 0
				PlatformString title(BufferList::instance().displayName(buffer));
#else
				PlatformString title(buffer.name());
#endif
				if(title != titleCache) {
					titleCache = title;
					title += L" - ";
//					title += ALPHA_APPLICATION_FULL_NAME;
					title += L"Alpha";
					::SetWindowTextW(handle().get(), title.c_str());
				}
//			}
		}

		/// @see ascension#win32#CustomControl#windowClass
		void MainWindow::windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT {
			out.name = L"alpha.MainWindow";
			out.styles = CS_HREDRAW | CS_VREDRAW;
		}
	}
}

#endif
