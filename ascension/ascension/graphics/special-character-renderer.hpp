/**
 * @file special-character-renderer.hpp
 * @date 2003-2006 (was LineLayout.h)
 * @date 2006-2010
 * @date 2010-11-20 separated from text-layout.hpp
 */

#ifndef ASCENSION_SPECIAL_CHARACTER_RENDERER_HPP
#define ASCENSION_SPECIAL_CHARACTER_RENDERER_HPP

#include <ascension/kernel/document.hpp>	// kernel.Newline
#include <ascension/presentation.hpp>
#include <ascension/graphics/color.hpp>
#include <ascension/graphics/rendering.hpp>	// IDefaultFontListener

namespace ascension {

	namespace presentation {class Presentation;}
	namespace viewers {class Caret;}

	namespace graphics {
		namespace font {

			class ISpecialCharacterRenderer {
			public:
				/// Destructor.
				virtual ~ISpecialCharacterRenderer() /*throw()*/ {}
			protected:
				/// Context of the layout.
				struct LayoutContext {
					ASCENSION_UNASSIGNABLE_TAG(LayoutContext);
				public:
					mutable Context& renderingContext;					///< the rendering context.
					presentation::ReadingDirection readingDirection;	///< the orientation of the character.
					/// Constructor.
					explicit LayoutContext(Context& renderingContext) /*throw()*/ : renderingContext(renderingContext) {}
				};
				/// Context of the drawing.
				struct DrawingContext : public LayoutContext {
					Rect<> rect;	///< the bounding box to draw.
					/// Constructor.
					DrawingContext(Context& deviceContext) /*throw()*/ : LayoutContext(renderingContext) {}
				};
			private:
				/**
				 * Draws the specified C0 or C1 control character.
				 * @param context the context
				 * @param c the code point of the character to draw
				 */
				virtual void drawControlCharacter(const DrawingContext& context, CodePoint c) const = 0;
				/**
				 * Draws the specified line break indicator.
				 * @param context the context
				 * @param newline the newline to draw
				 */
				virtual void drawLineTerminator(const DrawingContext& context, kernel::Newline newline) const = 0;
				/**
				 * Draws the width of a line wrapping mark.
				 * @param context the context
				 */
				virtual void drawLineWrappingMark(const DrawingContext& context) const = 0;
				/**
				 * Draws the specified white space character.
				 * @param context the context
				 * @param c the code point of the character to draw
				 */
				virtual void drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const = 0;
				/**
				 * Returns the width of the specified C0 or C1 control character.
				 * @param context the context
				 * @param c the code point of the character to layout
				 * @return the width or 0 if does not render the character
				 */
				virtual int getControlCharacterWidth(const LayoutContext& context, CodePoint c) const = 0;
				/**
				 * Returns the width of the specified line break indicator.
				 * @param context the context
				 * @param newline the newline to layout
				 * @return the width or 0 if does not render the indicator
				 */
				virtual int getLineTerminatorWidth(const LayoutContext& context, kernel::Newline newline) const = 0;
				/**
				 * Returns the width of a line wrapping mark.
				 * @param context the context
				 * @return the width or 0 if does not render the mark
				 */
				virtual int getLineWrappingMarkWidth(const LayoutContext& context) const = 0;
				/**
				 * Installs the drawer.
				 * @param textRenderer the text renderer
				 */
				virtual void install(TextRenderer& textRenderer) = 0;
				/// Uninstalls the drawer.
				virtual void uninstall() = 0;
				friend class LineLayout;
				friend class TextRenderer;
			};

			class DefaultSpecialCharacterRenderer : public ISpecialCharacterRenderer, public IDefaultFontListener {
			public:
				// constructors
				DefaultSpecialCharacterRenderer() /*throw()*/;
				// attributes
				const Color& controlCharacterColor() const /*throw()*/;
				const Color& lineTerminatorColor() const /*throw()*/;
				const Color& lineWrappingMarkColor() const /*throw()*/;
				void setControlCharacterColor(const Color& color) /*throw()*/;
				void setLineTerminatorColor(const Color& color) /*throw()*/;
				void setLineWrappingMarkColor(const Color& color) /*throw()*/;
				void setWhiteSpaceColor(const Color& color) /*throw()*/;
				void showLineTerminators(bool show) /*throw()*/;
				void showWhiteSpaces(bool show) /*throw()*/;
				bool showsLineTerminators() const /*throw()*/;
				bool showsWhiteSpaces() const /*throw()*/;
				const Color& whiteSpaceColor() const /*throw()*/;
			private:
				// ISpecialCharacterRenderer
				void drawControlCharacter(const DrawingContext& context, CodePoint c) const;
				void drawLineTerminator(const DrawingContext& context, kernel::Newline newline) const;
				void drawLineWrappingMark(const DrawingContext& context) const;
				void drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const;
				int getControlCharacterWidth(const LayoutContext& context, CodePoint c) const;
				int getLineTerminatorWidth(const LayoutContext& context, kernel::Newline newline) const;
				int getLineWrappingMarkWidth(const LayoutContext& context) const;
				void install(TextRenderer& textRenderer);
				void uninstall();
				// IDefaultFontListener
				void defaultFontChanged();
			private:
				TextRenderer* renderer_;
				Color controlColor_, eolColor_, wrapMarkColor_, whiteSpaceColor_;
				bool showsEOLs_, showsWhiteSpaces_;
				std::tr1::shared_ptr<const Font> font_;	// provides substitution glyphs
				enum {LTR_HORIZONTAL_TAB, RTL_HORIZONTAL_TAB, LINE_TERMINATOR, LTR_WRAPPING_MARK, RTL_WRAPPING_MARK, WHITE_SPACE};
				GlyphCode glyphs_[6];
				int glyphWidths_[6];
			};

		}
	}
} // namespace ascension.graphics.font

#endif // !ASCENSION_SPECIAL_CHARACTER_RENDERER_HPP
