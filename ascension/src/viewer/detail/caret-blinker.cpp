/**
 * @file caret-blinker.cpp
 * Implements @c CaretBlinker internal class.
 * @date 2014-01-29 Created.
 * @date 2015-11-22 Moved from ascension/src/viewer.
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/settings.h>
#endif

namespace ascension {
	namespace viewer {
		namespace detail {
			namespace {
				inline boost::optional<boost::chrono::milliseconds> systemBlinkTime(const Caret& caret) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#ifndef GTKMM_DISABLE_DEPRECATED
					const Glib::RefPtr<const Gtk::Settings> settings(caret.textArea().textViewer().get_settings());
					if(settings->property_gtk_cursor_blink().get_value())
						return boost::chrono::milliseconds(settings->property_gtk_cursor_blink_time().get_value());
#endif // !GTKMM_DISABLE_DEPRECATED
					return boost::none;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					const UINT ms = ::GetCaretBlinkTime();
					if(ms == 0)
						throw makePlatformError();
					return (ms != INFINITE) ? boost::chrono::milliseconds(ms) : boost::none;
#else
					ASCENSION_CANT_DETECT_PLATFORM();
#endif
				}
			}

			/**
			 * Constructor.
			 * @param caret The caret this object is associated with
			 */
			CaretBlinker::CaretBlinker(Caret& caret) : caret_(caret) {
			}

			/// Pends blinking of the caret(s).
			void CaretBlinker::pend() {
				if(widgetapi::hasFocus(caret_.textArea().textViewer())) {
					stop();
					setVisible(true);
					if(const boost::optional<boost::chrono::milliseconds> blinkTime = systemBlinkTime(caret_))
						timer_.start(boost::get(blinkTime), *this);
				}
			}

			inline void CaretBlinker::setVisible(bool visible) {
				if(visible == visible_)
					return;
				visible_ = visible;
				caret_.textArea().redrawLine(kernel::line(caret_));	// TODO: This code is not efficient.
			}

			/// Stops blinking of the caret(s).
			void CaretBlinker::stop() {
				timer_.stop();
			}

			/// @see HasTimer#timeElapsed
			void CaretBlinker::timeElapsed(Timer<>&) {
				if(!widgetapi::hasFocus(caret_.textArea().textViewer())) {
					timer_.stop();
					update();
					return;
				}

				setVisible(!visible_);
			}

			/// Checks and updates state of blinking of the caret.
			void CaretBlinker::update() {
				if(widgetapi::hasFocus(caret_.textArea().textViewer())) {
					if(const boost::optional<boost::chrono::milliseconds> blinkTime = systemBlinkTime(caret_)) {
						if(!timer_.isActive()) {
							setVisible(true);
							timer_.start(boost::get(blinkTime) / 2, *this);
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
}
