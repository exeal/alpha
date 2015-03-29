/**
 * @file default-caret-shaper.hpp
 * This header defines the two classes implement @c CaretShaper.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-09-25 separated from viewer.hpp
 * @date 2013-04-21 separated from caret-shaper.hpp
 */

#ifndef ASCENSION_DEFAULT_CARET_SHAPER_HPP
#define ASCENSION_DEFAULT_CARET_SHAPER_HPP
#include <ascension/graphics/color.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <map>


namespace ascension {
	namespace viewer {
		/**
		 * Default implementation of @c CaretShaper.
		 * @c DefaultCaretShaper returns system-defined caret shape (color, width) which depends on
		 * the writing mode of the text viewer and the line metrics.
		 */
		class DefaultCaretShaper : public CaretShaper, private boost::noncopyable {
		protected:
			Shape createSolidShape(const Caret& caret,
				const boost::optional<graphics::Color>& color, const boost::optional<std::uint32_t>& measure) const;
			// CaretShaper
			virtual void install(Caret& caret) BOOST_NOEXCEPT override;
			virtual Shape shape(const Caret& caret,
				const boost::optional<kernel::Position>& position) const BOOST_NOEXCEPT override;
			virtual void uninstall(Caret& caret) BOOST_NOEXCEPT override;
			// Caret.MotionSignal
			virtual void caretMoved(const Caret& caret, const kernel::Region& regionBeforeMotion);

		private:
			std::map<const Caret*, boost::signals2::connection> caretMotionConnections_;
		};

		/**
		 * @c LocaleSensitiveCaretShaper defines caret shape based on active keyboard layout.
		 * @note This class is not intended to be subclassed.
		 */
		class LocaleSensitiveCaretShaper : public DefaultCaretShaper {
		public:
			explicit LocaleSensitiveCaretShaper() BOOST_NOEXCEPT;

		private:
			// DefaultCaretShaper overrides
			void caretMoved(const Caret& caret, const kernel::Region& regionBeforeMotion) override;
			void install(Caret& caret) BOOST_NOEXCEPT override;
			Shape shape(const Caret& caret,
				const boost::optional<kernel::Position>& position) const BOOST_NOEXCEPT override;
			void uninstall(Caret& caret) BOOST_NOEXCEPT override;
			// Caret.InputModeChangedSignal
			void inputModeChanged(const Caret& caret, Caret::InputModeChangedSignalType type) BOOST_NOEXCEPT;

		private:
			std::map<const Caret*, boost::signals2::connection> inputModeChangedConnections_;
		};
	}
} // namespace ascension.viewer

#endif // !ASCENSION_DEFAULT_CARET_SHAPER_HPP
