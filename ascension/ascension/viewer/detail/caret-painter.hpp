/**
 * @file caret-painter.hpp
 * Defines @c CaretBlinker internal class.
 * @author exeal
 * @date 2015-11-22 Separated from text-area.hpp.
 * @date 2015-11-23 Renamed from caret-blinker.hpp.
 */

#ifndef ASCENSION_CARET_PAINTER_HPP
#define ASCENSION_CARET_PAINTER_HPP
#include <ascension/corelib/timer.hpp>
#include <boost/optional.hpp>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace ascension {
	namespace viewer {
		class Caret;
		class CaretShaper;

		namespace detail {
			class CaretPainter : private HasTimer<> {
			public:
				explicit CaretPainter(Caret& caret) BOOST_NOEXCEPT;
				void hide();
				bool isVisible() const BOOST_NOEXCEPT;
				void paintIfShows();
				void pend();
				void resetTimer();
				void show();
				BOOST_CONSTEXPR bool shows() const BOOST_NOEXCEPT;
				void update();

			private:
				void setVisible(bool visible);
				void timeElapsed(Timer<>& timer);
				Caret& caret_;
				Timer<> timer_;
				boost::chrono::milliseconds elapsedTimeFromLastUserInput_;
				boost::optional<bool> visible_;	// boost.none => hides, true => visible, false => blinking and invisible
				boost::signals2::scoped_connection caretMotionConnection_, viewerFocusChangedConnection_;
			};

			/// Returns @c true if the caret is visible.
			inline bool CaretPainter::isVisible() const BOOST_NOEXCEPT {
				return visible_;
			}

			/// Returns @c true if the caret is shown (may be blinking off).
			BOOST_CONSTEXPR inline bool CaretPainter::shows() const BOOST_NOEXCEPT {
				return visible_ != boost::none;
			}
		}
	}
}

#endif // !ASCENSION_CARET_PAINTER_HPP
