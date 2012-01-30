/**
 * @file default-special-character-renderer.cpp
 * @author exeal
 * @date 2003-2006 (was LineLayout.cpp)
 * @date 2006-2010
 * @date 2010-11-20 separated from text-layout.cpp
 */

#include <ascension/graphics/special-character-renderer.hpp>
#include <ascension/graphics/graphics.hpp>
#include <ascension/win32/windows.hpp>

using namespace ascension;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace ascension::presentation;
using namespace std;
namespace k = ascension::kernel;


/**
 * @class ascension::layout::ISpecialCharacterRenderer
 * Interface for objects which draw special characters.
 *
 * @c ISpecialCharacterRenderer hooks shaping and drawing processes of @c LineLayout about some
 * special characters. These include:
 * - C0 controls
 * - C1 controls
 * - End of line (Line terminators)
 * - White space characters
 * - Line wrapping marks
 *
 * <h2>Characters @c ISpecialCharacterRenderer can render</h2>
 *
 * <em>C0 controls</em> include characters whose code point is U+0000..001F or U+007F. But U+0009,
 * U+000A, and U+000D are excluded. These characters can be found in "White space characters" and
 * "End of line".
 *
 * <em>C1 controls</em> include characters whose code point is U+0080..009F. But only U+0085 is
 * excluded. This is one of "End of line" character.
 *
 * <em>End of line</em> includes any NLFs in Unicode. Identified by @c kernel#Newline enumeration.
 *
 * <em>White space characters</em> include all Unicode white spaces and horizontal tab (U+0009). An
 * instance of @c ISpecialCharacterRenderer can't set the width of these glyphs.
 *
 * <em>Line wrapping marks</em> indicate a logical is wrapped visually. Note that this is not an
 * actual character.
 *
 * <h2>Process</h2>
 *
 * @c ISpecialCharacterRenderer will be invoked at the following two stages.
 * -# To layout a special character.
 * -# To draw a special character.
 *
 * (1) When layout of a line is needed, @c TextRenderer creates and initializes a @c LineLayout.
 * In this process, the widths of the all characters in the line are calculated by Unicode script
 * processor (Uniscribe). For the above special characters, @c LineLayout queries the widths to
 * @c ISpecialCharacterRenderer (However, for white spaces, this query is not performed).
 *
 * (2) When a line is drawn, @c LineLayout#draw calls @c ISpecialCharacterRenderer::drawXxxx
 * methods to draw special characters with the device context, the orientation, and the rectangle
 * to paint.
 *
 * @see TextRenderer, TextRenderer#setSpecialCharacterRenderer
 */

namespace {
	inline void getControlPresentationString(CodePoint c, Char* buffer) {
		buffer[0] = L'^';
		buffer[1] = (c != 0x7f) ? static_cast<Char>(c) + 0x40 : L'?';
	}
}

/**
 * @class ascension::graphics::font::DefaultSpecialCharacterRenderer
 *
 * Default implementation of @c ISpecialCharacterRenderer interface. This renders special
 * characters with the glyphs provided by the standard international font "Lucida Sans Unicode".
 * The mapping special characters to characters provide glyphs are as follows:
 * - Horizontal tab (LTR) : U+2192 Rightwards Arrow (&#x2192;)
 * - Horizontal tab (RTL) : U+2190 Leftwards Arrow (&#x2190;)
 * - Line terminator : U+2193 Downwards Arrow (&#x2193;)
 * - Line wrapping mark (LTR) : U+21A9 Leftwards Arrow With Hook (&#x21A9;)
 * - Line wrapping mark (RTL) : U+21AA Rightwards Arrow With Hook (&#x21AA;)
 * - White space : U+00B7 Middle Dot (&#x00B7;)
 *
 * Default foreground colors of glyphs are as follows:
 * - Control characters : RGB(0x80, 0x80, 0x00)
 * - Line terminators : RGB(0x00, 0x80, 0x80)
 * - Line wrapping markers: RGB(0x00, 0x80, 0x80)
 * - White space characters : RGB(0x00, 0x80, 0x80)
 */

/// Default constructor.
DefaultSpecialCharacterRenderer::DefaultSpecialCharacterRenderer() /*throw()*/ : renderer_(0),
		controlColor_(0x80, 0x80, 0x00), eolColor_(0x00, 0x80, 0x80), wrapMarkColor_(0x00, 0x80, 0x80),
		whiteSpaceColor_(0x00, 0x80, 0x80), showsEOLs_(true), showsWhiteSpaces_(true), font_() {
}

/// @see ISpecialCharacterRenderer#drawControlCharacter
void DefaultSpecialCharacterRenderer::drawControlCharacter(const DrawingContext& context, CodePoint c) const {
	const win32::Handle<HDC>& dc = context.renderingContext.nativeHandle();
	shared_ptr<const Font> primaryFont(renderer_->primaryFont());
	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), primaryFont->nativeHandle().get()));
	::SetTextColor(dc.get(), controlColor_.asCOLORREF());
	Char buffer[2];
	getControlPresentationString(c, buffer);
	::ExtTextOutW(dc.get(), context.rect.left(), context.rect.top() + primaryFont->metrics().ascent(), 0, 0, buffer, 2, 0);
	::SelectObject(dc.get(), oldFont);
}

/// @see ISpecialCharacterRenderer#drawLineTerminator
void DefaultSpecialCharacterRenderer::drawLineTerminator(const DrawingContext& context, k::Newline) const {
	if(showsEOLs_ && glyphs_[LINE_TERMINATOR] != 0xffffu) {
		const win32::Handle<HDC>& dc = context.renderingContext.nativeHandle();
		shared_ptr<const Font> primaryFont(renderer_->primaryFont());
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
			(((glyphWidths_[LINE_TERMINATOR] & 0x80000000ul) != 0) ? font_ : primaryFont)->nativeHandle().get()));
		::SetTextColor(dc.get(), eolColor_.asCOLORREF());
		::ExtTextOutW(dc.get(),
			context.rect.left(), context.rect.top() + primaryFont->metrics().ascent(),
			ETO_GLYPH_INDEX, 0, reinterpret_cast<const WCHAR*>(&glyphs_[LINE_TERMINATOR]), 1, 0);
		::SelectObject(dc.get(), oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawLineWrappingMark
void DefaultSpecialCharacterRenderer::drawLineWrappingMark(const DrawingContext& context) const {
	const int id = (context.readingDirection == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK;
	const WCHAR glyph = glyphs_[id];
	if(glyph != 0xffffu) {
		const win32::Handle<HDC>& dc = context.renderingContext.nativeHandle();
		shared_ptr<const Font> primaryFont(renderer_->primaryFont());
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
			(win32::boole(glyphWidths_[id] & 0x80000000ul) ? font_ : primaryFont)->nativeHandle().get()));
		::SetTextColor(dc.get(), wrapMarkColor_.asCOLORREF());
		::ExtTextOutW(dc.get(),
			context.rect.left(), context.rect.top() + primaryFont->metrics().ascent(),
			ETO_GLYPH_INDEX, 0, &glyph, 1, 0);
		::SelectObject(dc.get(), oldFont);
	}
}

/// @see ISpecialCharacterRenderer#drawWhiteSpaceCharacter
void DefaultSpecialCharacterRenderer::drawWhiteSpaceCharacter(const DrawingContext& context, CodePoint c) const {
	if(!showsWhiteSpaces_)
		return;
	else if(c == 0x0009u) {
		const int id = (context.readingDirection == LEFT_TO_RIGHT) ? LTR_HORIZONTAL_TAB : RTL_HORIZONTAL_TAB;
		const WCHAR glyph = glyphs_[id];
		if(glyph != 0xffffu) {
			const win32::Handle<HDC>& dc = context.renderingContext.nativeHandle();
			shared_ptr<const Font> primaryFont(renderer_->primaryFont());
			HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
				(win32::boole(glyphWidths_[id] & 0x80000000ul) ? font_ : primaryFont)->nativeHandle().get()));
			const int glyphWidth = glyphWidths_[id] & 0x7ffffffful;
			const int x =
				((context.readingDirection == LEFT_TO_RIGHT && glyphWidth < context.rect.width())
					|| (context.readingDirection == RIGHT_TO_LEFT && glyphWidth > context.rect.width())) ?
				context.rect.left() : context.rect.right() - glyphWidth;
			::SetTextColor(dc.get(), whiteSpaceColor_.asCOLORREF());
			::ExtTextOutW(dc.get(),
				x, context.rect.top() + primaryFont->metrics().ascent(),
				ETO_CLIPPED | ETO_GLYPH_INDEX, &toNative(context.rect), &glyph, 1, 0);
			::SelectObject(dc.get(), oldFont);
		}
	} else if(glyphs_[WHITE_SPACE] != 0xffffu) {
		const win32::Handle<HDC>& dc = context.renderingContext.nativeHandle();
		shared_ptr<const Font> primaryFont(renderer_->primaryFont());
		HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(),
			(win32::boole(glyphWidths_[WHITE_SPACE] & 0x80000000ul) ? font_ : primaryFont)->nativeHandle().get()));
		::SetTextColor(dc.get(), whiteSpaceColor_.asCOLORREF());
		::ExtTextOutW(dc.get(),
			(context.rect.left() + context.rect.right() - (glyphWidths_[WHITE_SPACE] & 0x7ffffffful)) / 2,
			context.rect.top() + primaryFont->metrics().ascent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &toNative(context.rect),
			reinterpret_cast<const WCHAR*>(&glyphs_[WHITE_SPACE]), 1, 0);
		::SelectObject(dc.get(), oldFont);
	}
}

/// @see IDefaultFontListener#defaultFontChanged
void DefaultSpecialCharacterRenderer::defaultFontChanged() {
	static const Char codes[] = {0x2192u, 0x2190u, 0x2193u, 0x21a9u, 0x21aau, 0x00b7u};

	// using the primary font
	win32::Handle<HDC> dc(::GetDC(0), bind1st(ptr_fun(&::ReleaseDC), static_cast<HWND>(0)));
	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), renderer_->primaryFont()->nativeHandle().get()));
	::GetGlyphIndicesW(dc.get(), codes, ASCENSION_COUNTOF(codes), glyphs_, GGI_MARK_NONEXISTING_GLYPHS);
	::GetCharWidthI(dc.get(), 0, ASCENSION_COUNTOF(codes), glyphs_, glyphWidths_);

	// using the fallback font
	font_.reset();
	if(find(glyphs_, ASCENSION_ENDOF(glyphs_), 0xffffu) != ASCENSION_ENDOF(glyphs_)) {
		LOGFONTW lf;
		::GetObjectW(renderer_->primaryFont()->nativeHandle().get(), sizeof(LOGFONTW), &lf);
		lf.lfWeight = FW_REGULAR;
		lf.lfItalic = lf.lfUnderline = lf.lfStrikeOut = false;
		wcscpy(lf.lfFaceName, L"Lucida Sans Unicode");
		font_.reset(::CreateFontIndirectW(&lf), &::DeleteObject);
		::SelectObject(dc.get(), font_.get()->nativeHandle().get());
		WORD g[ASCENSION_COUNTOF(glyphs_)];
		int w[ASCENSION_COUNTOF(glyphWidths_)];
		::GetGlyphIndicesW(dc.get(), codes, ASCENSION_COUNTOF(codes), g, GGI_MARK_NONEXISTING_GLYPHS);
		::GetCharWidthI(dc.get(), 0, ASCENSION_COUNTOF(codes), g, w);
		for(int i = 0; i < ASCENSION_COUNTOF(glyphs_); ++i) {
			if(glyphs_[i] == 0xffffu) {
				if(g[i] != 0xffff) {
					glyphs_[i] = g[i];
					glyphWidths_[i] = w[i] | 0x80000000ul;
				} else
					glyphWidths_[i] = 0;	// missing
			}
		}
	}

	::SelectObject(dc.get(), oldFont);
}

/// @see ISpecialCharacterRenderer#getControlCharacterWidth
int DefaultSpecialCharacterRenderer::getControlCharacterWidth(const LayoutContext& context, CodePoint c) const {
	Char buffer[2];
	getControlPresentationString(c, buffer);
	const win32::Handle<HDC>& dc = context.renderingContext.nativeHandle();
	HFONT oldFont = static_cast<HFONT>(::SelectObject(dc.get(), renderer_->primaryFont()->nativeHandle().get()));
	SIZE temp;
	::GetTextExtentPoint32W(dc.get(), buffer, 2, &temp);
	::SelectObject(dc.get(), oldFont);
	return temp.cx;
}

/// @see ISpecialCharacterRenderer#getLineTerminatorWidth
int DefaultSpecialCharacterRenderer::getLineTerminatorWidth(const LayoutContext&, k::Newline) const {
	return showsEOLs_ ? (glyphWidths_[LINE_TERMINATOR] & 0x7ffffffful) : 0;
}

/// @see ISpecialCharacterRenderer#getLineWrappingMarkWidth
int DefaultSpecialCharacterRenderer::getLineWrappingMarkWidth(const LayoutContext& context) const {
	return glyphWidths_[(context.readingDirection == LEFT_TO_RIGHT) ? LTR_WRAPPING_MARK : RTL_WRAPPING_MARK] & 0x7ffffffful;
}

/// @see ISpecialCharacterRenderer#install
void DefaultSpecialCharacterRenderer::install(TextRenderer& renderer) {
	(renderer_ = &renderer)->addDefaultFontListener(*this);
	defaultFontChanged();
}

/// @see ISpecialCharacterRenderer#uninstall
void DefaultSpecialCharacterRenderer::uninstall() {
	renderer_->removeDefaultFontListener(*this);
	renderer_ = 0;
}
