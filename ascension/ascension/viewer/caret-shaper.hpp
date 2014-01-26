/**
 * @file caret-shaper.hpp
 * This header defines caret shaping stuffs.
 * @author exeal
 * @date 2003-2006 was EditView.h
 * @date 2006-2011 was viewer.hpp
 * @date 2011-09-25 separated from viewer.hpp
 * @date 2012-2014
 */

#ifndef ASCENSION_CARET_SHAPER_HPP
#define ASCENSION_CARET_SHAPER_HPP
#include <ascension/graphics/geometry.hpp>	// graphics.Scalar, graphics.Point
#include <boost/optional.hpp>
#include <memory>	// std.unique_ptr


namespace ascension {
	namespace graphics {
		class Image;
	}

	namespace viewers {

		// these classes are also declared by *-observers.hpp
		class Caret;
		class TextViewer;

		boost::optional<graphics::Rectangle> currentCharacterLogicalBounds(const Caret& caret);

		/**
		 * A @c CaretShapeUpdater gives a @c CaretShaper the trigger to update the visualization of
		 * the caret of the text viewer.
		 * @see Caret, CaretShaper
		 */
		class CaretShapeUpdater {
			ASCENSION_UNASSIGNABLE_TAG(CaretShapeUpdater);
		public:
			Caret& caret() BOOST_NOEXCEPT;
			const Caret& caret() const BOOST_NOEXCEPT;
			void update() BOOST_NOEXCEPT;
		private:
			explicit CaretShapeUpdater(Caret& caret) BOOST_NOEXCEPT;
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
			virtual ~CaretShaper() BOOST_NOEXCEPT {}
			/**
			 * Returns the bitmap defines caret shape.
			 * @param[out] image The bitmap defines caret shape. If @c null, the @c Caret ignores
			 *                   the all result and uses default implementation by
			 *                   @c DefaultCaretShaper class
			 * @param[out] alignmentPoint The alignment-point of @a image in pixels, which matches
			 *                            the alignment-point (a point on the start-edge of the
			 *                            glyph on the the baseline of the line (not the glyph)) of
			 *                            the character addressed by the caret
			 */
			virtual void shape(std::unique_ptr<graphics::Image>& image,
				graphics::geometry::BasicPoint<std::uint32_t>& alignmentPoint) const BOOST_NOEXCEPT = 0;
		private:
			/**
			 * Installs the shaper.
			 * @param updater The caret updater which notifies the text viewer to update the caret
			 */
			virtual void install(CaretShapeUpdater& updater) BOOST_NOEXCEPT = 0;
			/// Uninstalls the shaper.
			virtual void uninstall() BOOST_NOEXCEPT = 0;
			friend class Caret;
		};
	}
} // namespace ascension.viewers

#endif // !ASCENSION_CARET_SHAPER_HPP
