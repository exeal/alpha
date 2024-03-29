/**
 * @file cursor.hpp
 * @author exeal
 * @date 2011-06-25 created
 */

#ifndef ASCENSION_CURSOR_HPP
#define ASCENSION_CURSOR_HPP
#include <ascension/corelib/native-wrappers.hpp>
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/platforms.hpp>
#include <ascension/viewer/widgetapi/widget-proxy.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/cursor.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QCursor>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <NSCursor.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif
#include <boost/optional.hpp>

namespace ascension {

	namespace graphics {
		class Image;
	}

	namespace viewer {
		namespace widgetapi {
			/// Provides a (mouse) cursor.
			class Cursor : public SharedWrapper<Cursor> {
			public:
				/// The coordinate type for images.
				typedef std::uint16_t Coordinate;
				/// Defines system-defined cursor types.
				typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					Gdk::CursorType
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					Qt::CursorShape
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
					???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					LPCWSTR
#else
					ASCENSION_CANT_DETECT_PLATFORM();
#endif
					BuiltinShape;

			public:
				/**
				 * Creates a new cursor with the specified built-in shape.
				 * @param shape The built-in cursor shape
				 */
				explicit Cursor(BuiltinShape shape);
				/**
				 * Creates a new cursor with the given image.
				 * @param shape The image which provides cursor pixels
				 * @param hotspot The hotspot of the cursor in pixels. If @c boost#none, the center of @a shape is used
				 */
				explicit Cursor(const graphics::Image& shape,
					const boost::optional<boost::geometry::model::d2::point_xy<Coordinate>>& hotspot = boost::none);
				/// Copy-constructor.
//				Cursor(const Cursor& other);
				/// Copy-assignment operator.
//				Cursor& operator=(const Cursor& other);
				/**
				 * Creates a @c Cursor from the window system-native object.
				 * @param native The native object
				 */
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				explicit Cursor(Glib::RefPtr<Gdk::Cursor> native) : native_(native) {}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				explicit Cursor(std::shared_ptr<QCursor> native) BOOST_NOEXCEPT : native_(native) {}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				explicit Cursor(std::shared_ptr<NSCursor> native) BOOST_NOEXCEPT : native_(native) {}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				explicit Cursor(win32::Handle<HCURSOR> native) BOOST_NOEXCEPT : native_(native) {}
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				/// Returns the underlying native object.
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::Cursor> native() {return native_;}
				/// Returns the underlying native object.
				Glib::RefPtr<const Gdk::Cursor> native() const {return native_;}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				std::shared_ptr<QCursor> native() BOOST_NOEXCEPT {return native_;}
				/// Returns the underlying native object.
				std::shared_ptr<const QCursor> native() const BOOST_NOEXCEPT {return native_;}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				std::shared_ptr<NSCursor> native() BOOST_NOEXCEPT {return native_;}
				/// Returns the underlying native object.
				std::shared_ptr<const NSCursor> native() const BOOST_NOEXCEPT {return native_;}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::Handle<HCURSOR> native() const BOOST_NOEXCEPT {return native_;}
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				/**
				 * Creates a new cursor with the given monochrome pixel data.
				 * @param size The size of the @a bitmap in pixels
				 * @param bitmap The byte array which defines bitmap of the cursor
				 * @param mask The byte array which defines mask data
				 * @param hotspot The hotspot of the cursor in pixels. If @c boost#none, the center of bitmap is used
				 * @return The created cursor
				 */
				static std::unique_ptr<Cursor> createMonochrome(
					const graphics::geometry::BasicDimension<Coordinate>& size,
					const std::uint8_t* bitmap, const std::uint8_t* mask,
					const boost::optional<boost::geometry::model::d2::point_xy<Coordinate>>& hotspot = boost::none);

			public:
				/// Hides the global cursor.
				static void hide();
				/// Returns the position of the global cursor in pixels.
				static graphics::Point position();
				/// Returns the position of the global cursor in pixels relative to the origin of the given @a window.
				static graphics::Point position(Proxy<const Window> window);
//				/// Moves the global cursor to the specified position in pixels.
//				static void setPosition(const graphics::Point& p);
				/// Shows the global cursors.
				static void show();

			private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::Cursor> native_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				std::shared_ptr<QCursor> native_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				std::shared_ptr<NSCursor> native_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::Handle<HCURSOR> native_;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			};
		}
	}
}

#endif // !ASCENSION_CURSOR_HPP
