/**
 * @file caret-painter.cpp
 * Implements @c CaretPainter abstract class.
 * @date 2014-01-29 Created.
 * @date 2015-11-22 Moved from ascension/src/viewer.
 * @date 2015-11-23 Renamed from caret-blinker.cpp.
 * @date 2015-11-26 Reverted from ascension/src/viewer/detail/.
 */

#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/caret-painter.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#ifdef _DEBUG
#	include <ascension/log.hpp>
#endif
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK) && !defined(GTKMM_DISABLE_DEPRECATED)
#	include <gtkmm/settings.h>
#endif

namespace ascension {
	namespace viewer {
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

		/// Default constructor.
		CaretPainter::CaretPainter() : caret_(nullptr) {
		}

		/// Destructor.
		CaretPainter::~CaretPainter() BOOST_NOEXCEPT {
		}

		/**
		 * Computes and returns logical bounds of the character the given caret addresses.
		 * @param caret The caret
		 * @param layout The text layout on which @a caret is placed
		 * @return The logical character bounds and the 'alignment-point' of the character in user units. (0, 0) is the
		 *         'alignment-point' of @a layout. You can use @c presentation#mapFlowRelativeToPhysical function to
		 *         map these values into physical coordinates
		 */
		std::pair<presentation::FlowRelativeFourSides<graphics::Scalar>, presentation::FlowRelativeTwoAxes<graphics::Scalar>>
				CaretPainter::computeCharacterLogicalBounds(const kernel::Point& caret, const graphics::font::TextLayout& layout) {
			const Index subline = layout.lineAt(kernel::offsetInLine(caret));
			const auto extent(layout.extent(boost::irange(subline, subline + 1)));
			const auto leading(layout.hitToPoint(graphics::font::TextHit<>::leading(kernel::offsetInLine(caret))));
			const auto trailing(kernel::locations::isEndOfLine(caret) ? leading : layout.hitToPoint(graphics::font::TextHit<>::trailing(kernel::offsetInLine(caret))));

			return std::make_pair(
				presentation::FlowRelativeFourSides<graphics::Scalar>(
					presentation::_blockStart = *boost::const_begin(extent), presentation::_blockEnd = *boost::const_end(extent),
					presentation::_inlineStart = leading.ipd(), presentation::_inlineEnd = trailing.ipd()),
				leading);
		}

		/// @see CaretPainterBase#hide
		void CaretPainter::hide() {
			if(shows()) {
				timer_.stop();
				setVisible(false);
				visible_ = boost::none;
			}
		}

		/// @see CaretPainterBase#install
		void CaretPainter::install(Caret& caret) {
			assert(caret_ == nullptr);
			caret_ = &caret;
			caretMotionConnection_ = caret_->motionSignal().connect([this](const Caret& caret, const SelectedRegion& regionBeforeMotion) {
				TextArea& textArea = this->caret_->textArea();
				if(&caret == this->caret_ && this->shows() && widgetapi::isVisible(textArea.textViewer())) {
					this->resetTimer();
					this->pend();

					if(kernel::line(regionBeforeMotion.caret()) != kernel::line(caret)) {
						textArea.redrawLine(kernel::line(regionBeforeMotion.caret()));
						widgetapi::redrawScheduledRegion(textArea.textViewer());
					}
					textArea.redrawLine(kernel::line(caret));
				}
			});
			viewerFocusChangedConnection_ = caret_->textArea().textViewer().focusChangedSignal().connect([this](const TextViewer& viewer) {
				if(&viewer == &this->caret_->textArea().textViewer()/* && !viewer.isFrozen()*/) {
					this->resetTimer();
					if(widgetapi::hasFocus(viewer))
						this->update();
				}
			});

			update();
			installed();
		}

		/**
		 * This @c CaretPainter was installed into the @c TextArea.
		 * @note The default implementation does nothing.
		 */
		void CaretPainter::installed() {
		}

		/// @see CaretPainterBase#paintIfShows
		void CaretPainter::paintIfShows(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) {
			if(isVisible())
				return paint(context, layout, alignmentPoint);
		}

		/// @see CaretPainterBase#pend
		void CaretPainter::pend() {
			assert(caret_ != nullptr);
			if(isCaretBlinkable(*caret_)) {
				if(const boost::optional<boost::chrono::milliseconds> interval = systemBlinkTime(*caret_)) {
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
			assert(caret_ != nullptr);
			assert(shows());
			if(visible == visible_)
				return;
			visible_ = visible;
#ifdef _DEBUG
			ASCENSION_LOG_TRIVIAL(debug)
				<< "Requested redraw line: " << kernel::line(*caret_)
				<< (visible ? " (off => on)" : " (on => off)")
				<< std::endl;
#endif
			caret_->textArea().redrawLine(kernel::line(*caret_));	// TODO: This code is not efficient.
		}

		/// @see CaretPainterBase#show
		void CaretPainter::show() {
			assert(caret_ != nullptr);
			if(!shows()) {
				visible_ = false;
				resetTimer();
				if(widgetapi::hasFocus(caret_->textArea().textViewer()))
					update();
			}
		}

		/// @see HasTimer#timeElapsed
		void CaretPainter::timeElapsed(Timer<>&) {
			assert(caret_ != nullptr);
			timer_.stop();
			const auto interval(systemBlinkTime(*caret_));
			if(interval == boost::none || !widgetapi::hasFocus(caret_->textArea().textViewer())) {
				update();
				return;
			}

			const auto timeout(systemBlinkTimeout(*caret_));
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

		/// @see CaretPainterBase#uninstall
		void CaretPainter::uninstall(Caret& caret) {
			assert(caret_ != nullptr);
			timer_.stop();
			caret_ = nullptr;
			uninstalled();
		}

		/**
		 * This @c CaretPainter was uninstalled from the @c TextArea.
		 * @note The default implementation does nothing.
		 */
		void CaretPainter::uninstalled() {
		}

		/// @see CaretPainterBase#update
		void CaretPainter::update() {
			assert(caret_ != nullptr);
			if(shows()) {
				if(isCaretBlinkable(*caret_)) {
					if(const boost::optional<boost::chrono::milliseconds> interval = systemBlinkTime(*caret_)) {
						if(!timer_.isActive()) {
							setVisible(true);
							timer_.start(boost::get(interval) * BLINK_RATE_SHOWING_MULTIPLIER / BLINK_RATE_DIVIDER, *this);
							return;
						}
					}
				}
				
				timer_.stop();
				setVisible(false);
			}
		}
	}
}
