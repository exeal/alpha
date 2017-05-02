/**
 * @file text-renderer.hpp
 * Defines @c TextRenderer class.
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2013
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 */

#ifndef ASCENSION_TEXT_RENDERER_HPP
#define ASCENSION_TEXT_RENDERER_HPP
//#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/corelib/signals.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/font/font-collection.hpp>
#include <ascension/graphics/font/text-alignment.hpp>	// TextAnchor
#include <ascension/graphics/font/visual-line.hpp>
#include <ascension/graphics/geometry/dimension.hpp>
#include <ascension/graphics/geometry/point.hpp>
#include <ascension/presentation/writing-mode.hpp>
#include <boost/range/irange.hpp>
#include <memory>	// std.shared_ptr, std.unique_ptr

namespace ascension {
	namespace kernel {
		class Document;
	}

	namespace presentation {
		struct ComputedStyledTextRunIterator;
		struct ComputedTextLineStyle;
	}

	namespace graphics {
		class Paint;
		class PaintContext;
		template<typename T> class PhysicalFourSides;
		class RenderingContext2D;

		namespace font {
			class Font;
			class LineLayoutVector;
			class LineRenderingOptions;
			class TextLayout;
			class TextViewport;

			// documentation is text-renderer.cpp
			class TextRenderer : private boost::noncopyable {
			public:
				/**
				 * @see TextRenderer#lineRelativeAlignment, TextAlignment, TextAnchor
				 */
				enum LineRelativeAlignmentAxis {
					/// Left edges of lines are left edge of the renderer.
					LEFT,
					/// Right edges of lines are right edge of the renderer.
					RIGHT,
					/// Horizontal center of lines are horizontal center of the renderer.
					HORIZONTAL_CENTER,
					/// Top edges of lines are top edge of the renderer.
					TOP,
					/// Bottom edges of lines are bottom edge of the renderer.
					BOTTOM,
					/// Vertical center of lines are vertical center of the renderer.
					VERTICAL_CENTER
				};

			public:
				virtual ~TextRenderer() BOOST_NOEXCEPT;

				/// @name Layout
				/// @{
				virtual std::unique_ptr<const TextLayout> createLineLayout(Index line) const = 0;
				LineLayoutVector& layouts() BOOST_NOEXCEPT;
				const LineLayoutVector& layouts() const BOOST_NOEXCEPT;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				virtual Scalar width() const BOOST_NOEXCEPT = 0;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				/// @}

				/// @name The Default Font
				/// @{
				std::shared_ptr<const Font> defaultFont() const BOOST_NOEXCEPT;
				typedef boost::signals2::signal<void(const TextRenderer&)> DefaultFontChangedSignal;
				SignalConnector<DefaultFontChangedSignal> defaultFontChangedSignal() BOOST_NOEXCEPT;
				/// @}

				/// @name Writing Modes
				/// @{
				virtual presentation::BlockFlowDirection blockFlowDirection() const BOOST_NOEXCEPT = 0;
				virtual presentation::ReadingDirection inlineFlowDirection() const BOOST_NOEXCEPT = 0;
				virtual presentation::TextOrientation textOrientation() const BOOST_NOEXCEPT = 0;
				presentation::WritingMode writingModes() const BOOST_NOEXCEPT;
				typedef boost::signals2::signal<void(const TextRenderer&)> WritingModesChangedSignal;
				SignalConnector<WritingModesChangedSignal> writingModesChangedSignal() BOOST_NOEXCEPT;
				/// @}

				/// @name Text Metrics
				/// @{
				Scalar baselineDistance(const boost::integer_range<VisualLine>& lines) const;
				LineRelativeAlignmentAxis lineRelativeAlignment() const BOOST_NOEXCEPT;
				Scalar lineStartEdge(const VisualLine& line) const;
				const PhysicalFourSides<Scalar>& spaceWidths() const BOOST_NOEXCEPT;
				virtual TextAnchor textAnchor() const BOOST_NOEXCEPT = 0;
				/// @}

				/// @name Painting
				/// @{
				void paint(PaintContext& context, const TextViewport& viewport, const LineRenderingOptions* options = nullptr) const;
				void paint(Index line, PaintContext& context,
					const Point& alignmentPoint, const LineRenderingOptions* options = nullptr) const;
				/// @}

			protected:
				TextRenderer(kernel::Document& document, const Dimension& initialSize);
				virtual std::shared_ptr<const Paint> actualBackground() const BOOST_NOEXCEPT = 0;
				virtual Color actualLineBackgroundColor(const TextLayout& layout) const BOOST_NOEXCEPT = 0;
				virtual std::shared_ptr<const Font> newDefaultFont() const BOOST_NOEXCEPT = 0;

			private:
				std::unique_ptr<const TextLayout> generateLineLayout(Index line) const;
				void paint(const TextLayout& layout, Index line,
					PaintContext& context, const Point& alignmentPoint, const LineRenderingOptions* options) const;
				void updateDefaultFont();
			private:
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				Scalar textWrappingMeasureInPixels_;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				std::unique_ptr<LineLayoutVector> layouts_;
				std::shared_ptr<const Font> defaultFont_;
//				class SpacePainter;
//				std::unique_ptr<SpacePainter> spacePainter_;
				DefaultFontChangedSignal defaultFontChangedSignal_;
				WritingModesChangedSignal writingModesChangedSignal_;
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
				mutable win32::Handle<HDC> memoryDC_;
				mutable win32::Handle<HBITMAP> memoryBitmap_;
#endif // ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
			};

			/// Returns the vector of layouts.
			inline LineLayoutVector& TextRenderer::layouts() BOOST_NOEXCEPT {
				return *layouts_;
			}

			/// Returns the vector of layouts.
			inline const LineLayoutVector& TextRenderer::layouts() const BOOST_NOEXCEPT {
				return *layouts_;
			}

			/// Returns the primary font. The returned value can't be @c null.
			inline std::shared_ptr<const Font> TextRenderer::defaultFont() const BOOST_NOEXCEPT {
				if(defaultFont_ == nullptr)
					const_cast<TextRenderer*>(this)->updateDefaultFont();
				return defaultFont_;
			}

#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Returns the text wrapping measure in pixels or zero if no wrap.
			 * @see #setTextWrapping, #textWrapping
			 */
			inline Scalar TextRenderer::textWrappingMeasureInPixels() const BOOST_NOEXCEPT {
				return textWrappingMeasureInPixels_;
			}
#endif // ASCENSION_ABANDONED_AT_VERSION_08

			/// Returns the @c WritingMode.
			inline presentation::WritingMode TextRenderer::writingModes() const BOOST_NOEXCEPT {
				return presentation::WritingMode(inlineFlowDirection(), blockFlowDirection(), textOrientation());
			}
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_RENDERER_HPP
