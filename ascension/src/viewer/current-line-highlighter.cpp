/**
 * @file current-line-highlighter.cpp
 * Implements @c CurrentLineHighlighter class.
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2013 was viewer.cpp
 * @date 2013-12-15 separated from viewer.cpp
 */

#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/current-line-highlighter.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>

namespace ascension {
	namespace viewer {

		/**
		 * @class ascension::viewer::CurrentLineHighlighter
		 * Highlights a line the caret is on with the specified background color.
		 *
		 * Because an instance automatically registers itself as a line color director, you should
		 * not call @c Presentation#addLineColorDirector method. Usual usage is as follows.
		 *
		 * @code
		 * Caret& caret = ...;
		 * new CurrentLineHighlighter(caret);
		 * @endcode
		 *
		 * When the caret has a selection, highlight is canceled.
		 */

		/// The priority value this class returns.
		const presentation::TextLineColorSpecifier::Priority CurrentLineHighlighter::LINE_COLOR_PRIORITY = 0x40;

		/**
		 * Constructor.
		 * @param caret The caret
		 * @param foreground The initial foreground color
		 * @param background The initial background color
		 */
		CurrentLineHighlighter::CurrentLineHighlighter(Caret& caret,
				const boost::optional<graphics::Color>& foreground, const boost::optional<graphics::Color>& background)
				: caret_(&caret), foreground_(foreground), background_(background) {
			std::shared_ptr<presentation::TextLineColorSpecifier> temp(this);
			caret.textViewer().presentation().addTextLineColorSpecifier(temp);
			caretDestructionConnection_ = caret.destructionSignal().connect(std::bind(&CurrentLineHighlighter::caretDestructed, this));
			caretMotionConnection_ = caret.motionSignal().connect(std::bind(&CurrentLineHighlighter::caretMoved, this, std::placeholders::_1, std::placeholders::_2));
		}

		/// Destructor.
		CurrentLineHighlighter::~CurrentLineHighlighter() BOOST_NOEXCEPT {
			if(caret_ != nullptr) {
//				caretMotionConnection_.disconnect();
				caret_->textViewer().presentation().removeTextLineColorSpecifier(*this);
			}
		}

		/// Returns the background color.
		const boost::optional<graphics::Color>& CurrentLineHighlighter::background() const BOOST_NOEXCEPT {
			return background_;
		}

		/// @see Point#DestructionSignal
		void CurrentLineHighlighter::caretDestructed() {
//			caret_->removeListener(*this);
//			caret_->removeStateListener(*this);
			caret_ = nullptr;
		}

		/// @see CaretListener#caretMoved
		void CurrentLineHighlighter::caretMoved(const Caret&, const kernel::Region& oldRegion) {
			if(oldRegion.isEmpty()) {
				if(!isSelectionEmpty(*caret_) || kernel::line(*caret_) != oldRegion.first.line)
					caret_->textViewer().textArea().redrawLine(oldRegion.first.line, false);
			}
			if(isSelectionEmpty(*caret_)) {
				if(!oldRegion.isEmpty() || kernel::line(*caret_) != oldRegion.first.line)
					caret_->textViewer().textArea().redrawLine(kernel::line(*caret_), false);
			}
		}

		/// Returns the foreground color.
		const boost::optional<graphics::Color>& CurrentLineHighlighter::foreground() const BOOST_NOEXCEPT {
			return foreground_;
		}

		/// @see TextLineColorSpecifier#specifyTextLineColors
		presentation::TextLineColorSpecifier::Priority CurrentLineHighlighter::specifyTextLineColors(Index line,
				boost::optional<graphics::Color>& foreground, boost::optional<graphics::Color>& background) const {
			if(caret_ != nullptr && isSelectionEmpty(*caret_) && kernel::line(*caret_) == line && widgetapi::hasFocus(caret_->textViewer())) {
				foreground = foreground_;
				background = background_;
				return LINE_COLOR_PRIORITY;
			} else {
				foreground = background = boost::none;
				return 0;
			}
		}

		/**
		 * Sets the background color and redraws the window.
		 * @param color The background color to set
		 */
		void CurrentLineHighlighter::setBackground(const boost::optional<graphics::Color>& color) BOOST_NOEXCEPT {
			background_ = color;
		}

		/**
		 * Sets the foreground color and redraws the window.
		 * @param color The foreground color to set
		 */
		void CurrentLineHighlighter::setForeground(const boost::optional<graphics::Color>& color) BOOST_NOEXCEPT {
			foreground_ = color;
		}
	}
}