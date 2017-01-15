/**
 * @file auto-scroll-origin-mark.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-10-04 separated from viewer.cpp
 * @date 2011-2014
 * @date 2014-08-23 Separated from default-mouse-input-strategy.cpp
 */

#include <ascension/graphics/image.hpp>
#include <ascension/graphics/paint.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/geometry/native-conversions.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <ascension/viewer/caret.hpp>
//#include <ascension/viewer/default-mouse-input-strategy.hpp>
#include <ascension/viewer/text-viewer.hpp>

namespace ascension {
	namespace viewer {
		namespace {
			typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Gtk::Window
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QWidget
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				NSView
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::CustomControl
#endif
				AutoScrollOriginMarkBase;

			/// Circled window displayed at which the auto scroll started.
			class AutoScrollOriginMark : public AutoScrollOriginMarkBase {
			public:
				/// Defines the type of the cursors obtained by @c #cursorForScrolling method.
				enum CursorType {
					CURSOR_NEUTRAL,	///< Indicates no scrolling.
					CURSOR_UPWARD,	///< Indicates scrolling upward.
					CURSOR_DOWNWARD	///< Indicates scrolling downward.
				};
			public:
				explicit AutoScrollOriginMark(TextViewer& viewer) BOOST_NOEXCEPT;
				static const widgetapi::Cursor& cursorForScrolling(CursorType type);
				void resetWidgetShape();
			private:
				void paint(graphics::PaintContext& context) const;
				void paintPattern(graphics::RenderingContext2D& context) const;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				void on_realize() override {resetWidgetShape();}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
					if(message == WM_PAINT) {
						PAINTSTRUCT ps;
						::BeginPaint(handle().get(), &ps);
						graphics::RenderingContext2D temp(win32::borrowed(ps.hdc));
						paint(graphics::PaintContext(std::move(temp), fromNative<graphics::Rectangle>(ps.rcPaint)));
						::EndPaint(handle().get(), &ps);
						consumed = true;
					}
					return win32::CustomControl::processMessage(message, wp, lp, consumed);
				}
				void provideClassInformation(win32::CustomControl::ClassInformation& classInformation) const {
					classInformation.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
					classInformation.background = COLOR_WINDOW;
					classInformation.cursor = MAKEINTRESOURCEW(32513);	// IDC_IBEAM
				}
				std::basic_string<WCHAR> provideClassName() const override {return L"AutoScrollOriginMark";}
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			private:
				graphics::Scalar width_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				COLORREF maskColor_;
#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			};
		} // namespace @0

		/**
		 * Constructor.
		 * @param viewer The text viewer. The widget becomes the child of this viewer
		 */
		AutoScrollOriginMark::AutoScrollOriginMark(TextViewer& viewer) BOOST_NOEXCEPT {
//			resetWidgetShape();
			widgetapi::setParentWidget(*this, viewer);
		}

		/**
		 * Returns the cursor should be shown when the auto-scroll is active.
		 * @param type The type of the cursor to obtain
		 * @return The cursor. Do not destroy the returned value
		 * @throw UnknownValueException @a type is unknown
		 */
		const widgetapi::Cursor& AutoScrollOriginMark::cursorForScrolling(CursorType type) {
			static std::array<std::unique_ptr<widgetapi::Cursor>, 3> instances;
			if(static_cast<std::size_t>(type) >= instances.size())
				throw UnknownValueException("type");
			if(instances[type].get() == nullptr) {
				static const std::uint8_t AND_LINE_3_TO_11[] = {
					0xff, 0xfe, 0x7f, 0xff,
					0xff, 0xfc, 0x3f, 0xff,
					0xff, 0xf8, 0x1f, 0xff,
					0xff, 0xf0, 0x0f, 0xff,
					0xff, 0xe0, 0x07, 0xff,
					0xff, 0xc0, 0x03, 0xff,
					0xff, 0x80, 0x01, 0xff,
					0xff, 0x00, 0x00, 0xff,
					0xff, 0x80, 0x01, 0xff
				};
				static const std::uint8_t XOR_LINE_3_TO_11[] = {
					0x00, 0x01, 0x80, 0x00,
					0x00, 0x02, 0x40, 0x00,
					0x00, 0x04, 0x20, 0x00,
					0x00, 0x08, 0x10, 0x00,
					0x00, 0x10, 0x08, 0x00,
					0x00, 0x20, 0x04, 0x00,
					0x00, 0x40, 0x02, 0x00,
					0x00, 0x80, 0x01, 0x00,
					0x00, 0x7f, 0xfe, 0x00
				};
				static const std::uint8_t AND_LINE_13_TO_18[] = {
					0xff, 0xfe, 0x7f, 0xff,
					0xff, 0xfc, 0x3f, 0xff,
					0xff, 0xf8, 0x1f, 0xff,
					0xff, 0xf8, 0x1f, 0xff,
					0xff, 0xfc, 0x3f, 0xff,
					0xff, 0xfe, 0x7f, 0xff,
				};
				static const std::uint8_t XOR_LINE_13_TO_18[] = {
					0x00, 0x01, 0x80, 0x00,
					0x00, 0x02, 0x40, 0x00,
					0x00, 0x04, 0x20, 0x00,
					0x00, 0x04, 0x20, 0x00,
					0x00, 0x02, 0x40, 0x00,
					0x00, 0x01, 0x80, 0x00
				};
				static const std::uint8_t AND_LINE_20_TO_28[] = {
					0xff, 0x80, 0x01, 0xff,
					0xff, 0x00, 0x00, 0xff,
					0xff, 0x80, 0x01, 0xff,
					0xff, 0xc0, 0x03, 0xff,
					0xff, 0xe0, 0x07, 0xff,
					0xff, 0xf0, 0x0f, 0xff,
					0xff, 0xf8, 0x1f, 0xff,
					0xff, 0xfc, 0x3f, 0xff,
					0xff, 0xfe, 0x7f, 0xff
				};
				static const std::uint8_t XOR_LINE_20_TO_28[] = {
					0x00, 0x7f, 0xfe, 0x00,
					0x00, 0x80, 0x01, 0x00,
					0x00, 0x40, 0x02, 0x00,
					0x00, 0x20, 0x04, 0x00,
					0x00, 0x10, 0x08, 0x00,
					0x00, 0x08, 0x10, 0x00,
					0x00, 0x04, 0x20, 0x00,
					0x00, 0x02, 0x40, 0x00,
					0x00, 0x01, 0x80, 0x00
				};
				std::uint8_t andBits[4 * 32], xorBits[4 * 32];
				// fill canvases
				std::memset(andBits, 0xff, 4 * 32);
				std::memset(xorBits, 0x00, 4 * 32);
				// draw lines
				if(type == CURSOR_NEUTRAL || type == CURSOR_UPWARD) {
					std::memcpy(andBits + 4 * 3, AND_LINE_3_TO_11, sizeof(AND_LINE_3_TO_11));
					std::memcpy(xorBits + 4 * 3, XOR_LINE_3_TO_11, sizeof(XOR_LINE_3_TO_11));
				}
				std::memcpy(andBits + 4 * 13, AND_LINE_13_TO_18, sizeof(AND_LINE_13_TO_18));
				std::memcpy(xorBits + 4 * 13, XOR_LINE_13_TO_18, sizeof(XOR_LINE_13_TO_18));
				if(type == CURSOR_NEUTRAL || type == CURSOR_DOWNWARD) {
					std::memcpy(andBits + 4 * 20, AND_LINE_20_TO_28, sizeof(AND_LINE_20_TO_28));
					std::memcpy(xorBits + 4 * 20, XOR_LINE_20_TO_28, sizeof(XOR_LINE_20_TO_28));
				}
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				namespace geometry = graphics::geometry;
				instances[type] = widgetapi::Cursor::createMonochrome(
					geometry::BasicDimension<std::uint16_t>(geometry::_dx = 32, geometry::_dy = 32),
					andBits, xorBits,
					geometry::make<boost::geometry::model::d2::point_xy<std::uint16_t>>((geometry::_x = 16, geometry::_y = 16)));
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				instances[type].reset(new widgetapi::Cursor(win32::makeHandle(
					::CreateCursor(::GetModuleHandleW(nullptr), 16, 16, 32, 32, andBits, xorBits), &::DestroyCursor)));
#else
				instances[type].reset(new widgetapi::Cursor(bitmap));
#endif
			}
			return *instances[type];
		}

		/// @see Widget#paint
		void AutoScrollOriginMark::paint(graphics::PaintContext& context) const {
			paintPattern(context);
		}

		/// @internal
		void AutoScrollOriginMark::paintPattern(graphics::RenderingContext2D& context) const {
			const graphics::Color color(boost::get_optional_value_or(
				graphics::SystemColors::get(graphics::SystemColors::APP_WORKSPACE), graphics::Color::OPAQUE_BLACK));
			context.setStrokeStyle(std::make_shared<graphics::SolidColor>(color));
			context.setFillStyle(std::make_shared<graphics::SolidColor>(color));

			namespace geometry = graphics::geometry;
			using graphics::Point;
			context
				.beginPath()
				.moveTo(geometry::make<Point>((geometry::_x = 13.0f, geometry::_y = 3.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 7.0f, geometry::_y = 9.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 20.0f, geometry::_y = 9.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 14.0f, geometry::_y = 3.0f)))
				.closePath()
				.fill();
			context
				.beginPath()
				.moveTo(geometry::make<Point>((geometry::_x = 13.0f, geometry::_y = 24.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 7.0f, geometry::_y = 18.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 20.0f, geometry::_y = 18.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 14.0f, geometry::_y = 24.0f)))
				.closePath()
				.fill();
			context
				.beginPath()
				.moveTo(geometry::make<Point>((geometry::_x = 13.0f, geometry::_y = 12.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 15.0f, geometry::_y = 12.0f)))
				.stroke();
			context
				.beginPath()
				.moveTo(geometry::make<Point>((geometry::_x = 12.0f, geometry::_y = 13.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 16.0f, geometry::_y = 13.0f)))
				.stroke();
			context
				.beginPath()
				.moveTo(geometry::make<Point>((geometry::_x = 12.0f, geometry::_y = 14.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 16.0f, geometry::_y = 14.0f)))
				.stroke();
			context
				.beginPath()
				.moveTo(geometry::make<Point>((geometry::_x = 13.0f, geometry::_y = 15.0f)))
				.lineTo(geometry::make<Point>((geometry::_x = 15.0f, geometry::_y = 15.0f)))
				.stroke();
		}

		void AutoScrollOriginMark::resetWidgetShape() {
			width_ = 28;	// TODO: This value must be computed by using user settings.
			widgetapi::resize(widgetapi::window(*this), graphics::Dimension(width_ + 1, width_ + 1));
	
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			// TODO: Implement by using Gtk.Window.shape_combine_region(Cairo.Region).
			// TODO: Implement by using Gtk.Widget.shape_combine_mask(,int,int) and Gdk.Pixmap.create_cairo_context.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			// TODO: Implement by using QWidget.setMask(QBitmap).
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			// TODO: Implement by using [NSWindow setBackgroundColor:[NSColor clearColor]] and [NSWindow setOpaque:NO].
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			// calling CreateWindowExW with WS_EX_LAYERED will fail on NT 4.0
			::SetWindowLongW(handle().get(), GWL_EXSTYLE,
				::GetWindowLongW(handle().get(), GWL_EXSTYLE) | WS_EX_LAYERED);
			::SetLayeredWindowAttributes(handle().get(), maskColor_ = ::GetSysColor(COLOR_WINDOW), 0, LWA_COLORKEY);
//			win32::Handle<HRGN> rgn(::CreateEllipticRgn(0, 0, width_ + 1, width_ + 1), &::DeleteObject);
//			::SetWindowRgn(asNativeObject().get(), rgn.get(), true);
#endif
		}

		// window system-dependent implementations ////////////////////////////////////////////////////////////////////

#if defined(ASCENSION_WINDOWS_SYSTEM_GTK)
#elif defined(ASCENSION_WINDOWS_SYSTEM_QT)
#elif defined(ASCENSION_WINDOWS_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOWS_SYSTEM_WIN32)
#endif
	}
}
