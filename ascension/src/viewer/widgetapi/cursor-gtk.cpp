/**
 * @file cursor-gtk.cpp
 * Implements @c ascension#viewer#widgetapi#Cursor class on gtkmm window system.
 * @date 2014-02-01 Created.
 */

#include <ascension/viewer/widgetapi/cursor.hpp>
#include <ascension/graphics/geometry/algorithm.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#include <ascension/graphics/image.hpp>
#include <gdkmm/device.h>
#include <gdkmm/devicemanager.h>
#if ASCENSION_SUPPORTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/graphics/rendering-context.hpp>
#	include <cairomm/win32_surface.h>
#endif
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
#	include <ascension/viewer/widgetapi/screen.hpp>
#endif

namespace ascension {
	namespace viewer {
		namespace widgetapi {
			Cursor::Cursor(Cursor::BuiltinShape shape) : impl_(Gdk::Cursor::create(shape)) {
			}

			Cursor::Cursor(const graphics::Image& shape,
					const boost::optional<graphics::geometry::BasicPoint<Cursor::Coordinate>>& hotspot /* = boost::none */) {
				Cairo::RefPtr<Cairo::Surface> surface;
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(CAIRO)
				surface = shape.asNativeObject();
#elif ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
				const std::unique_ptr<graphics::RenderingContext2D> context(Screen::defaultInstance().createRenderingContext());
				const win32::Handle<HDC>::Type dc(context->native());
				HBITMAP oldBitmap = static_cast<HBITMAP>(::SelectObject(dc.get(), shape.asNative().get()));
				surface = Cairo::Win32Surface::create(context->native().get());
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
#if GTKMM_MAJOR_VERSION >= 3 && GTKMM_MINOR_VERSION >= 10
				impl_ = Gdk::Cursor::create(Gdk::Display::get_default(), surface,
					(hotspot != boost::none) ? graphics::geometry::x(*hotspot) : shape.width() / 2,
					(hotspot != boost::none) ? graphics::geometry::y(*hotspot) : shape.height() / 2);
#else
#endif

#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI)
				::SelectObject(dc.get(), oldBitmap);
#endif
			}

			Cursor::Cursor(const Cursor& other) {
				const Glib::RefPtr<const Gdk::Cursor> native(other.asNativeObject());
				const Gdk::CursorType type = native->get_cursor_type();
				if(type != Gdk::CURSOR_IS_PIXMAP)
					impl_ = Gdk::Cursor::create(type);
				else {
#if GTKMM_MAJOR_VERSION >= 3 && GTKMM_MINOR_VERSION >= 10
					// TODO: Write code using Cairo.Surface.
#else
					impl_ = Gdk::Cursor::create(const_cast<Cursor&>(other).asNativeObject()->get_display(), other.impl_->get_image()->copy(), -1, -1);
#endif
				}
			}

			std::unique_ptr<Cursor> Cursor::createMonochrome(
					const graphics::geometry::BasicDimension<Cursor::Coordinate>& size,
					const std::uint8_t* bitmap, const std::uint8_t* mask,
					const boost::optional<graphics::geometry::BasicPoint<Cursor::Coordinate>>& hotspot /* = boost::none */) {
				if(bitmap == nullptr)
					throw NullPointerException("bitmap");
				else if(mask == nullptr)
					throw NullPointerException("mask");

				const std::uint32_t stride = graphics::Image::stride(graphics::geometry::dx(size), graphics::Image::ARGB32);
				const std::uint32_t dy = graphics::geometry::dy(size);
				std::unique_ptr<std::uint8_t[]> bits(new std::uint8_t[stride * dy]);

				for(std::size_t x = 0, byteOffset = 0, bitOffset = 0; x < stride; x += 4) {
					for(std::size_t y = 0; y < dy; ++y) {
						std::uint32_t& pixel = *reinterpret_cast<std::uint32_t*>(bits[y * stride + x]);
						const std::uint8_t bitMask = 1 << bitOffset;
						pixel = ((bitmap[byteOffset] & bitMask) != 0) ? 0 : 0xffffffu;
						pixel |= ((mask[byteOffset] & bitMask) != 0) ? 0 : 0xff000000u;

						if(++bitOffset == 8) {
							bitOffset = 0;
							++byteOffset;
						}
					}
				}
				return std::unique_ptr<Cursor>(
					new Cursor(
						graphics::Image(
							std::move(bits),
							static_cast<graphics::geometry::BasicDimension<std::uint32_t>>(size),
							graphics::Image::ARGB32), hotspot));
			}

			void Cursor::hide() {
				// TODO: Not implemented.
			}

			graphics::Point Cursor::position() {
				if(const Glib::RefPtr<const Gdk::Display> defaultDisplay = Gdk::Display::get_default()) {
					if(const Glib::RefPtr<const Gdk::DeviceManager> deviceManager = defaultDisplay->get_device_manager()) {
						if(const Glib::RefPtr<const Gdk::Device> clientPointer = deviceManager->get_client_pointer()) {
							int x, y;
							clientPointer->get_position(x, y);
							return graphics::geometry::make<graphics::Point>((graphics::geometry::_x = x, graphics::geometry::_y = y));
						}
					}
				}
				throw makePlatformError();
			}

			void Cursor::show() {
				// TODO: Not implemented.
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
