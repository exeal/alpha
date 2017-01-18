/**
 * @file standard-caret-shaper.hpp
 * This header defines the two classes implement @c CaretShaper.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-09-25 separated from viewer.hpp
 * @date 2013-04-21 separated from caret-shaper.hpp
 * @date 2017-01-18 Renamed from default-caret-shaper.hpp.
 */

#ifndef ASCENSION_STANDARD_CARET_SHAPER_HPP
#define ASCENSION_STANDARD_CARET_SHAPER_HPP
#include <ascension/graphics/color.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <map>

namespace ascension {
	namespace viewer {
		/**
		 * Standard implementation of @c CaretShaper.
		 * @c StandardCaretShaper returns standard shapes based on the system setting, the active keyboard layout and
		 * the writing mode.
		 */
		class StandardCaretShaper : public CaretShaper, private boost::noncopyable {
		protected:
			// CaretShaper
			void install(Caret& caret) BOOST_NOEXCEPT override;
			Shape shape(const Caret& caret,
				const boost::optional<kernel::Position>& position) const BOOST_NOEXCEPT override;
			void uninstall(Caret& caret) BOOST_NOEXCEPT override;
			// Caret.MotionSignal
			void caretMoved(const Caret& caret, const SelectedRegion& regionBeforeMotion);
			// Caret.InputModeChangedSignal
			void inputModeChanged(const Caret& caret, Caret::InputModeChangedSignalType type) BOOST_NOEXCEPT;

		private:
			std::map<const Caret*, boost::signals2::connection> caretMotionConnections_;
			std::map<const Caret*, boost::signals2::connection> inputModeChangedConnections_;
		};
	}
} // namespace ascension.viewer

#endif // !ASCENSION_STANDARD_CARET_SHAPER_HPP
