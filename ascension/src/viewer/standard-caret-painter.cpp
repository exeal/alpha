/**
 * @c file standard-caret-painter.cpp
 * Implements @c StandardCaretPainter class.
 * @author exeal
 * @date 2015-11-28 Created.
 */

#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/paint.hpp>
#include <ascension/graphics/rendering-context.hpp>
#include <ascension/graphics/geometry/algorithms/translate.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/standard-caret-painter.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#ifdef _DEBUG
#	include <ascension/log.hpp>
#endif

namespace ascension {
	namespace viewer {
		namespace {
			template<typename T>
			T systemCaretDu(const Caret& caret, T dv) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				if(Glib::RefPtr<const Gtk::StyleContext> styles = caret.textArea().textViewer().get_style_context()) {
					gfloat aspectRatio;
					styles->get_style_property("cursor-aspect-ratio", aspectRatio);
					return dv * aspectRatio + 1;
				}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				DWORD width;
				if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
					width = 1;	// NT4 does not support SPI_GETCARETWIDTH
				return static_cast<std::uint32_t>(width);
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
				return 1;
			}
		}

		/// Implements @c CaretPainter#paint method.
		void StandardCaretPainter::paint(graphics::PaintContext& context,
				const graphics::font::TextLayout& layout, const graphics::Point& alignmentPoint) {
#ifdef _DEBUG
			ASCENSION_LOG_TRIVIAL(debug) << "StandardCaretPainter.paint() with line number " << kernel::line(caret()) << std::endl;
#endif
			const auto writingMode(graphics::font::writingMode(layout));
			const auto logicalBounds(computeCharacterLogicalBounds(caret(), layout));
			auto logicalShape(std::get<0>(logicalBounds));

			if(!caret().isOvertypeMode() || !isSelectionEmpty(caret())) {
				const auto advance = systemCaretDu(caret(), presentation::extent(std::get<0>(logicalBounds)));
				logicalShape.inlineEnd() = logicalShape.inlineStart() + advance;
			}
			graphics::Rectangle box;
			{
				graphics::PhysicalFourSides<graphics::Scalar> temp;
				presentation::mapDimensions(writingMode, presentation::_from = logicalShape, presentation::_to = temp);
				boost::geometry::assign(box, graphics::geometry::make<graphics::Rectangle>(temp));
			}
			graphics::geometry::translate((
				graphics::geometry::_to = box, graphics::geometry::_from = box,
				graphics::geometry::_dx = graphics::geometry::x(alignmentPoint), graphics::geometry::_dy = graphics::geometry::y(alignmentPoint)));

			context.save();
			try {
				context
//					.setGlobalCompositeOperation(graphics::XOR)
					.setFillStyle(std::make_shared<graphics::SolidColor>(graphics::Color::OPAQUE_BLACK))
					.fillRectangle(box);
			} catch(...) {
			}
			context.restore();
		}
	}
}
