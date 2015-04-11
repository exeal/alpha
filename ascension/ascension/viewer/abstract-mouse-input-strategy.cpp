/**
 * @file abstract-mouse-input-strategy.cpp
 * Implements @c AbstractMouseInputStrategy class.
 * @author exeal
 * @date 2015-04-08 Created.
 */

#include <ascension/corelib/numeric-range-algorithm/clamp.hpp>
#include <ascension/graphics/font/font-metrics.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-viewport.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/text-editor/session.hpp>	// texteditor.endIncrementalSearch
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/mouse-input-strategy.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/text-viewer-utility.hpp>	// utils.closeCompletionProposalsPopup
#include <ascension/viewer/widgetapi/cursor.hpp>
#include <ascension/viewer/widgetapi/event/mouse-button-input.hpp>
#include <ascension/viewer/widgetapi/event/mouse-wheel-input.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/optional.hpp>

namespace ascension {
	namespace viewer {
		const unsigned int AbstractMouseInputStrategy::SELECTION_EXPANSION_INTERVAL_IN_MILLISECONDS = 100;
		const unsigned int AbstractMouseInputStrategy::DRAGGING_TRACK_INTERVAL_IN_MILLISECONDS = 100;

		struct AbstractMouseInputStrategy::Tracking {
			std::shared_ptr<MouseInputStrategy> mouseInputStrategy;
			TextViewer* viewer;
			Timer timer;
			TargetLocker* inputTargetLocker;
			bool autoScroll, locateCursor;
		};

		/// Default constructor.
		AbstractMouseInputStrategy::AbstractMouseInputStrategy() BOOST_NOEXCEPT {
			// hides reference to the destructor of Tracking inner class.
		}

		/// Destructor.
		AbstractMouseInputStrategy::~AbstractMouseInputStrategy() BOOST_NOEXCEPT {
		}

		/**
		 * Begins tracking of the mouse location. See the descriptions of the parameters.
		 * @param viewer The text viewer
		 * @param targetLocker The @c MouseInputStrategy#TargetLocker object. This method calls
		 *                     @c MouseInputStrategy#TargetLocker#lockMouseInputTarget method.
		 * @param autoScroll If @c true, this object scrolls @a viewer automatically and continuously toward the
		 *                   location of the mouse if the mouse was outside @a viewer
		 * @param locateCursor If @c true, @c #trackedLocationChanged method is called continuously
		 * @see #endLocationTracking, #trackedLocationChanged
		 */
		void AbstractMouseInputStrategy::beginLocationTracking(TextViewer& viewer, TargetLocker& targetLocker, bool autoScroll, bool locateCursor) {
			if(!isTrackingLocation()) {
				// cancel other modes
				utils::closeCompletionProposalsPopup(viewer);
				texteditor::endIncrementalSearch(viewer);

				// begin tracking
				tracking_.reset(new Tracking);
				tracking_->mouseInputStrategy.reset(this, boost::null_deleter());
				tracking_->viewer = &viewer;
				(tracking_->inputTargetLocker = &targetLocker)->lockMouseInputTarget(tracking_->mouseInputStrategy);
				tracking_->autoScroll = autoScroll;
				tracking_->locateCursor = locateCursor;
				tracking_->timer.start(SELECTION_EXPANSION_INTERVAL_IN_MILLISECONDS, *this);
			}
		}

		/// Does nothing.
		void AbstractMouseInputStrategy::captureChanged() {
		}

		/**
		 * Ends tracking of the mouse location.
		 * @see #beginLocationTracking
		 */
		void AbstractMouseInputStrategy::endLocationTracking() {
			if(isTrackingLocation()) {
				tracking_->timer.stop();
				tracking_->inputTargetLocker->unlockMouseInputTarget(*tracking_->mouseInputStrategy);
				tracking_.reset();
			}
		}
		
		/// Returns @c null.
		std::shared_ptr<widgetapi::DropTarget> AbstractMouseInputStrategy::handleDropTarget() const {
			return std::shared_ptr<widgetapi::DropTarget>();
		}
		
		/// Calls @c #endTrackingLocation method.
		void AbstractMouseInputStrategy::interruptMouseReaction(bool) {
			endTrackingLocation();
		}

		/// Returns @c true if is tracking the location.
		bool AbstractMouseInputStrategy::isTrackingLocation() const BOOST_NOEXCEPT {
			return tracking_.get() != nullptr;
		}
		
		/// Ignores the input.
		void AbstractMouseInputStrategy::mouseButtonInput(Action, widgetapi::event::MouseButtonInput& input, TargetLocker&) {
			return input.ignore();
		}

		/// Calls @c #endTrackingLocation method.
		void AbstractMouseInputStrategy::mouseInputTargetUnlocked() {
			endTrackingLocation();
		}
		
		/// Ignores the input.
		void AbstractMouseInputStrategy::mouseMoved(widgetapi::event::LocatedUserInput& input, TargetLocker&) {
			return input.ignore();
		}
		
		/// Ignores the input.
		void AbstractMouseInputStrategy::mouseWheelRotated(widgetapi::event::MouseWheelInput& input, TargetLocker&) {
			return input.ignore();
		}

		/**
		 * Shows the built-in arrow cursor.
		 * @param viewer The text viewer
		 * @return @c true
		 * @see #showCursor
		 */
		bool AbstractMouseInputStrategy::showArrowCursor(TextViewer& viewer) {
			boost::optional<widgetapi::Cursor::BuiltinShape> builtinShape(
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Gdk::ARROW
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				Qt::ArrowCursor
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
				[NSCursor arrowCursor]
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				IDC_ARROW
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			);
			const widgetapi::Cursor cursor(*builtinShape);
			showCursor(viewer, cursor);
			return true;
		}

		/**
		 * Shows the specified cursor.
		 * @param viewer The text viewer
		 * @param cursor The cursor to show
		 * @return @c true
		 */
		bool AbstractMouseInputStrategy::showCursor(TextViewer& viewer, const widgetapi::Cursor& cursor) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			viewer.get_window()->set_cursor(const_cast<widgetapi::Cursor&>(cursor).asNativeObject());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			QApplication::setOverrideCursor(cursor.asNativeObject());	// TODO: Restore later.
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			[cursor set];
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			::SetCursor(cursor.asNativeObject().get());
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			return true;
		}
		
		/// Returns @c false.
		bool AbstractMouseInputStrategy::showCursor(const graphics::Point&) {
			return false;
		}

		/// @see HasTimer#timeElapsed
		void AbstractMouseInputStrategy::timeElapsed(Timer& timer) {
			if(tracking_.get() != nullptr) {
				TextViewer& viewer = *tracking_->viewer;

				// scroll the text viewer automatically
				if(tracking_->autoScroll) {
					const std::shared_ptr<graphics::font::TextViewport> viewport(viewer.textArea().textRenderer().viewport());
					const graphics::Point p(widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position()));
					const graphics::Rectangle contentRectangle(viewer.textAreaContentRectangle());
					graphics::Dimension scrollUnits(
						graphics::geometry::_dx = graphics::font::inlineProgressionOffsetInViewerGeometry(*viewport, 1),
						graphics::geometry::_dy = widgetapi::createRenderingContext(viewer)->fontMetrics(viewer.textArea().textRenderer().defaultFont())->linePitch());
					if(isVertical(viewer.textArea().textRenderer().computedBlockFlowDirection()))
						graphics::geometry::transpose(scrollUnits);

					graphics::PhysicalTwoAxes<graphics::font::TextViewportSignedScrollOffset> scrollOffsets(0, 0);
					// no rationale about these scroll amounts
					if(graphics::geometry::y(p) < graphics::geometry::top(contentRectangle))
						scrollOffsets.y() =
							static_cast<graphics::font::TextViewportSignedScrollOffset>((graphics::geometry::y(p)
								- (graphics::geometry::top(contentRectangle))) / graphics::geometry::dy(scrollUnits) - 1);
					else if(graphics::geometry::y(p) >= graphics::geometry::bottom(contentRectangle))
						scrollOffsets.y() =
							static_cast<graphics::font::TextViewportSignedScrollOffset>((graphics::geometry::y(p)
								- (graphics::geometry::bottom(contentRectangle))) / graphics::geometry::dy(scrollUnits) + 1);
					else if(graphics::geometry::x(p) < graphics::geometry::left(contentRectangle))
						scrollOffsets.x() =
							static_cast<graphics::font::TextViewportSignedScrollOffset>((graphics::geometry::x(p)
								- (graphics::geometry::left(contentRectangle))) / graphics::geometry::dx(scrollUnits) - 1);
					else if(graphics::geometry::x(p) >= graphics::geometry::right(contentRectangle))
						scrollOffsets.x() =
							static_cast<graphics::font::TextViewportSignedScrollOffset>((graphics::geometry::x(p)
								- (graphics::geometry::right(contentRectangle))) / graphics::geometry::dx(scrollUnits) + 1);
					if(scrollOffsets.x() != 0 || scrollOffsets.y() != 0)
						viewport->scroll(scrollOffsets);
				}

				// locate the cursor position
				if(tracking_->locateCursor) {
					graphics::Point p(widgetapi::mapFromGlobal(viewer, widgetapi::Cursor::position()));

					// snap cursor position into 'content-rectangle' of the text area
					const graphics::Rectangle contentRectangle(viewer.textAreaContentRectangle());
					graphics::geometry::x(p) = clamp(static_cast<graphics::Scalar>(graphics::geometry::x(p)), graphics::geometry::range<0>(contentRectangle));
					graphics::geometry::y(p) = clamp(static_cast<graphics::Scalar>(graphics::geometry::y(p)), graphics::geometry::range<1>(contentRectangle));

					trackedLocationChanged(graphics::font::viewToModel(*viewer.textArea().textRenderer().viewport(), p).characterIndex());
				}
			}
		}

		/**
		 * Called after @c #beginLocationTracking call continuously.
		 * @param position The position which addresses the character nearest the mouse.
		 * @see graphics#font#viewToModel
		 */
		void AbstractMouseInputStrategy::trackedLocationChanged(const kernel::Position&) {
		}
	}
}
