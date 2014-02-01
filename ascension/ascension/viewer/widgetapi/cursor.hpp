/**
 * @file cursor.hpp
 * @author exeal
 * @date 2011-06-25 created
 */

#ifndef ASCENSION_CURSOR_HPP
#define ASCENSION_CURSOR_HPP

#include <ascension/graphics/geometry.hpp>
#include <ascension/platforms.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gdkmm/cursor.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QCursor>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#	include <NSCursor.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/windows.hpp>
#endif
#include <boost/optional.hpp>

namespace ascension {

	namespace graphics {
		class Image;
	}

	namespace viewers {
		namespace widgetapi {
			/// Provides a (mouse) cursor.
			class Cursor {
			public:
				/// The coordinate type for images.
				typedef std::uint16_t Coordinate;
				/// Defines system-defined cursor types.
				typedef
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
					Gdk::CursorType
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
					Qt::CursorShape
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
					???
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
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
					const boost::optional<graphics::geometry::BasicPoint<Coordinate>>& hotspot = boost::none);
				/// Copy-constructor.
				Cursor(const Cursor& other);
				/// Copy-assignment operator.
				Cursor& operator=(const Cursor& other);
				/// Returns the underlying native object.
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Glib::RefPtr<const Gdk::Cursor>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				const QCursor&
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				NSCursor???
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				win32::Handle<HCURSOR>::Type
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
					asNativeObject() const BOOST_NOEXCEPT {return impl_;}
				/**
				 * Creates a new cursor with the given monochrome pixel data.
				 * @param size The size of the @a bitmap in pixels
				 * @param bitmap The byte array which defines bitmap of the cursor
				 * @param mask The byte array which defines mask data
				 * @param hotspot The hotspot of the cursor in pixels. If @c boost#none, the center of bitmap is used
				 */
				static Cursor&& createMonochrome(
					const graphics::geometry::BasicDimension<Coordinate>& size,
					const std::uint8_t* bitmap, const std::uint8_t* mask,
					const boost::optional<graphics::geometry::BasicPoint<Coordinate>>& hotspot = boost::none);

			public:
				/// Hides the global cursor.
				static void hide();
				/// Returns the position of the global cursor in pixels.
				static graphics::Point position();
				/// Moves the global cursor to the specified position in pixels.
				static void setPosition(const graphics::Point& p);
				/// Shows the global cursors.
				static void show();

			private:
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Glib::RefPtr<const Gdk::Cursor>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				QCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
				NSCursor
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				win32::Handle<HCURSOR>::Type
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
					impl_;
			};

		}
	}
}

#endif // !ASCENSION_CURSOR_HPP
