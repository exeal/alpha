/**
 * @file caret-blinker.cpp
 * Implements @c CaretBlinker internal class.
 * @date 2014-01-29 Created.
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-viewer.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/settings.h>
#endif

namespace ascension {
	namespace viewer {
		namespace {
			inline boost::optional<unsigned int> systemBlinkTimeInMilliseconds(TextViewer& viewer) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#ifndef GTKMM_DISABLE_DEPRECATED
				const Glib::RefPtr<const Gtk::Settings> settings(viewer.get_settings());
				if(settings->property_gtk_cursor_blink().get_value())
					return settings->property_gtk_cursor_blink_time().get_value();
#endif // !GTKMM_DISABLE_DEPRECATED
				return boost::none;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				const UINT ms = ::GetCaretBlinkTime();
				if(ms == 0)
					throw makePlatformError();
				return (ms != INFINITE) ? ms : boost::none;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			}
		}

		/**
		 * Constructor.
		 * @param viewer The text viewer this object is associated with
		 */
		TextViewer::CaretBlinker::CaretBlinker(TextViewer& viewer) : viewer_(viewer) {
		}

		/// Pends blinking of the caret(s).
		void TextViewer::CaretBlinker::pend() {
			if(widgetapi::hasFocus(viewer_)) {
				stop();
				setVisible(true);
				if(const boost::optional<unsigned int> blinkTime = systemBlinkTimeInMilliseconds(viewer_))
					timer_.start(*blinkTime, *this);
			}
		}

		inline void TextViewer::CaretBlinker::setVisible(bool visible) {
			if(visible == visible_)
				return;
			visible_ = visible;
			viewer_.redrawLine(kernel::line(viewer_.caret()));	// TODO: This code is not efficient.
		}

		/// Stops blinking of the caret(s).
		void TextViewer::CaretBlinker::stop() {
			timer_.stop();
		}

		/// @see HasTimer#timeElapsed
		void TextViewer::CaretBlinker::timeElapsed(Timer&) {
			if(!widgetapi::hasFocus(viewer_)) {
				timer_.stop();
				update();
				return;
			}

			setVisible(!visible_);
		}

		/// Checks and updates state of blinking of the caret.
		void TextViewer::CaretBlinker::update() {
			if(widgetapi::hasFocus(viewer_)) {
				if(const boost::optional<unsigned int> blinkTime = systemBlinkTimeInMilliseconds(viewer_)) {
					if(!timer_.isActive()) {
						setVisible(true);
						timer_.start(*blinkTime / 2, *this);
					}
				} else {
					stop();
					setVisible(true);
				}
			} else {
				stop();
				setVisible(false);
			}
		}
	}
}
