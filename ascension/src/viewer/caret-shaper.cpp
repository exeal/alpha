/**
 * @file caret-shaper.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-09-25 separated from viewer.cpp
 */

#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/caret-shaper.hpp>
#include <ascension/viewer/viewer.hpp>

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::viewers::base;
using namespace ascension::presentation;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;
namespace k = ascension::kernel;


inline NativeSize viewers::currentCharacterSize(const Caret& caret) {
	const TextRenderer& textRenderer = caret.textViewer().textRenderer();
	const Scalar extent = textRenderer.defaultFont()->metrics().cellHeight();
	Scalar measure;
	if(k::locations::isEndOfLine(caret))	// EOL
		measure = textRenderer.defaultFont()->metrics().averageCharacterWidth();
	else {
		const TextLayout& layout = textRenderer.layouts().at(caret.line());
		const Scalar leading = geometry::x(layout.location(caret.column(), TextLayout::LEADING));
		const Scalar trailing = geometry::x(layout.location(caret.column(), TextLayout::TRAILING));
		measure = static_cast<Scalar>(detail::distance(leading, trailing));
	}
	const bool horizontal = WritingModeBase::isHorizontal(caret.textViewer().textRenderer().writingMode().blockFlowDirection);
	return geometry::make<NativeSize>(horizontal ? measure : extent, horizontal ? extent : measure);
}


// CaretShapeUpdater //////////////////////////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param caret The caret
 */
CaretShapeUpdater::CaretShapeUpdater(const Caret& caret) /*throw()*/ : caret_(caret) {
}

/// Returns the caret.
const Caret& CaretShapeUpdater::caret() const /*throw()*/ {
	return caret_;
}

/// Notifies the text viewer to update the shape of the caret.
void CaretShapeUpdater::update() /*throw()*/ {
	viewer_.recreateCaret();	// $friendly-access
}


// DefaultCaretShaper /////////////////////////////////////////////////////////////////////////////

/// Constructor.
DefaultCaretShaper::DefaultCaretShaper() /*throw()*/ : caret_(0) {
}

/// @see CaretShaper#install
void DefaultCaretShaper::install(CaretShapeUpdater& updater) /*throw()*/ {
	caret_ = &updater.caret();
}

namespace {
	inline uint32_t packColor(const Color& color) {
		return (0xff << 12) | (color.red() << 8) | (color.green() << 4) | color.blue();
	}
	/**
	 * Creates the image for solid (rectangular) caret.
	 * @param width The width of the rectangle in pixels
	 * @param height The height of the rectangle in pixels
	 * @param color The color
	 * @return The image
	 */
	inline auto_ptr<Image> createSolidCaretImage(uint16_t width, uint16_t height, const Color& color) {
		const AutoBuffer<uint32_t> pattern(new uint32_t[width * height]);
		uninitialized_fill(pattern.get(), pattern.get() + (width * height), packColor(color));
		return auto_ptr<Image>(new Image(reinterpret_cast<uint8_t*>(
			pattern.get()), geometry::make<NativeSize>(width, height), Image::ARGB_32));
	}
}

/// @see CaretShaper#shape
void DefaultCaretShaper::shape(auto_ptr<Image>& image, NativePoint& alignmentPoint) const /*throw()*/ {
	Scalar measure;
#if defined(ASCENSION_OS_WINDOWS)
	DWORD width;
	if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
		width = 1;	// NT4 does not support SPI_GETCARETWIDTH
	measure = width;
#else
	// TODO: Write codes in other platforms.
#endif

	const TextRenderer& renderer = caret_->textViewer().textRenderer();
	const TextLayout& layout = renderer.layouts().at(caret_->line());
	const LineMetrics& lineMetrics = layout.lineMetrics(layout.lineAt(caret_->column()));
	const Scalar extent = lineMetrics.height();
	const bool horizontal = WritingModeBase::isHorizontal(layout.writingMode().blockFlowDirection);
	image = createSolidCaretImage(horizontal ? measure : extent, horizontal ? extent : measure, Color(0, 0, 0));
	switch(layout.writingMode().blockFlowDirection) {
		case WritingModeBase::HORIZONTAL_TB:
			geometry::x(alignmentPoint) = (layout.bidiEmbeddingLevel(caret_->column()) % 2 == 0) ? 0 : measure - 1;
			geometry::y(alignmentPoint) = lineMetrics.ascent();
			break;
		case WritingModeBase::VERTICAL_RL:
			// TODO: Not implemented.
			break;
		case WritingModeBase::VERTICAL_LR:
			// TODO: Not implemented.
			break;
		default:
			ASCENSION_ASSERT_NOT_REACHED();
	}
}

/// @see CaretShaper#uninstall
void DefaultCaretShaper::uninstall() /*throw()*/ {
	caret_ = 0;
}


// LocaleSensitiveCaretShaper /////////////////////////////////////////////////////////////////////

namespace {
	/// Returns @c true if the specified language is RTL.
	inline bool isRTLLanguage(LANGID id) /*throw()*/ {
		return id == LANG_ARABIC || id == LANG_FARSI || id == LANG_HEBREW || id == LANG_SYRIAC || id == LANG_URDU;
	}
	/// Returns @c true if the specified language is Thai or Lao.
	inline bool isTISLanguage(LANGID id) /*throw()*/ {
#ifndef LANG_LAO
		const LANGID LANG_LAO = 0x54;
#endif // !LANG_LAO
		return id == LANG_THAI || id == LANG_LAO;
	}
	/**
	 * Creates the bitmap for RTL caret.
	 * @param height The height of the image in pixels
	 * @param bold Set @c true to create a bold shape
	 * @param color The color
	 * @return The bitmap
	 */
	inline auto_ptr<Image> createRTLCaretImage(uint16_t height, bool bold, const Color& color) {
		const uint32_t white = 0, black = packColor(color);
		AutoBuffer<uint32_t> pattern(new uint32_t[5 * height]);
		assert(height > 3);
		uninitialized_fill(pattern.get(), pattern.get() + 5 * height, white);
		pattern[0] = pattern[1] = pattern[2] = pattern[6] = pattern[7] = pattern[12] = black;
		for(uint16_t i = 0; i < height; ++i) {
			pattern[i * 5 + 3] = black;
			if(bold)
				pattern[i * 5 + 4] = black;
		}
		return auto_ptr<Image>(new Image(reinterpret_cast<uint8_t*>(
			pattern.get()), geometry::make<NativeSize>(5, height), Image::ARGB_32));
	}
	/**
	 * Creates the bitmap for Thai or Lao caret.
	 * @param height The height of the image in pixels
	 * @param bold Set @c true to create a bold shape
	 * @param color The color
	 * @return The bitmap
	 */
	inline auto_ptr<Image> createTISCaretImage(uint16_t height, bool bold, const Color& color) {
		const uint32_t white = 0, black = packColor(color);
		const uint16_t width = max<uint16_t>(height / 8, 3);
		AutoBuffer<uint32_t> pattern(new uint32_t[width * height]);
		assert(height > 3);
		uninitialized_fill(pattern.get(), pattern.get() + width * height, white);
		for(uint16_t y = 0; y < height - 1; ++y) {
			pattern[y * width] = black;
			if(bold)
				pattern[y * width + 1] = black;
		}
		if(bold)
			for(uint16_t x = 2; x < width; ++x)
				pattern[width * (height - 2) + x] = black;
		for(uint16_t x = 0; x < width; ++x)
			pattern[width * (height - 1) + x] = black;
		return auto_ptr<Image>(new Image(reinterpret_cast<uint8_t*>(
			pattern.get()), geometry::make<NativeSize>(width, height), Image::ARGB_32));
	}
} // namespace @0

/// Constructor.
LocaleSensitiveCaretShaper::LocaleSensitiveCaretShaper(bool bold /* = false */) /*throw()*/ : updater_(0), bold_(bold) {
}

/// @see CaretListener#caretMoved
void LocaleSensitiveCaretShaper::caretMoved(const Caret& self, const k::Region&) {
	if(self.isOvertypeMode())
		updater_->update();
}

/// @see CaretShaper#install
void LocaleSensitiveCaretShaper::install(CaretShapeUpdater& updater) {
	updater_ = &updater;
}

/// @see CaretStateListener#matchBracketsChanged
void LocaleSensitiveCaretShaper::matchBracketsChanged(const Caret&, const std::pair<k::Position, k::Position>&, bool) {
}

/// @see CaretStateListener#overtypeModeChanged
void LocaleSensitiveCaretShaper::overtypeModeChanged(const Caret&) {
	updater_->update();
}

/// @see CaretShapeListener#selectionShapeChanged
void LocaleSensitiveCaretShaper::selectionShapeChanged(const Caret&) {
}

/// @see CaretShaper#shape
void LocaleSensitiveCaretShaper::shape(auto_ptr<Image>& image, NativePoint& alignmentPoint) const /*throw()*/ {
	const Caret& caret = updater_->textViewer().caret();
	const bool overtype = caret.isOvertypeMode() && isSelectionEmpty(caret);

	if(!overtype) {
		geometry::dx(solidSize) = bold_ ? 2 : 1;	// this ignores the system setting...
		geometry::dy(solidSize) = updater_->textViewer().textRenderer().defaultFont()->metrics().cellHeight();
	} else	// use the width of the glyph when overtype mode
		solidSize = currentCharacterSize(updater_->textViewer());
	readingDirection = LEFT_TO_RIGHT;

	HIMC imc = ::ImmGetContext(updater_->textViewer().identifier().get());
	const bool imeOpened = win32::boole(::ImmGetOpenStatus(imc));
	::ImmReleaseContext(updater_->textViewer().identifier().get(), imc);
	auto_ptr<Image> bitmap;
	if(imeOpened) {	// CJK and IME is open
		static const Color red(0x80, 0x00, 0x00);
		bitmap = createSolidCaretBitmap(
			static_cast<uint16_t>(geometry::dx(solidSize)), static_cast<uint16_t>(geometry::dy(solidSize)), red);
	} else if(!overtype && geometry::dy(solidSize) > 3) {
		static const Color black(0x00, 0x00, 0x00);
		const WORD langID = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
		if(isRTLLanguage(langID)) {	// RTL
			bitmap = createRTLCaretBitmap(static_cast<uint16_t>(geometry::dy(solidSize)), bold_, black);
			readingDirection = RIGHT_TO_LEFT;
		} else if(isTISLanguage(langID)) {	// Thai relations
			bitmap = createTISCaretBitmap(static_cast<uint16_t>(geometry::dy(solidSize)), bold_, black);
		}
	}
	image = *bitmap.release();
}

/// @see TextViewerInputStatusListener#textViewerIMEOpenStatusChanged
void LocaleSensitiveCaretShaper::textViewerIMEOpenStatusChanged() /*throw()*/ {
	updater_->update();
}

/// @see TextViewerInputStatusListener#textViewerInputLanguageChanged
void LocaleSensitiveCaretShaper::textViewerInputLanguageChanged() /*throw()*/ {
	updater_->update();
}

/// @see CaretShapeProvider#uninstall
void LocaleSensitiveCaretShaper::uninstall() {
	updater_ = 0;
}
