/**
 * @c file standard-caret-painter.hpp
 * Defines @c StandardCaretPainter class.
 * @author exeal
 * @date 2015-11-28 Created.
 */

#ifndef ASCENSION_STANDARD_CARET_PAINTER_HPP
#define ASCENSION_STANDARD_CARET_PAINTER_HPP
#include <ascension/corelib/timer.hpp>
#include <ascension/presentation/flow-relative-four-sides.hpp>
#include <ascension/viewer/caret-painter.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <boost/optional.hpp>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace ascension {
	namespace viewer {
		class StandardCaretPainter : public CaretPainter, private HasTimer<> {
		public:
			/// Blinking intervals setting.
			struct BlinkIntervals {
				/// The duration of the caret showing.
				/// The real duration time is `(showingRate / (showingRate + hidingRate)) * interval`.
				unsigned int showingRate;
				/// The duration of the caret hiding.
				/// The real duration time is `(hidingDuration / (showingRate + hidingRate)) * interval`.
				unsigned int hidingRate;
				/// The duration of the caret blinking pending. If @c boost#none, uses @c showingDuration.
				/// The real duration time is `(pendingDuration / (showingRate + hidingRate)) * interval`.
				boost::optional<unsigned int> pendingDuration;
				/// Interval duration.
				/// - If @c boost#none, uses the system setting.
				/// - If @c boost#chrono#milliseconds#zero(), the caret does not blink.
				boost::optional<boost::chrono::milliseconds> interval;
				/// Timeout duration.
				/// - If @c boost#none, uses the system setting.
				/// - If @c boost#chrono#milliseconds#zero(), the caret blinks eternally.
				boost::optional<boost::chrono::milliseconds> timeout;

				/// Creates a @c BlinkIntervals with showing rate is 2 and hiding rate is 1.
				BlinkIntervals() : showingRate(2), hidingRate(1) {}
			};

			StandardCaretPainter();

			/// @name Strategies
			/// @{
			void setBlinkIntervals(const BlinkIntervals& newIntervals);
			void setShaper(std::unique_ptr<CaretShaper> newShaper);
			/// @}

		private:
			static std::pair<
				presentation::FlowRelativeFourSides<graphics::Scalar>, presentation::FlowRelativeTwoAxes<graphics::Scalar>
			> computeCharacterLogicalBounds(const Caret& caret, const graphics::font::TextLayout& layout);
			bool isVisible() const BOOST_NOEXCEPT;
			void paint(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint);
			void pend();
			void resetTimer();
			void setVisible(bool visible);
			void timeElapsed(Timer<>& timer);
			void update();
			// detail.CaretPainterBase
			void hide() override;
			void install(Caret& caret) override;
			void paintIfShows(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) override;
			void show() override;
			bool shows() const BOOST_NOEXCEPT override;
			void uninstall(Caret& caret) override;
		private:
			Caret* caret_;	// weak reference
			Timer<> timer_;
			boost::chrono::milliseconds elapsedTimeFromLastUserInput_;
			boost::optional<bool> visible_;	// boost.none => hides, true => visible, false => blinking and invisible
			boost::signals2::scoped_connection caretMotionConnection_, viewerFocusChangedConnection_;
			BlinkIntervals blinkIntervals_;
		};
	}
}

#endif // !ASCENSION_STANDARD_CARET_PAINTER_HPP
