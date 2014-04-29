/**
 * @file cursor.hpp
 * @author exeal
 * @date 2011-06-25 created
 */

#ifndef ASCENSION_CURSOR_HPP
#define ASCENSION_CURSOR_HPP

#include <ascension/graphics/geometry.hpp>
#include <ascension/platforms.hpp>
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

	namespace viewers {
		namespace widgetapi {
			/// Provides a (mouse) cursor.
			class Cursor {
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
					const boost::optional<graphics::geometry::BasicPoint<Coordinate>>& hotspot = boost::none);
				/// Copy-constructor.
				Cursor(const Cursor& other);
				/// Copy-assignment operator.
				Cursor& operator=(const Cursor& other);
				/// Returns the underlying native object.
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::Cursor> asNativeObject() BOOST_NOEXCEPT {return impl_;}
				/// Returns the underlying native object.
				Glib::RefPtr<const Gdk::Cursor>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				const QCursor&
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				NSCursor???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
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
//				/// Moves the global cursor to the specified position in pixels.
//				static void setPosition(const graphics::Point& p);
				/// Shows the global cursors.
				static void show();

			private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::Cursor>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QCursor
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				NSCursor
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
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
