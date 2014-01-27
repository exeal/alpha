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
#include <ascension/graphics/font/text-renderer.hpp>	// graphics.font.ComputedBlockFlowDirectionListener, graphics.font.VisualLinesListener
#include <ascension/viewer/caret-shaper.hpp>
#include <utility>	// std.pair


namespace ascension {
	namespace viewers {
		/**
		 * Default implementation of @c CaretShaper.
		 * @c DefaultCaretShaper returns system-defined caret shape (color, width) which depends on
		 * the writing mode of the text viewer and the line metrics.
		 * @note This class is not intended to be subclassed.
		 */
		class DefaultCaretShaper : public CaretShaper,
			public graphics::font::ComputedBlockFlowDirectionListener,
			public graphics::font::VisualLinesListener {
			ASCENSION_NONCOPYABLE_TAG(DefaultCaretShaper);
		public:
			DefaultCaretShaper() BOOST_NOEXCEPT;
		protected:
			CaretShapeUpdater* updater() BOOST_NOEXCEPT {return updater_;}
			const CaretShapeUpdater* updater() const BOOST_NOEXCEPT {return updater_;}
			// CaretShaper
			virtual void install(CaretShapeUpdater& updater) BOOST_NOEXCEPT;
			virtual void shape(std::unique_ptr<graphics::Image>& image,
				graphics::geometry::BasicPoint<std::uint32_t>& alignmentPoint) const BOOST_NOEXCEPT;
			virtual void uninstall() BOOST_NOEXCEPT;
			// Caret.MotionSignal
			virtual void caretMoved(const Caret& caret, const kernel::Region& regionBeforeMotion);
			// graphics.font.ComputedBlockFlowDirectionListener
			void computedBlockFlowDirectionChanged(presentation::BlockFlowDirection used);
			// graphics.font.VisualLinesListener
			void visualLinesDeleted(const boost::integer_range<Index>& lines,
				Index sublines, bool longestLineChanged) BOOST_NOEXCEPT;
			void visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT;
			void visualLinesModified(
				const boost::integer_range<Index>& lines, SignedIndex sublinesDifference,
				bool documentChanged, bool longestLineChanged) BOOST_NOEXCEPT;
		private:
			CaretShapeUpdater* updater_;
			boost::signals2::connection caretMotionConnection_;
		};

		/**
		 * @c LocaleSensitiveCaretShaper defines caret shape based on active keyboard layout.
		 * @note This class is not intended to be subclassed.
		 */
		class LocaleSensitiveCaretShaper : public DefaultCaretShaper {
		public:
			explicit LocaleSensitiveCaretShaper() BOOST_NOEXCEPT;
		private:
			// CaretShaper
			void install(CaretShapeUpdater& updater) BOOST_NOEXCEPT;
			void shape(std::unique_ptr<graphics::Image>& image,
				graphics::geometry::BasicPoint<std::uint32_t>& alignmentPoint) const BOOST_NOEXCEPT;
			void uninstall() BOOST_NOEXCEPT;
			// Caret signals
			void caretMoved(const Caret& caret, const kernel::Region& regionBeforeMotion);
			void inputLocaleChanged(const Caret& caret) BOOST_NOEXCEPT;
			void inputMethodOpenStatusChanged(const Caret& caret) BOOST_NOEXCEPT;
			void overtypeModeChanged(const Caret& caret);
		private:
			boost::signals2::connection caretOvertypeModeChangedConnection_,
				inputLocaleChangedConnection_, inputMethodOpenStatusChangedConnection_;
		};

	}
} // namespace ascension.viewers

#endif // !ASCENSION_DEFAULT_CARET_SHAPER_HPP
