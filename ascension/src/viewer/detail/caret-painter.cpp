/**
 * @file caret-painter.cpp
 * Implements @c CaretPainter internal class.
 * @date 2014-01-29 Created.
 * @date 2015-11-22 Moved from ascension/src/viewer.
 * @date 2015-11-23 Renamed from caret-blinker.cpp.
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/detail/caret-painter.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK) && !defined(GTKMM_DISABLE_DEPRECATED)
#	include <gtkmm/settings.h>
#endif

namespace ascension {
	namespace viewer {
		namespace detail {
			namespace {
				const int BLINK_RATE_DIVIDER = 3;
				const int BLINK_RATE_PENDING_MULTIPLIER = BLINK_RATE_DIVIDER;
				const int BLINK_RATE_SHOWING_MULTIPLIER = 2;
				const int BLINK_RATE_HIDING_MULTIPLIER = BLINK_RATE_PENDING_MULTIPLIER - BLINK_RATE_SHOWING_MULTIPLIER;

				inline bool isCaretBlinkable(const Caret& caret) {
					// TODO: Check if the text viewer is also editable.
					return widgetapi::hasFocus(caret.textArea().textViewer());
				}

				inline boost::optional<boost::chrono::milliseconds> systemBlinkTime(const Caret& caret) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#ifndef GTKMM_DISABLE_DEPRECATED
					const Glib::RefPtr<const Gtk::Settings> settings(caret.textArea().textViewer().get_settings());
					if(settings->property_gtk_cursor_blink().get_value())
						return boost::chrono::milliseconds(settings->property_gtk_cursor_blink_time().get_value());
					return boost::none;
#else
					return boost::chrono::milliseconds(1200);	// CURSOR_BLINK_TIME
#endif // !GTKMM_DISABLE_DEPRECATED
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

				inline boost::optional<boost::chrono::milliseconds> systemBlinkTimeout(const Caret& caret) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#ifndef GTKMM_DISABLE_DEPRECATED
					const Glib::RefPtr<const Gtk::Settings> settings(caret.textArea().textViewer().get_settings());
					if(settings->property_gtk_cursor_blink_timeout().get_value()) {
						const int seconds = settings->property_gtk_cursor_blink_time().get_value();
						if(seconds > 0)
							return boost::chrono::seconds(seconds);
					}
					return boost::none;
#else
					return boost::chrono::seconds(10);	// CURSOR_BLINK_TIMEOUT_SEC
#endif // !GTKMM_DISABLE_DEPRECATED
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					return boost::none;
#else
					ASCENSION_CANT_DETECT_PLATFORM();
#endif
				}
			}

			/**
			 * Constructor.
			 * @param caret The caret this object is associated with
			 */
			CaretPainter::CaretPainter(Caret& caret) : caret_(caret) {
				caretMotionConnection_ = caret_.motionSignal().connect([this](const Caret& caret, const kernel::Region&) {
					if(&caret == &this->caret_ && this->shows() && widgetapi::isVisible(this->caret_.textArea().textViewer())) {
						this->resetTimer();
						this->pend();
					}
				});

				viewerFocusChangedConnection_ = caret_.textArea().textViewer().focusChangedSignal().connect([this](const TextViewer& viewer) {
					if(&viewer == &this->caret_.textArea().textViewer()/* && !viewer.isFrozen()*/) {
						this->resetTimer();
						if(widgetapi::hasFocus(viewer))
							this->update();
					}
				});

				update();
			}

			/// Hides the cursor.
			void CaretPainter::hide() {
				if(visible_ != boost::none) {
					timer_.stop();
					setVisible(false);
					visible_ = boost::none;
				}
			}

			/// Pends blinking of the caret(s).
			void CaretPainter::pend() {
				if(isCaretBlinkable(caret_)) {
					if(const boost::optional<boost::chrono::milliseconds> interval = systemBlinkTime(caret_)) {
						timer_.stop();
						timer_.start(boost::get(interval) * BLINK_RATE_PENDING_MULTIPLIER / BLINK_RATE_DIVIDER, *this);
						setVisible(true);
					}
				}
			}

			void CaretPainter::resetTimer() {
				elapsedTimeFromLastUserInput_ = boost::chrono::milliseconds::zero();
			}

			inline void CaretPainter::setVisible(bool visible) {
				if(visible == visible_)
					return;
				visible_ = visible;
				caret_.textArea().redrawLine(kernel::line(caret_));	// TODO: This code is not efficient.
			}

			/// Shows and begins blinking the caret.
			void CaretPainter::show() {
				resetTimer();
				if(widgetapi::hasFocus(caret_.textArea().textViewer()))
					update();
			}

			/// @see HasTimer#timeElapsed
			void CaretPainter::timeElapsed(Timer<>&) {
				timer_.stop();
				const auto interval(systemBlinkTime(caret_));
				if(interval == boost::none || !widgetapi::hasFocus(caret_.textArea().textViewer())) {
					update();
					return;
				}

				const auto timeout(systemBlinkTimeout(caret_));
				if(timeout != boost::none && elapsedTimeFromLastUserInput_ > boost::get(timeout)) {
					// stop blinking
					setVisible(true);
				} else if(isVisible()) {
					setVisible(false);
					timer_.start(boost::get(interval) * BLINK_RATE_HIDING_MULTIPLIER / BLINK_RATE_DIVIDER, *this);
				} else {
					setVisible(true);
					elapsedTimeFromLastUserInput_ += boost::get(interval);
					timer_.start(boost::get(interval) * BLINK_RATE_SHOWING_MULTIPLIER / BLINK_RATE_DIVIDER, *this);
				}
			}

			/// Checks and updates state of blinking of the caret.
			void CaretPainter::update() {
				if(isCaretBlinkable(caret_)) {
					if(const boost::optional<boost::chrono::milliseconds> interval = systemBlinkTime(caret_)) {
						if(!timer_.isActive()) {
							setVisible(true);
							timer_.start(boost::get(interval) * BLINK_RATE_SHOWING_MULTIPLIER / BLINK_RATE_DIVIDER, *this);
							return;
						}
					}
				}
				
				timer_.stop();
				visible_ = true;
			}
		}
	}
}
