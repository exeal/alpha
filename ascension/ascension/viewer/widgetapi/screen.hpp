/**
 * @file screen.hpp
 * @author exeal
 * @date 2010-2011
 * @date 2011-06-20
 * @date 2014-05-27 Separated from graphics/rendering-device.hpp
 */

#ifndef ASCENSION_SCREEN_HPP
#define ASCENSION_SCREEN_HPP

#include <ascension/graphics/rendering-device.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/screen.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QDesktopWidget>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <NSScreen.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/windows.hpp>
#endif

namespace ascension {	
	namespace viewer {
		namespace widgetapi {
			class Screen : public graphics::RenderingDevice {
			public:
				static Screen& defaultInstance();

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				explicit Screen(Glib::RefPtr<Gdk::Screen> nativeObject);
				Glib::RefPtr<Gdk::Screen> native();
				Glib::RefPtr<const Gdk::Screen> native() const;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				explicit Screen(QDesktopWidget& nativeObject);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#endif

				// graphics.RenderingDevice
				std::unique_ptr<graphics::RenderingContext2D> createRenderingContext() const override;
				std::uint8_t depth() const override;
				std::uint32_t height() const override;
				graphics::Scalar heightInMillimeters() const override;
				std::uint16_t logicalDpiX() const override;
				std::uint16_t logicalDpiY() const override;
				std::uint32_t numberOfColors() const override;
				std::uint16_t physicalDpiX() const override;
				std::uint16_t physicalDpiY() const override;
				std::uint32_t width() const override;
				graphics::Scalar widthInMillimeters() const override;

			private:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::Screen> nativeObject_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QDesktopWidget& nativeObject_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#endif
			};
		}
	}
}

#endif // !ASCENSION_SCREEN_HPP
