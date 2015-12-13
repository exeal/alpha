/**
 * @file caret-painter.hpp
 * Defines @c CaretPainter abstract class.
 * @author exeal
 * @date 2015-11-22 Separated from text-area.hpp.
 * @date 2015-11-23 Renamed from caret-blinker.hpp.
 * @date 2015-11-26 Reverted from ascension/viewer/detail/.
 */

#ifndef ASCENSION_CARET_PAINTER_HPP
#define ASCENSION_CARET_PAINTER_HPP
#include <ascension/corelib/timer.hpp>
#include <ascension/presentation/flow-relative-directions-dimensions.hpp>
#include <ascension/viewer/detail/caret-painter-base.hpp>
#include <boost/optional.hpp>
#include <boost/signals2/connection.hpp>
#include <memory>

namespace ascension {
	namespace kernel {
		class Point;
	}

	namespace viewer {
		class Caret;

		/**
		 * Paints the caret on the @c TextArea with blinking.
		 * @c CaretPainter is an abstract class and the derived class should implement @c #paint method to draw a
		 * concrete figure of the caret.
		 * @see SolidCaretPainter, LocaleSensitivePainter
		 */
		class CaretPainter : public detail::CaretPainterBase, private HasTimer<> {
		public:
			virtual ~CaretPainter() BOOST_NOEXCEPT;

		protected:
			CaretPainter();
			BOOST_CONSTEXPR Caret& caret() BOOST_NOEXCEPT;
			BOOST_CONSTEXPR const Caret& caret() const BOOST_NOEXCEPT;
			static std::pair<
				presentation::FlowRelativeFourSides<graphics::Scalar>, presentation::FlowRelativeTwoAxes<graphics::Scalar>
			> computeCharacterLogicalBounds(const kernel::Point& caret, const graphics::font::TextLayout& layout);
			virtual void installed() BOOST_NOEXCEPT;
			virtual void paint(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) = 0;
			virtual void uninstalled() BOOST_NOEXCEPT;

		private:
			void setVisible(bool visible);
			void timeElapsed(Timer<>& timer);
			// detail.CaretPainterBase
			void hide() override;
			void install(Caret& caret) override;
			bool isVisible() const BOOST_NOEXCEPT override;
			void paintIfShows(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) override;
			void pend() override;
			void resetTimer() override;
			void show() override;
			BOOST_CONSTEXPR bool shows() const BOOST_NOEXCEPT override;
			void uninstall(Caret& caret) override;
			void update() override;
		private:
			Caret* caret_;
			Timer<> timer_;
			boost::chrono::milliseconds elapsedTimeFromLastUserInput_;
			boost::optional<bool> visible_;	// boost.none => hides, true => visible, false => blinking and invisible
			boost::signals2::scoped_connection caretMotionConnection_, viewerFocusChangedConnection_;
		};

		/// Returns the caret.
		BOOST_CONSTEXPR inline Caret& CaretPainter::caret() BOOST_NOEXCEPT {
			assert(caret_ != nullptr);
			return *caret_;
		}

		/// Returns the caret.
		BOOST_CONSTEXPR inline const Caret& CaretPainter::caret() const BOOST_NOEXCEPT {
			assert(caret_ != nullptr);
			return *caret_;
		}

		/// @see CaretPainterBase#isVisible
		inline bool CaretPainter::isVisible() const BOOST_NOEXCEPT {
			return boost::get_optional_value_or(visible_, false);
		}

		/// @see CaretPainterBase#shows
		BOOST_CONSTEXPR inline bool CaretPainter::shows() const BOOST_NOEXCEPT {
			return visible_ != boost::none;
		}
	}
}

#endif // !ASCENSION_CARET_PAINTER_HPP
