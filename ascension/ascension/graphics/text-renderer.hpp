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
#include <ascension/graphics/line-layout-vector.hpp>
#include <ascension/presentation/presentation.hpp>
#include <ascension/presentation/text-style.hpp>
#include <memory>	// std.shared_ptr, std.unique_ptr

namespace ascension {
	namespace graphics {
		namespace font {

			class TextRenderer;

			/**
			 * @see TextRenderer#addComputedBlockFlowDirectionListener,
			 *      TextRenderer#removeComputedBlockFlowDirectionListener,
			 *      presentation#TextToplevelStyleListener
			 */
			class ComputedBlockFlowDirectionListener {
			private:
				/**
				 * The computed block flow direction of the text renderer was changed.
				 * @param used The block flow direction used
				 */
				virtual void computedBlockFlowDirectionChanged(
					presentation::BlockFlowDirection used) = 0;
				friend class TextRenderer;
			};

			/**
			 * Interface for objects which are interested in change of the default font of
			 * @c TextRenderer.
			 * @see TextRenderer#addDefaultFontListener, TextRenderer#removeDefaultFontListener
			 */
			class DefaultFontListener {
			private:
				/// The font settings was changed.
				virtual void defaultFontChanged() = 0;
				friend class TextRenderer;
			};

			/**
			 * Options for line rendering of @c TextRenderer object.
			 * @see TextRenderer#setLineRenderingOptions
			 */
			class LineRenderingOptions {
			private:
				/**
				 * Returns the inline object renders the end of line.
				 * @param line The line to render
				 * @return The inline object renders the end of line, or @c null
				 */
				virtual const InlineObject* endOfLine(Index line) const /*throw()*/ = 0;
				/**
				 * Returns the object overrides text paint properties for line rendering. For the
				 * detail semantics of paint override, see the documentation of
				 * @c TextPaintOverride class.
				 * @param line The line to render
				 * @return The object overrides text paint properties for line rendering, or @c null
				 */
				virtual const TextPaintOverride* textPaintOverride(Index line) const /*throw()*/ = 0;
				/**
				 * Returns the inline object renders the mark of text wrapping.
				 * @param line The line to render
				 * @return The inline object renders the mark of text wrapping, or @c null
				 */
				virtual const InlineObject* textWrappingMark(Index line) const /*throw()*/ = 0;
				friend class TextRenderer;
			};

			class TextViewport;

			// documentation is text-renderer.cpp
			class TextRenderer :
				public presentation::GlobalTextStyleSwitch,
				public presentation::TextToplevelStyleListener {
			public:
				TextRenderer(presentation::Presentation& presentation,
					const FontCollection& fontCollection, const NativeSize& initialSize);
				TextRenderer(const TextRenderer& other);
				virtual ~TextRenderer() BOOST_NOEXCEPT;
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
				decltype(presentation::TextToplevelStyle().writingMode) writingMode() const BOOST_NOEXCEPT;
				/// @}

				/// @name Other Global Text Style Switch
				/// @{
				void setDirection(decltype(presentation::TextLineStyle().direction) direction);
				void setTextAlignment(decltype(presentation::TextLineStyle().textAlignment) textAlignment);
				void setTextOrientation(decltype(presentation::TextLineStyle().textOrientation) textOrientation);
				void setWhiteSpace(decltype(presentation::TextLineStyle().whiteSpace) whiteSpace);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				Scalar textWrappingMeasureInPixels() const BOOST_NOEXCEPT;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				// presentation.GlobalTextStyleSwitch
				decltype(presentation::TextLineStyle().direction) direction() const BOOST_NOEXCEPT;
				decltype(presentation::TextLineStyle().textAlignment) textAlignment() const BOOST_NOEXCEPT;
				decltype(presentation::TextLineStyle().textOrientation) textOrientation() const BOOST_NOEXCEPT;
				decltype(presentation::TextLineStyle().whiteSpace) whiteSpace() const BOOST_NOEXCEPT;
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
				Scalar baselineDistance(const Range<VisualLine>& lines) const;
				const PhysicalFourSides<Scalar>& spaceWidths() const BOOST_NOEXCEPT;
				/// @}

				/// @name Painting
				/// @{
				void paint(PaintContext& context) const;
				void paint(Index line, PaintContext& context, const NativePoint& alignmentPoint) const;
				void setLineRenderingOptions(const std::shared_ptr<LineRenderingOptions> options);
				/// @}
			protected:
				void buildLineLayoutConstructionParameters(Index line, ComputedTextLineStyle& lineStyle,
					std::unique_ptr<ComputedStyledTextRunIterator>& runStyles, FontCollection& fontCollection) const;
			private:
				std::unique_ptr<const TextLayout> generateLineLayout(Index line) const;
				void updateComputedBlockFlowDirectionChanged();
				// presentation.TextToplevelStyleListener
				void textToplevelStyleChanged(std::shared_ptr<const presentation::TextToplevelStyle> used);
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
				decltype(presentation::TextLineStyle().direction) direction_;
				decltype(presentation::TextLineStyle().textAlignment) textAlignment_;
				decltype(presentation::TextLineStyle().textOrientation) textOrientation_;
				decltype(presentation::TextLineStyle().whiteSpace) whiteSpace_;
				decltype(presentation::TextToplevelStyle().writingMode) writingMode_;
				presentation::BlockFlowDirection computedBlockFlowDirection_;
				detail::Listeners<ComputedBlockFlowDirectionListener> computedBlockFlowDirectionListeners_;
				detail::Listeners<DefaultFontListener> defaultFontListeners_;
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
				mutable win32::Handle<HDC> memoryDC_;
				mutable win32::Handle<HBITMAP> memoryBitmap_;
#endif // defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
			};

			/// @see presentation#GlobalTextStyleSwitch#direction
			inline decltype(presentation::TextLineStyle().direction) TextRenderer::direction() const BOOST_NOEXCEPT {
				return direction_;
			}

			/// @see presentation#GlobalTextStyleSwitch#textAlignment
			inline decltype(presentation::TextLineStyle().textAlignment) TextRenderer::textAlignment() const BOOST_NOEXCEPT {
				return textAlignment_;
			}

			/// @see presentation#GlobalTextStyleSwitch#textOrientation
			inline decltype(presentation::TextLineStyle().textOrientation) TextRenderer::textOrientation() const BOOST_NOEXCEPT {
				return textOrientation_;
			}

			/// @see presentation#GlobalTextStyleSwitch#whiteSpace
			inline decltype(presentation::TextLineStyle().whiteSpace) TextRenderer::whiteSpace() const BOOST_NOEXCEPT {
				return whiteSpace_;
			}

			/// @see presentation#GlobalTextStyleSwitch#writingMode
			inline decltype(presentation::TextToplevelStyle().writingMode) TextRenderer::writingMode() const BOOST_NOEXCEPT {
				return writingMode_;
			}

			/// Returns the presentation used by this object.
			inline const presentation::Presentation& TextRenderer::presentation() const BOOST_NOEXCEPT {
				return presentation_;
			}

			/// Returns the primary font.
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

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_RENDERER_HPP
