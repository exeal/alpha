/**
 * @file text-renderer.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2012
 * @date 2010-11-20 separated from ascension/layout.hpp
 * @date 2011-11-12 renamed from rendering.hpp
 */

#ifndef ASCENSION_TEXT_RENDERER_HPP
#define ASCENSION_TEXT_RENDERER_HPP

//#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/graphics/line-layout-vector.hpp>
#include <ascension/presentation/presentation.hpp>
#include <memory>	// std.shared_ptr, std.unique_ptr, std.weak_ptr

namespace ascension {
	namespace graphics {
		namespace font {

			class TextRenderer;

			/**
			 * @see TextRenderer#addComputedWritingModeListener,
			 *      TextRenderer#removeComputedWritingModeListener
			 */
			class ComputedWritingModeListener {
			private:
				virtual void computedWritingModeChanged(const presentation::WritingMode& used) = 0;
				friend class TextRenderer;
			};

			/*
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

			// documentation is layout.cpp
			class TextRenderer : public presentation::GlobalTextStyleListener {
			public:
				// constructors
				TextRenderer(presentation::Presentation& presentation,
					const FontCollection& fontCollection, const NativeSize& initialSize);
				TextRenderer(const TextRenderer& other);
				virtual ~TextRenderer() /*throw()*/;
				// viewport
				std::weak_ptr<TextViewport> viewport() const /*throw()*/;
				// layout
				virtual std::unique_ptr<const TextLayout> createLineLayout(Index line) const = 0;
				LineLayoutVector& layouts() /*throw()*/;
				const LineLayoutVector& layouts() const /*throw()*/;
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
				virtual Scalar width() const /*throw()*/ = 0;
#endif // ASCENSION_ABANDONED_AT_VERSION_08
				// writing modes
				void addComputedWritingModeListener(ComputedWritingModeListener& listener);
				const presentation::WritingMode& defaultUIWritingMode() const /*throw()*/;
				void removeComputedWritingModeListener(ComputedWritingModeListener& listener);
				presentation::WritingMode writingMode() const /*throw()*/;
				// text wrappings
				void setTextWrapping(
					const presentation::TextWrapping<presentation::Length>& newValue,
					const RenderingContext2D* renderingContext);
				const presentation::TextWrapping<presentation::Length>& textWrapping() const /*throw()*/;
				Scalar textWrappingMeasureInPixels() const /*throw()*/;
				// default font
				void addDefaultFontListener(DefaultFontListener& listener);
				std::shared_ptr<const Font> defaultFont() const /*throw()*/;
				void removeDefaultFontListener(DefaultFontListener& listener);
				// text metrics
				Scalar baselineDistance(const Range<VisualLine>& lines) const;
				const PhysicalFourSides<Scalar>& spaceWidths() const /*throw()*/;
				// paint
				void paint(PaintContext& context) const;
				void paint(Index line, PaintContext& context, const NativePoint& alignmentPoint) const;
				void setLineRenderingOptions(const std::shared_ptr<LineRenderingOptions> options);

				// LayoutInformationProvider
				const FontCollection& fontCollection() const /*throw()*/;
				const presentation::Presentation& presentation() const /*throw()*/;
//				SpecialCharacterRenderer* specialCharacterRenderer() const /*throw()*/;
//				const Font::Metrics& textMetrics() const /*throw()*/;
			protected:
				void buildLineLayoutConstructionParameters(Index line,
					TextLayout::ConstructionParameters& parameters) const;
				void setDefaultUIWritingMode(const presentation::WritingMode& writingMode);
			private:
				void fireComputedWritingModeChanged(
					const presentation::TextToplevelStyle& globalTextStyle,
					const presentation::WritingMode& defaultUI);
				std::unique_ptr<const TextLayout> generateLineLayout(Index line) const;
				void updateDefaultFont();
				// presentation.GlobalTextStyleListener
				void globalTextStyleChanged(std::shared_ptr<const presentation::TextToplevelStyle> used);
			private:
				presentation::Presentation& presentation_;
				presentation::WritingMode defaultUIWritingMode_;
				presentation::TextWrapping<presentation::Length> textWrapping_;
				Scalar textWrappingMeasureInPixels_;
				std::unique_ptr<LineLayoutVector> layouts_;
				const FontCollection& fontCollection_;
				std::shared_ptr<const Font> defaultFont_;
				std::shared_ptr<const LineRenderingOptions> lineRenderingOptions_;
				std::shared_ptr<TextViewport> viewport_;
				class SpacePainter;
				std::unique_ptr<SpacePainter> spacePainter_;
				detail::Listeners<ComputedWritingModeListener> computedWritingModeListeners_;
				detail::Listeners<DefaultFontListener> defaultFontListeners_;
#if defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
				mutable std::shared_ptr<std::remove_pointer<HDC>::type> memoryDC_;
				mutable std::shared_ptr<std::remove_pointer<HBITMAP>::type> memoryBitmap_;
#endif // defined(ASCENSION_GRAPHICS_SYSTEM_WIN32_GDI) && ASCENSION_ABANDONED_AT_VERSION_08
			};


			/// Returns the primary font.
			inline std::shared_ptr<const Font> TextRenderer::defaultFont() const /*throw()*/ {
				return defaultFont_;
			}

			/**
			 * Returns the default writing mode for user interface. This setting is used to resolve
			 * ambiguous properties specified by @c presentation#Presentation#writingMode method.
			 * Derived classes can reset this value by calling @c #setDefaultUIWritingMode method.
			 * The default value is initialized by the default constructor of
			 * @c presentation#WritingMode class.
			 * @see #setDefaultUIWritingMode, #writingMode
			 */
			inline const presentation::WritingMode& TextRenderer::defaultUIWritingMode() const /*throw()*/ {
				return defaultUIWritingMode_;
			}

			/// Returns the font collection used by this object.
			inline const FontCollection& TextRenderer::fontCollection() const /*throw()*/ {
				return fontCollection_;
			}

			/// Returns the presentation used by this object.
			inline const presentation::Presentation& TextRenderer::presentation() const /*throw()*/ {
				return presentation_;
			}

			/// @see LayoutInformationProvider#textMetrics
//			inline const Font::Metrics& TextRenderer::textMetrics() const /*throw()*/ {
//				return primaryFont()->metrics();
//			}

			/**
			 * Returns the text wrapping settings.
			 * @see #setTextWrapping, #textWrappingMeasureInPixels
			 */
			inline const presentation::TextWrapping<presentation::Length>& TextRenderer::textWrapping() const /*throw()*/ {
				return textWrapping_;
			}

			/**
			 * Returns the text wrapping measure in pixels or zero if no wrap.
			 * @see #setTextWrapping, #textWrapping
			 */
			inline Scalar TextRenderer::textWrappingMeasureInPixels() const /*throw()*/ {
				return textWrappingMeasureInPixels_;
			}

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_TEXT_RENDERER_HPP
