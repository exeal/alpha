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
#include <ascension/corelib/signals.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/graphics/geometry/rectangle.hpp>
#include <boost/optional.hpp>
#include <memory>	// std.shared_ptr
#include <tuple>

namespace ascension {
	namespace graphics {
		class Image;
	}

	namespace kernel {
		class Position;
	}

	namespace viewer {
		class Caret;
		class TextArea;

		boost::optional<graphics::Rectangle> currentCharacterLogicalBounds(const Caret& caret);

		/**
		 * Interface for objects which define the shape of the text viewer's caret.
		 * @see TextArea#setCaretShaper, CaretShapeUpdater, DefaultCaretShaper, LocaleSensitiveCaretShaper
		 */
		class CaretShaper {
		public:
			/// Describes a shape of the caret.
			struct Shape {
				/// An image which defines shape of the caret. If this is @c null, the @c Caret ignores the result of
				/// @c CaretShaper#shape and uses default implementation by @c DefaultCaretShaper class.
				std::shared_ptr<const graphics::Image> image;
				/// The alignment-point of the @c #image in pixels, which matches the alignment-point (a point on the
				/// start-edge of the glyph on the the baseline of the line (not the glyph)) of the character addressed
				/// by the caret.
				graphics::geometry::BasicPoint<std::uint32_t> alignmentPoint;
			};
			/// Destructor.
			virtual ~CaretShaper() BOOST_NOEXCEPT {}
			/**
			 * Returns the image defines caret shape.
			 * @param caret The caret to shape
			 * @param position The prior position of the @a caret. If this is @c boost#none, the value of
			 *                 @c Caret#position() should be used
			 * @return A shape of the caret
			 */
			virtual Shape shape(const Caret& caret,
				const boost::optional<kernel::Position>& position) const BOOST_NOEXCEPT = 0;
			typedef boost::signals2::signal<void(const Caret&)> StaticShapeChangedSignal;
			SignalConnector<StaticShapeChangedSignal> staticShapeChangedSignal() BOOST_NOEXCEPT;

		protected:
			/**
			 * Invokes @c StaticShapeChangedSignal with the specified caret.
			 * @param caret The argument passed to the slots
			 */
			void signalStaticShapeChanged(const Caret& caret) BOOST_NOEXCEPT {
				staticShapeChangedSignal_(caret);
			}

		private:
			/**
			 * Installs the shaper for the specified caret.
			 * @param caret The caret
			 */
			virtual void install(Caret& caret) BOOST_NOEXCEPT = 0;
			/**
			 * Uninstalls the shaper for the specified caret.
			 * @param caret The caret
			 */
			virtual void uninstall(Caret& caret) BOOST_NOEXCEPT = 0;
			friend class TextArea;

		private:
			StaticShapeChangedSignal staticShapeChangedSignal_;
		};
	}
} // namespace ascension.viewer

#endif // !ASCENSION_CARET_SHAPER_HPP
