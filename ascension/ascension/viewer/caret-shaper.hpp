/**
 * @file caret-shaper.hpp
 * This header defines caret shaping stuffs.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-09-25 separated from viewer.hpp
 */

#ifndef ASCENSION_CARET_SHAPER_HPP
#define ASCENSION_CARET_SHAPER_HPP
#include <ascension/graphics/geometry.hpp>			// graphics.NativeSize
#include <ascension/graphics/image.hpp>				// graphics.Image
#include <ascension/kernel/position.hpp>			// kernel.Position, kernel.Region
#include <ascension/presentation/writing-mode.hpp>	// presentation.ReadDirection
#include <ascension/viewer/caret-observers.hpp>		// CaretListener, CaretStateListener
#include <ascension/viewer/viewer-observers.hpp>	// TextViewerInputStatusListener
#include <utility>	// std.pair


namespace ascension {
	namespace viewers {

		// these classes are also declared by *-observers.hpp
		class Caret;
		class TextViewer;

		graphics::NativeSize currentCharacterSize(const Caret& caret);

		/**
		 * @c CaretShapeUpdater updates the caret of the text viewer.
		 * @see Caret, CaretShaper
		 */
		class CaretShapeUpdater {
			ASCENSION_UNASSIGNABLE_TAG(CaretShapeUpdater);
		public:
			Caret& caret() /*throw()*/;
			void update() /*throw()*/;
		private:
			explicit CaretShapeUpdater(Caret& caret) /*throw()*/;
			Caret& caret_;
			friend class Caret;
		};

		/**
		 * Interface for objects which define the shape of the text viewer's caret.
		 * @see TextViewer#setCaretShaper, CaretShapeUpdater, DefaultCaretShaper,
		 *      LocaleSensitiveCaretShaper
		 */
		class CaretShaper {
		public:
			/// Destructor.
			virtual ~CaretShaper() /*throw()*/ {}
			/**
			 * Returns the bitmap or the solid size defines caret shape.
			 * @param[out] image The bitmap defines caret shape. If @c null, @a solidSize is used
			 *                   and the shape is solid
			 * @param[out] solidSize The size of solid caret. If @a image is not @c null, this
			 *                       parameter is ignored
			 * @param[out] readingDirection The orientation of the caret. this value is used for
			 *                              hot spot calculation
			 */
			virtual void shape(
				std::auto_ptr<graphics::Image>& image, graphics::NativeSize& solidSize,
				presentation::ReadingDirection& readingDirection) const /*throw()*/ = 0;
		private:
			/**
			 * Installs the shaper.
			 * @param updater The caret updater which notifies the text viewer to update the caret
			 */
			virtual void install(CaretShapeUpdater& updater) /*throw()*/ = 0;
			/// Uninstalls the shaper.
			virtual void uninstall() /*throw()*/ = 0;
			friend class Caret;
		};

		/**
		 * Default implementation of @c CaretShaper.
		 * @note This class is not intended to be subclassed.
		 */
		class DefaultCaretShaper : public CaretShaper {
		public:
			DefaultCaretShaper() /*throw()*/;
		private:
			void install(CaretShapeUpdater& updater) /*throw()*/;
			void shape(
				std::auto_ptr<graphics::Image>& image, graphics::NativeSize& solidSize,
				presentation::ReadingDirection& readingDirection) const /*throw()*/;
			void uninstall() /*throw()*/;
		private:
			const TextViewer* viewer_;
		};

		/**
		 * @c LocaleSensitiveCaretShaper defines caret shape based on active keyboard layout.
		 * @note This class is not intended to be subclassed.
		 */
		class LocaleSensitiveCaretShaper : public CaretShaper,
			public CaretListener, public CaretStateListener, public InputStatusListener {
		public:
			explicit LocaleSensitiveCaretShaper(bool bold = false) /*throw()*/;
		private:
			// CaretShaper
			void install(CaretShapeUpdater& updater) /*throw()*/;
			void shape(
				std::auto_ptr<graphics::Image>& image, graphics::NativeSize& solidSize,
				presentation::ReadingDirection& readingDirection) const /*throw()*/;
			void uninstall() /*throw()*/;
			// CaretListener
			void caretMoved(const class Caret& self, const kernel::Region& oldRegion);
			// CaretStateListener
			void matchBracketsChanged(const Caret& self,
				const std::pair<kernel::Position, kernel::Position>& oldPair, bool outsideOfView);
			void overtypeModeChanged(const Caret& self);
			void selectionShapeChanged(const Caret& self);
		private:
			CaretShapeUpdater* updater_;	// weak ref.
			bool bold_;
		};

	}
} // namespace ascension.viewers

#endif // !ASCENSION_CARET_SHAPER_HPP
