/**
 * @file text-renderer.hpp
 * @date 2003-2006 was LineLayout.h
 * @date 2006-2013
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 */

#ifndef ASCENSION_TEXT_RENDERER_HPP
#define ASCENSION_TEXT_RENDERER_HPP

//#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-renderer-observers.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/text-style.hpp>
#include <memory>	// std.shared_ptr, std.unique_ptr

namespace ascension {
	namespace graphics {
		namespace font {

			class TextViewport;

			// documentation is text-renderer.cpp
			class TextRenderer :
				public presentation::GlobalTextStyleSwitch,
				public presentation::TextToplevelStyleListener {
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
				TextRenderer(presentation::Presentation& presentation,
					const FontCollection& fontCollection, const Dimension& initialSize);
				TextRenderer(const TextRenderer& other);
				virtual ~TextRenderer() BOOST_NOEXCEPT;
				presentation::Presentation& presentation() BOOST_NOEXCEPT;
				const presentation::Presentation& presentation() const BOOST_NOEXCEPT;

				/// @name Viewport
				/// @{
				std::shared_ptr<TextViewport> viewport() BOOST_NOEXCEPT;
				std::shared_ptr<const TextViewport> viewport() const BOOST_NOEXCEPT;
				/// @}

				/// @name Layout
				/// @{
				virtual std::unique_ptr<const TextLayout> createLineLayout(Index line) const = 0;
				LineLayoutVector& layouts() BOOST_NOEXCEPT;
				const LineLayoutVector& layouts() const BOOST_NOEXCEPT;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				virtual Scalar width() const BOOST_NOEXCEPT = 0;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				/// @}

				/// @name Block Flow Direction
				/// @{
				void addComputedBlockFlowDirectionListener(ComputedBlockFlowDirectionListener& listener);
				presentation::BlockFlowDirection computedBlockFlowDirection() const BOOST_NOEXCEPT;
				void removeComputedBlockFlowDirectionListener(ComputedBlockFlowDirectionListener& listener);
				void setWritingMode(decltype(presentation::TextToplevelStyle().writingMode) writingMode);
				// presentation.GlobalTextStyleSwitch
				WritingMode writingMode() const BOOST_NOEXCEPT;
				/// @}

				/// @name Other Global Text Style Switch
				/// @{
				void setDirection(Direction direction);
				void setTextAlignment(TextAlignment textAlignment);
				void setTextOrientation(TextOrientation textOrientation);
				void setWhiteSpace(WhiteSpace whiteSpace);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				Scalar textWrappingMeasureInPixels() const BOOST_NOEXCEPT;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				// presentation.GlobalTextStyleSwitch
				Direction direction() const override BOOST_NOEXCEPT;
				TextAlignment textAlignment() const override BOOST_NOEXCEPT;
				TextOrientation textOrientation() const override BOOST_NOEXCEPT;
				WhiteSpace whiteSpace() const override BOOST_NOEXCEPT;
				/// @}

				/// @name Default (Globally Nominal) Font
				/// @{
				void addDefaultFontListener(DefaultFontListener& listener);
				std::shared_ptr<const Font> defaultFont() const BOOST_NOEXCEPT;
				void removeDefaultFontListener(DefaultFontListener& listener);
				void setDefaultFont(const String& familyName, double pointSize);
				/// @}

				/// @name Text Metrics
				/// @{
				Scalar baselineDistance(const boost::integer_range<VisualLine>& lines) const;
				LineRelativeAlignmentAxis lineRelativeAlignment() const BOOST_NOEXCEPT;
				Scalar lineStartEdge(const VisualLine& line) const;
				const PhysicalFourSides<Scalar>& spaceWidths() const BOOST_NOEXCEPT;
				/// @}

				/// @name Painting
				/// @{
				void paint(PaintContext& context) const;
				void paint(Index line, PaintContext& context, const Point& alignmentPoint) const;
				void setLineRenderingOptions(const std::shared_ptr<LineRenderingOptions> options);
				/// @}

			protected:
				void buildLineLayoutConstructionParameters(Index line, const RenderingContext2D& graphics2D,
					ComputedTextLineStyle& lineStyle, std::unique_ptr<ComputedStyledTextRunIterator>& runStyles) const;
				const FontCollection& fontCollection() const BOOST_NOEXCEPT;

			private:
				std::unique_ptr<const TextLayout> generateLineLayout(Index line) const;
				void updateComputedBlockFlowDirectionChanged();
				// presentation.TextToplevelStyleListener
				void textToplevelStyleChanged(std::shared_ptr<const presentation::TextToplevelStyle> used) override;
			private:
				presentation::Presentation& presentation_;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				Scalar textWrappingMeasureInPixels_;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				std::unique_ptr<LineLayoutVector> layouts_;
				const FontCollection& fontCollection_;
				std::shared_ptr<const Font> defaultFont_;
				std::shared_ptr<const LineRenderingOptions> lineRenderingOptions_;
				std::shared_ptr<TextViewport> viewport_;
//				class SpacePainter;
//				std::unique_ptr<SpacePainter> spacePainter_;
				Direction direction_;
				TextAlignment textAlignment_;
				TextOrientation textOrientation_;
				WhiteSpace whiteSpace_;
				decltype(presentation::TextToplevelStyle().writingMode) writingMode_;
				presentation::BlockFlowDirection computedBlockFlowDirection_;
				ascension::detail::Listeners<ComputedBlockFlowDirectionListener> computedBlockFlowDirectionListeners_;
				ascension::detail::Listeners<DefaultFontListener> defaultFontListeners_;
#if ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
				mutable win32::Handle<HDC> memoryDC_;
				mutable win32::Handle<HBITMAP> memoryBitmap_;
#endif // ASCENSION_SELECTS_GRAPHICS_SYSTEM(WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
			};

			/// Returns the computed block-flow-direction.
			inline presentation::BlockFlowDirection TextRenderer::computedBlockFlowDirection() const BOOST_NOEXCEPT {
				return computedBlockFlowDirection_;
			}

			/// @see presentation#GlobalTextStyleSwitch#direction
			inline TextRenderer::Direction TextRenderer::direction() const BOOST_NOEXCEPT {
				return direction_;
			}

			/**
			 * Returns the @c FontCollection object used by this @c TextRenderer.
			 * @see #buildLineLayoutConstructionParameters
			 */
			inline const FontCollection& TextRenderer::fontCollection() const BOOST_NOEXCEPT {
				return fontCollection_;
			}

			/// Returns the vector of layouts.
			inline LineLayoutVector& TextRenderer::layouts() BOOST_NOEXCEPT {
				return *layouts_;
			}

			/// Returns the vector of layouts.
			inline const LineLayoutVector& TextRenderer::layouts() const BOOST_NOEXCEPT {
				return *layouts_;
			}

			/// @see presentation#GlobalTextStyleSwitch#textAlignment
			inline TextRenderer::TextAlignment TextRenderer::textAlignment() const BOOST_NOEXCEPT {
				return textAlignment_;
			}

			/// @see presentation#GlobalTextStyleSwitch#textOrientation
			inline TextRenderer::TextOrientation TextRenderer::textOrientation() const BOOST_NOEXCEPT {
				return textOrientation_;
			}

			/// @see presentation#GlobalTextStyleSwitch#whiteSpace
			inline TextRenderer::WhiteSpace TextRenderer::whiteSpace() const BOOST_NOEXCEPT {
				return whiteSpace_;
			}

			/// @see presentation#GlobalTextStyleSwitch#writingMode
			inline TextRenderer::WritingMode TextRenderer::writingMode() const BOOST_NOEXCEPT {
				return writingMode_;
			}

			/// Returns the presentation used by this object.
			inline presentation::Presentation& TextRenderer::presentation() BOOST_NOEXCEPT {
				return presentation_;
			}

			/// Returns the presentation used by this object.
			inline const presentation::Presentation& TextRenderer::presentation() const BOOST_NOEXCEPT {
				return presentation_;
			}

			/// Returns the primary font. The returned value can't be @c null.
			inline std::shared_ptr<const Font> TextRenderer::defaultFont() const BOOST_NOEXCEPT {
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

			/// Returns the viewport.
			inline std::shared_ptr<TextViewport> TextRenderer::viewport() BOOST_NOEXCEPT {
				return viewport_;
			}

			/// Returns the viewport.
			inline std::shared_ptr<const TextViewport> TextRenderer::viewport() const BOOST_NOEXCEPT {
				return viewport_;
			}
		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_RENDERER_HPP
