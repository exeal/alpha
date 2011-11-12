/**
 * @file rendering.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 separated from ascension/layout.hpp
 */

#ifndef ASCENSION_RENDERING_HPP
#define ASCENSION_RENDERING_HPP

//#include <ascension/config.hpp>	// ASCENSION_DEFAULT_TEXT_READING_DIRECTION
#include <ascension/graphics/line-layout-vector.hpp>
#include <ascension/presentation/presentation.hpp>

namespace ascension {

	namespace viewers {class Caret;}

	namespace graphics {
		namespace font {

			class TextRenderer;

			/**
			 * @see TextRenderer#addComputedWritingModeListener,
			 *      TextRenderer#removeComputedWritingModeListener
			 */
			class ComputedWritingModeListener {
			private:
				virtual void computedWritingModeChanged(const presentation::WritingMode<false>& used) = 0;
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

			// documentation is layout.cpp
			class TextRenderer : public presentation::GlobalTextStyleListener {
			public:
				// constructors
				TextRenderer(presentation::Presentation& presentation,
					const FontCollection& fontCollection, bool enableDoubleBuffering);
				TextRenderer(const TextRenderer& other);
				virtual ~TextRenderer() /*throw()*/;
				// layout
				virtual std::auto_ptr<const TextLayout> createLineLayout(length_t line) const = 0;
				LineLayoutVector& layouts() /*throw()*/;
				const LineLayoutVector& layouts() const /*throw()*/;
				virtual Scalar width() const /*throw()*/ = 0;
				// writing modes
				void addComputedWritingModeListener(ComputedWritingModeListener& listener);
				const presentation::WritingMode<false>& defaultUIWritingMode() const /*throw()*/;
				void removeComputedWritingModeListener(ComputedWritingModeListener& listener);
				presentation::WritingMode<false> writingMode() const /*throw()*/;
				// default font
				void addDefaultFontListener(DefaultFontListener& listener);
				std::tr1::shared_ptr<const Font> defaultFont() const /*throw()*/;
				void removeDefaultFontListener(DefaultFontListener& listener);
				// text metrics
				Scalar baselineDistance(const Range<VisualLine>& lines) const;
				Scalar lineIndent(length_t line, length_t subline = 0) const;
				Scalar lineStartEdge(length_t line) const;
				// operation
				void renderLine(length_t line, PaintContext& context,
					const NativePoint& origin, const TextPaintOverride* paintOverride = 0,
					const InlineObject* endOfLine = 0, const InlineObject* lineWrappingMark = 0) const /*throw()*/;
					
				// LayoutInformationProvider
				const FontCollection& fontCollection() const /*throw()*/;
				const presentation::Presentation& presentation() const /*throw()*/;
//				SpecialCharacterRenderer* specialCharacterRenderer() const /*throw()*/;
//				const Font::Metrics& textMetrics() const /*throw()*/;
			protected:
				void setDefaultUIWritingMode(const presentation::WritingMode<false>& writingMode);
			private:
				void fireComputedWritingModeChanged(
					const presentation::TextToplevelStyle& globalTextStyle,
					const presentation::WritingMode<false>& defaultUI);
				std::auto_ptr<const TextLayout> generateLineLayout(length_t line) const;
				void updateDefaultFont();
				// presentation.GlobalTextStyleListener
				void globalTextStyleChanged(std::tr1::shared_ptr<const presentation::TextToplevelStyle> used);
			private:
				presentation::Presentation& presentation_;
				presentation::WritingMode<false> defaultUIWritingMode_;
				std::auto_ptr<LineLayoutVector> layouts_;
				const FontCollection& fontCollection_;
				const bool enablesDoubleBuffering_;
				std::tr1::shared_ptr<const Font> defaultFont_;
				detail::Listeners<ComputedWritingModeListener> computedWritingModeListeners_;
				detail::Listeners<DefaultFontListener> defaultFontListeners_;
				mutable win32::Handle<HDC> memoryDC_;
				mutable win32::Handle<HBITMAP> memoryBitmap_;
			};


			/// Returns the primary font.
			inline std::tr1::shared_ptr<const Font> TextRenderer::defaultFont() const /*throw()*/ {
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
			inline const presentation::WritingMode<false>& TextRenderer::defaultUIWritingMode() const /*throw()*/ {
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

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_RENDERING_HPP
