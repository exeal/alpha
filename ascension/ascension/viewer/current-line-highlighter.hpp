/**
 * @file current-line-highlighter.hpp
 * Defines @c CurrentLineHighlighter class.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2013 was viewer.hpp
 * @date 2013-12-15 separated from viewer.hpp
 */

#ifndef ASCENSION_CURRENT_LINE_HIGHLIGHTER_HPP
#define ASCENSION_CURRENT_LINE_HIGHLIGHTER_HPP

#include <ascension/kernel/point.hpp>				// kernel.PointLifeCycleListener
#include <ascension/graphics/color.hpp>				// graphics.Color
#include <ascension/presentation/presentation.hpp>	// presentation.TextLineColorSpecifier
#include <ascension/viewer/caret-observers.hpp>		// CaretListener, CaretStateListener
#include <boost/optional.hpp>

namespace ascension {
	namespace viewers {
		class Caret;

		/// Highlights the line on which the caret is put.
		class CurrentLineHighlighter : public presentation::TextLineColorSpecifier,
				public CaretListener, public CaretStateListener, public kernel::PointLifeCycleListener {
			ASCENSION_NONCOPYABLE_TAG(CurrentLineHighlighter);
		public:
			// constant
			static const presentation::TextLineColorSpecifier::Priority LINE_COLOR_PRIORITY;
			// constructors
			CurrentLineHighlighter(Caret& caret,
				const boost::optional<graphics::Color>& foreground,
				const boost::optional<graphics::Color>& background);
			~CurrentLineHighlighter() BOOST_NOEXCEPT;
			// attributes
			const boost::optional<graphics::Color>& background() const BOOST_NOEXCEPT;
			const boost::optional<graphics::Color>& foreground() const BOOST_NOEXCEPT;
			void setBackground(const boost::optional<graphics::Color>& color) BOOST_NOEXCEPT;
			void setForeground(const boost::optional<graphics::Color>& color) BOOST_NOEXCEPT;
		private:
			// presentation.TextLineColorDirector
			presentation::TextLineColorSpecifier::Priority specifyTextLineColors(
				Index line, boost::optional<graphics::Color>& foreground,
				boost::optional<graphics::Color>& background) const;
			// CaretListener
			void caretMoved(const Caret& self, const kernel::Region& oldRegion);
			// CaretStateListener
			void matchBracketsChanged(const Caret& self,
				const boost::optional<std::pair<kernel::Position, kernel::Position>>& oldPair,
				bool outsideOfView);
			void overtypeModeChanged(const Caret& self);
			void selectionShapeChanged(const Caret& self);
			// kernel.PointLifeCycleListener
			void pointDestroyed();
		private:
			Caret* caret_;
			boost::optional<graphics::Color> foreground_, background_;
		};
	}
}

#endif	// !ASCENSION_CURRENT_LINE_HIGHLIGHTER_HPP
