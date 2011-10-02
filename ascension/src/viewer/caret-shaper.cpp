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


inline NativeSize viewers::currentCharacterSize(const TextViewer& viewer) {
	const Scalar cy = viewer.textRenderer().defaultFont()->metrics().cellHeight();
	const Caret& caret = viewer.caret();
	if(k::locations::isEndOfLine(caret))	// EOL
		return geometry::make<NativeSize>(viewer.textRenderer().defaultFont()->metrics().averageCharacterWidth(), cy);
	else {
		const TextLayout& layout = viewer.textRenderer().layouts().at(caret.line());
		const Scalar leading = geometry::x(layout.location(caret.column(), TextLayout::LEADING));
		const Scalar trailing = geometry::x(layout.location(caret.column(), TextLayout::TRAILING));
		return geometry::make<NativeSize>(static_cast<Scalar>(detail::distance(leading, trailing)), cy);
	}
}


// CaretShapeUpdater ////////////////////////////////////////////////////////

/**
 * Private constructor.
 * @param viewer the text viewer
 */
CaretShapeUpdater::CaretShapeUpdater(TextViewer& viewer) /*throw()*/ : viewer_(viewer) {
}

/// Notifies the text viewer to update the shape of the caret.
void CaretShapeUpdater::update() /*throw()*/ {
	viewer_.recreateCaret();	// $friendly-access
}

/// Returns the text viewer.
TextViewer& CaretShapeUpdater::textViewer() /*throw()*/ {
	return viewer_;
}


// DefaultCaretShaper ///////////////////////////////////////////////////////

/// Constructor.
DefaultCaretShaper::DefaultCaretShaper() /*throw()*/ : viewer_(0) {
}

/// @see CaretShaper#install
void DefaultCaretShaper::install(CaretShapeUpdater& updater) /*throw()*/ {
	viewer_ = &updater.textViewer();
}

/// @see CaretShaper#shape
void DefaultCaretShaper::shape(Image&, NativeSize& solidSize, ReadingDirection& readingDirection) /*throw()*/ {
	DWORD width;
	if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
		width = 1;	// NT4 does not support SPI_GETCARETWIDTH
	solidSize = geometry::make<NativeSize>(width, viewer_->textRenderer().defaultFont()->metrics().cellHeight());
	readingDirection = LEFT_TO_RIGHT;	// no matter
}

/// @see CaretShaper#uninstall
void DefaultCaretShaper::uninstall() /*throw()*/ {
	viewer_ = 0;
}


// LocaleSensitiveCaretShaper ///////////////////////////////////////////////

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
	inline uint32_t packColor(const Color& color) {
		return (0xff << 12) | (color.red() << 8) | (color.green() << 4) | color.blue();
	}
	/**
	 * Creates the bitmap for solid caret.
	 * @param width The width of the rectangle in pixels
	 * @param height The height of the rectangle in pixels
	 * @param color The color
	 * @return The bitmap
	 */
	inline auto_ptr<Image> createSolidCaretBitmap(uint16_t width, uint16_t height, const Color& color) {
		const AutoBuffer<uint32_t> pattern(new uint32_t[width * height]);
		uninitialized_fill(pattern.get(), pattern.get() + (width * height), packColor(color));
		return auto_ptr<Image>(new Image(reinterpret_cast<uint8_t*>(
			pattern.get()), geometry::make<NativeSize>(width, height), Image::ARGB_32));
	}
	/**
	 * Creates the bitmap for RTL caret.
	 * @param height The height of the image in pixels
	 * @param bold Set @c true to create a bold shape
	 * @param color The color
	 * @return The bitmap
	 */
	inline auto_ptr<Image> createRTLCaretBitmap(uint16_t height, bool bold, const Color& color) {
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
	inline auto_ptr<Image> createTISCaretBitmap(uint16_t height, bool bold, const Color& color) {
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
void LocaleSensitiveCaretShaper::shape(
		Image& image, NativeSize& solidSize, ReadingDirection& readingDirection) /*throw()*/ {
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
