/**
 * @c file standard-caret-painter.cpp
 * Implements @c StandardCaretPainter class.
 * @author exeal
 * @date 2015-11-28 Created.
 * @date 2017-01-15 Joined with caret-painter.cpp.
 */

#include <ascension/corelib/numeric-range-algorithm/includes.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/paint.hpp>
#include <ascension/graphics/paint-context.hpp>
#include <ascension/graphics/geometry/algorithms/translate.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/locations.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/standard-caret-painter.hpp>
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
			inline boost::chrono::milliseconds hidingDuration(const StandardCaretPainter::BlinkIntervals& intervals, const boost::chrono::milliseconds& interval) BOOST_NOEXCEPT {
				return interval * intervals.showingRate / (intervals.showingRate + intervals.hidingRate);
			}

			inline bool isCaretBlinkable(const Caret& caret) {
				// TODO: Check if the text viewer is also editable.
				return widgetapi::hasFocus(caret.textArea().textViewer());
			}

			inline boost::chrono::milliseconds pendingDuration(const StandardCaretPainter::BlinkIntervals& intervals, const boost::chrono::milliseconds& interval) BOOST_NOEXCEPT {
				return interval * boost::get_optional_value_or(intervals.pendingDuration, intervals.showingRate) / (intervals.showingRate + intervals.hidingRate);
			}

			inline boost::chrono::milliseconds showingDuration(const StandardCaretPainter::BlinkIntervals& intervals, const boost::chrono::milliseconds& interval) BOOST_NOEXCEPT {
				return interval - hidingDuration(intervals, interval);
			}

			inline boost::optional<boost::chrono::milliseconds> systemBlinkTime(const Caret& caret) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#ifndef GTKMM_DISABLE_DEPRECATED
				const Glib::RefPtr<const Gtk::Settings> settings(const_cast<TextViewer&>(caret.textArea().textViewer()).get_settings());
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
				return (ms != INFINITE) ? boost::make_optional(boost::chrono::milliseconds(ms * 2)) : boost::none;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			}

			inline boost::optional<boost::chrono::milliseconds> systemBlinkTimeout(const Caret& caret) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#ifndef GTKMM_DISABLE_DEPRECATED
				const Glib::RefPtr<const Gtk::Settings> settings(const_cast<TextViewer&>(caret.textArea().textViewer()).get_settings());
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

			template<typename T>
			T systemCaretDu(const Caret& caret, T dv) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				if(Glib::RefPtr<const Gtk::StyleContext> styles = caret.textArea().textViewer().get_style_context()) {
					gfloat aspectRatio;
					styles->get_style_property("cursor-aspect-ratio", aspectRatio);
					return dv * aspectRatio + 1;
				}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				DWORD width;
				if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
					width = 1;	// NT4 does not support SPI_GETCARETWIDTH
				return static_cast<T>(width);
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				return 1;
			}
		}

		/// Creates a @c StandardCaretPainter.
		StandardCaretPainter::StandardCaretPainter() : caret_(nullptr) {
		}

		/**
		 * @internal Computes and returns logical bounds of the character the given caret addresses.
		 * @param caret The caret
		 * @param layout The text layout on which @a caret is placed
		 * @return The logical character bounds and the 'alignment-point' of the character in user units. (0, 0) is the
		 *         'alignment-point' of @a layout. You can use @c presentation#mapFlowRelativeToPhysical function to
		 *         map these values into physical coordinates
		 */
		std::pair<presentation::FlowRelativeFourSides<graphics::Scalar>, presentation::FlowRelativeTwoAxes<graphics::Scalar>>
				StandardCaretPainter::computeCharacterLogicalBounds(const Caret& caret, const graphics::font::TextLayout& layout) {
			const auto h(inlineHit(caret.hit()));
			const Index subline = layout.lineAt(h);
			const auto extent(layout.extent(boost::irange(subline, subline + 1)));
			const auto leading(layout.hitToPoint(h));
			const auto trailing(kernel::locations::isEndOfLine(caret) ?
				leading : layout.hitToPoint(graphics::font::makeTrailingTextHit(kernel::offsetInLine(caret.hit().characterIndex()))));

			return std::make_pair(
				presentation::FlowRelativeFourSides<graphics::Scalar>(
					presentation::_blockStart = *boost::const_begin(extent), presentation::_blockEnd = *boost::const_end(extent),
					presentation::_inlineStart = leading.ipd(), presentation::_inlineEnd = trailing.ipd()),
				leading);
		}

		/// Implements @c CaretPainter#hide. This method stops the blinking interval.
		void StandardCaretPainter::hide() {
			if(shows()) {
				timer_.stop();
				setVisible(false);
				visible_ = boost::none;
			}
		}

		/// @see CaretPainter#install
		void StandardCaretPainter::install(Caret& caret) {
			assert(caret_ == nullptr);
			caret_ = &caret;
			caretMotionConnection_ = caret_->motionSignal().connect([this](const Caret& caret, const SelectedRegion& regionBeforeMotion) {
				TextArea& textArea = this->caret_->textArea();
				if(&caret == this->caret_ && this->shows() && widgetapi::isVisible(textArea.textViewer())) {
					this->resetTimer();
					this->pend();

					const Index lineBeforeMotion = kernel::line(insertionPosition(caret.document(), regionBeforeMotion.caret()));
					if(lineBeforeMotion != kernel::line(caret) && includes(caret.document().region().lines(), lineBeforeMotion)) {
						textArea.redrawLine(lineBeforeMotion);
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
		}

		/// @internal Returns @c true if the caret is in blinking-on.
		inline bool StandardCaretPainter::isVisible() const BOOST_NOEXCEPT {
			return boost::get_optional_value_or(visible_, false);
		}

		/// Implements @c CaretPainter#paint method.
		void StandardCaretPainter::paint(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) {
			if(caret_ == nullptr)
				return;
#ifdef _DEBUG
			ASCENSION_LOG_TRIVIAL(debug) << "StandardCaretPainter.paint() with line number " << kernel::line(*caret_) << std::endl;
#endif
			const auto writingMode(graphics::font::writingMode(layout));
			const auto logicalBounds(computeCharacterLogicalBounds(*caret_, layout));
			auto logicalShape(std::get<0>(logicalBounds));

			if(!caret_->isOvertypeMode() || !isSelectionEmpty(*caret_)) {
				const auto advance = systemCaretDu(*caret_, presentation::extent(std::get<0>(logicalBounds)));
				logicalShape.inlineEnd() = logicalShape.inlineStart() + advance;
			}
			graphics::Rectangle box;
			{
				graphics::PhysicalFourSides<graphics::Scalar> temp;
				presentation::mapDimensions(writingMode, presentation::_from = logicalShape, presentation::_to = temp);
				boost::geometry::assign(box, graphics::geometry::make<graphics::Rectangle>(temp));
			}
			graphics::geometry::translate(
				graphics::geometry::_to = box, graphics::geometry::_from = box,
				graphics::geometry::_tx = graphics::geometry::x(alignmentPoint), graphics::geometry::_ty = graphics::geometry::y(alignmentPoint));

			context.save();
			try {
				context
//					.setGlobalCompositeOperation(graphics::XOR)
					.setFillStyle(std::make_shared<graphics::SolidColor>(graphics::Color::OPAQUE_BLACK))
					.fillRectangle(box);
			} catch(...) {
			}
			context.restore();
		}

		/// Implements @c CaretPainterBase#paintIfShows.
		void StandardCaretPainter::paintIfShows(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) {
			if(isVisible())
				return paint(context, layout, alignmentPoint);
		}

		/// @internal Pends the blinking interval timer.
		void StandardCaretPainter::pend() {
			assert(caret_ != nullptr);
			if(isCaretBlinkable(*caret_)) {
				if(const boost::optional<boost::chrono::milliseconds> interval = systemBlinkTime(*caret_)) {
					timer_.stop();
					timer_.start(pendingDuration(blinkIntervals_, boost::get(interval)), *this);
					setVisible(true);
				}
			}
		}

		/// @internal Resets the blinking interval timer.
		inline void StandardCaretPainter::resetTimer() {
			elapsedTimeFromLastUserInput_ = boost::chrono::milliseconds::zero();
		}

		/**
		 * @throw std#invalid_argument
		 */
		void StandardCaretPainter::setBlinkIntervals(const BlinkIntervals& newIntervals) {
			if(newIntervals.showingRate == 0)
				throw std::invalid_argument("newIntervals.showingRate");
			else if(newIntervals.hidingRate == 0)
				throw std::invalid_argument("newIntervals.hidingRate");
			blinkIntervals_ = newIntervals;
		}

		/// @internal
		inline void StandardCaretPainter::setVisible(bool visible) {
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

		/// Implements @c CaretPainterBase#show. Shows the caret and restarts the blinking interval.
		void StandardCaretPainter::show() {
			assert(caret_ != nullptr);
			if(!shows()) {
				visible_ = false;
				resetTimer();
				if(widgetapi::hasFocus(caret_->textArea().textViewer()))
					update();
			}
		}

		/// Implements @c CaretPainterBase#shows. Returns @c true even if the caret is in blinking-off.
		bool StandardCaretPainter::shows() const BOOST_NOEXCEPT {
			return visible_ != boost::none;
		}

		/// @see HasTimer#timeElapsed
		void StandardCaretPainter::timeElapsed(Timer<>&) {
			assert(caret_ != nullptr);
			timer_.stop();
			const auto interval(systemBlinkTime(*caret_));
			if(interval == boost::none || !widgetapi::hasFocus(caret_->textArea().textViewer())) {
				update();
				return;
			}

			auto timeout(blinkIntervals_.timeout);
			if(timeout == boost::none)
				timeout = systemBlinkTimeout(*caret_);
			if(timeout != boost::none && boost::get(timeout) != boost::chrono::milliseconds::zero() && elapsedTimeFromLastUserInput_ > boost::get(timeout)) {
				// stop blinking
				setVisible(true);
			} else if(isVisible()) {
				setVisible(false);
				if(blinkIntervals_.showingRate != 0)
					timer_.start(hidingDuration(blinkIntervals_, boost::get(interval)), *this);
			} else {
				setVisible(true);
				elapsedTimeFromLastUserInput_ += boost::get(interval);
				if(blinkIntervals_.hidingRate != 0)
					timer_.start(showingDuration(blinkIntervals_, boost::get(interval)), *this);
			}
		}

		/// @see CaretPainter#uninstall
		void StandardCaretPainter::uninstall(Caret& caret) {
			assert(caret_ != nullptr);
			timer_.stop();
			caret_ = nullptr;
		}

		/// @internal Checks and updates state of blinking of the caret.
		void StandardCaretPainter::update() {
			assert(caret_ != nullptr);
			if(shows()) {
				if(isCaretBlinkable(*caret_)) {
					auto interval(blinkIntervals_.interval);
					if(interval == boost::none)
						interval = systemBlinkTime(*caret_);
					if(interval != boost::none && boost::get(interval) != boost::chrono::milliseconds::zero()) {
						if(!timer_.isActive()) {
							setVisible(true);
							timer_.start(showingDuration(blinkIntervals_, boost::get(interval)), *this);
							return;
						}
					} else
						setVisible(true);
				}

				timer_.stop();
				setVisible(false);
			}
		}
	}
}
