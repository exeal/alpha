/**
 * @file default-caret-shaper.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-09-25 separated from viewer.cpp
 * @date 2013-04-21 separated from caret-shaper.cpp
 */

#include <ascension/graphics/image.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/default-caret-shaper.hpp>
#include <ascension/viewer/viewer.hpp>

using namespace ascension;
using namespace ascension::viewers;
using namespace ascension::presentation;
using namespace ascension::graphics;
using namespace ascension::graphics::font;
using namespace std;
namespace k = ascension::kernel;


// DefaultCaretShaper /////////////////////////////////////////////////////////////////////////////

/// Constructor.
DefaultCaretShaper::DefaultCaretShaper() BOOST_NOEXCEPT : updater_(nullptr) {
}

/// @see CaretListener#caretMoved
void DefaultCaretShaper::caretMoved(const Caret& caret, const k::Region& oldRegion) {
	if(updater_ != nullptr) {
		assert(&updater_->caret() == &caret);	// sanity check...
		if(line(caret) != oldRegion.second.line)
			updater_->update();
	}
}

/// @see graphics#font#ComputedBlockFlowDirectionListener#computedBlockFlowDirectionChanged
void DefaultCaretShaper::computedBlockFlowDirectionChanged(BlockFlowDirection used) {
	if(updater_ != nullptr)
		updater_->update();
}

/// @see CaretShaper#install
void DefaultCaretShaper::install(CaretShapeUpdater& updater) BOOST_NOEXCEPT {
	updater_ = &updater;
	updater_->caret().addListener(*this);
	updater_->caret().textViewer().textRenderer().addComputedBlockFlowDirectionListener(*this);
	updater_->caret().textViewer().textRenderer().layouts().addVisualLinesListener(*this);
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
	inline unique_ptr<Image> createSolidCaretImage(uint16_t width, uint16_t height, const Color& color) {
		const unique_ptr<uint32_t[]> pattern(new uint32_t[width * height]);
		uninitialized_fill(pattern.get(), pattern.get() + (width * height), packColor(color));
		return unique_ptr<Image>(new Image(reinterpret_cast<uint8_t*>(pattern.get()),
			geometry::BasicDimension<uint16_t>(geometry::_dx = width, geometry::_dy = height), Image::ARGB_32));
	}
	inline uint16_t systemDefinedCaretMeasure() {
#if defined(ASCENSION_OS_WINDOWS)
		DWORD width;
		if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
			width = 1;	// NT4 does not support SPI_GETCARETWIDTH
		return static_cast<uint16_t>(width);
#else
		// TODO: Write codes in other platforms.
#endif
	}
	/// Returns @c true if the specified language is RTL.
	inline bool isRTLLanguage(LANGID id) BOOST_NOEXCEPT {
		return id == LANG_ARABIC || id == LANG_FARSI || id == LANG_HEBREW || id == LANG_SYRIAC || id == LANG_URDU;
	}
	/// Returns @c true if the specified language is Thai or Lao.
	inline bool isTISLanguage(LANGID id) BOOST_NOEXCEPT {
#ifndef LANG_LAO
		const LANGID LANG_LAO = 0x54;
#endif // !LANG_LAO
		return id == LANG_THAI || id == LANG_LAO;
	}
	/**
	 * Creates the bitmap for RTL caret.
	 * @param extent The extent (height) of the image in pixels
	 * @param color The color
	 * @return The bitmap
	 */
	inline unique_ptr<Image> createRTLCaretImage(uint16_t extent, const Color& color) {
		const uint32_t white = 0, black = packColor(color);
		unique_ptr<uint32_t[]> pattern(new uint32_t[5 * extent]);
		assert(extent > 3);
		uninitialized_fill(pattern.get(), pattern.get() + 5 * extent, white);
		pattern[0] = pattern[1] = pattern[2] = pattern[6] = pattern[7] = pattern[12] = black;
		for(uint16_t i = 0; i < extent; ++i) {
			pattern[i * 5 + 3] = black;
//			if(bold)
//				pattern[i * 5 + 4] = black;
		}
		return unique_ptr<Image>(new Image(reinterpret_cast<uint8_t*>(pattern.get()),
			geometry::BasicDimension<uint16_t>(geometry::_dx = 5, geometry::_dy = extent), Image::ARGB_32));
	}
	/**
	 * Creates the bitmap for Thai or Lao caret.
	 * @param extent The extent (height) of the image in pixels
	 * @param color The color
	 * @return The bitmap
	 */
	inline unique_ptr<Image> createTISCaretImage(uint16_t extent, const Color& color) {
		const uint32_t white = 0, black = packColor(color);
		const uint16_t width = max<uint16_t>(extent / 8, 3);
		unique_ptr<uint32_t[]> pattern(new uint32_t[width * extent]);
		assert(extent > 3);
		uninitialized_fill(pattern.get(), pattern.get() + width * extent, white);
		for(uint16_t y = 0; y < extent - 1; ++y) {
			pattern[y * width] = black;
//			if(bold)
//				pattern[y * width + 1] = black;
		}
//		if(bold)
//			for(uint16_t x = 2; x < width; ++x)
//				pattern[width * (extent - 2) + x] = black;
		for(uint16_t x = 0; x < width; ++x)
			pattern[width * (extent - 1) + x] = black;
		return unique_ptr<Image>(new Image(reinterpret_cast<uint8_t*>(pattern.get()),
			geometry::BasicDimension<uint16_t>(geometry::_dx = width, geometry::_dy = extent), Image::ARGB_32));
	}

	void shapeCaret(const Caret& caret, bool localeSensitive, unique_ptr<Image>& image, Point& alignmentPoint) {
		const bool overtype = caret.isOvertypeMode() && isSelectionEmpty(caret);
		const TextRenderer& renderer = caret.textViewer().textRenderer();
		const TextLayout& layout = renderer.layouts().at(line(caret));

		graphics::Rectangle bounds(currentCharacterLogicalBounds(caret));
		if(!localeSensitive || !overtype) {
			const uint16_t advance = systemDefinedCaretMeasure();
			FlowRelativeFourSides<Scalar> temp(mapPhysicalToFlowRelative(layout.writingMode(), PhysicalFourSides<Scalar>(bounds)));
			temp.end() = temp.start() + advance;
			bounds = geometry::make<graphics::Rectangle>(mapFlowRelativeToPhysical(layout.writingMode(), temp));
		}

		static const Color black(0, 0, 0);
		if(localeSensitive) {
			win32::Handle<HIMC>::Type imc(win32::inputMethod(caret.textViewer()));
			if(win32::boole(::ImmGetOpenStatus(imc.get()))) {
				static const Color red(0x80, 0x00, 0x00);
				image = createSolidCaretImage(static_cast<uint16_t>(geometry::dx(bounds)), static_cast<uint16_t>(geometry::dy(bounds)), red);
				return;
//			} else if(isHorizontal(layout.writingMode().blockFlowDirection)) {
//				const WORD language = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
//				if(isRTLLanguage(language)) {	// RTL
//					image = createRTLCaretImage(extent, black);
//					return;
//				} else if(isTISLanguage(language)) {	// Thai relations
//					image = createTISCaretImage(extent, black);
//					return;
//				}
			}
		}
		image = createSolidCaretImage(static_cast<uint16_t>(geometry::dx(bounds)), static_cast<uint16_t>(geometry::dy(bounds)), black);
	}
}

/// @see CaretShaper#shape
void DefaultCaretShaper::shape(unique_ptr<Image>& image, Point& alignmentPoint) const BOOST_NOEXCEPT {
	return shapeCaret(updater_->caret(), false, image, alignmentPoint);
}

/// @see CaretShaper#uninstall
void DefaultCaretShaper::uninstall() BOOST_NOEXCEPT {
	updater_->caret().removeListener(*this);
	updater_->caret().textViewer().textRenderer().removeComputedBlockFlowDirectionListener(*this);
	updater_->caret().textViewer().textRenderer().layouts().removeVisualLinesListener(*this);
	updater_ = nullptr;
}

/// @see CaretShaper#visualLinesModified
void DefaultCaretShaper::visualLinesDeleted(const boost::integer_range<Index>&, Index, bool) BOOST_NOEXCEPT {
}

/// @see CaretShaper#visualLinesModified
void DefaultCaretShaper::visualLinesInserted(const boost::integer_range<Index>& lines) BOOST_NOEXCEPT {
}

/// @see CaretShaper#visualLinesModified
void DefaultCaretShaper::visualLinesModified(const boost::integer_range<Index>& lines, SignedIndex, bool, bool) BOOST_NOEXCEPT {
	if(updater_ != nullptr && includes(lines, line(updater_->caret())))
		updater_->update();
}


// LocaleSensitiveCaretShaper /////////////////////////////////////////////////////////////////////

namespace {
} // namespace @0

/// @see CaretListener#caretMoved
void LocaleSensitiveCaretShaper::caretMoved(const Caret& caret, const k::Region& oldRegion) {
	if(updater() != nullptr && caret.isOvertypeMode())
		updater()->update();
	else
		DefaultCaretShaper::caretMoved(caret, oldRegion);
}

/// @see TextViewerInputStatusListener#inputLocaleChanged
void LocaleSensitiveCaretShaper::inputLocaleChanged() BOOST_NOEXCEPT {
	if(updater() != nullptr)
		updater()->update();
}

/// @see InputPropertyListener#inputMethodOpenStatusChanged
void LocaleSensitiveCaretShaper::inputMethodOpenStatusChanged() BOOST_NOEXCEPT {
	if(updater() != nullptr)
		updater()->update();
}

/// @see CaretShaper#install
void LocaleSensitiveCaretShaper::install(CaretShapeUpdater& updater) {
	DefaultCaretShaper::install(updater);
	updater.caret().addStateListener(*this);
	updater.caret().addInputPropertyListener(*this);
}

/// @see CaretStateListener#matchBracketsChanged
void LocaleSensitiveCaretShaper::matchBracketsChanged(const Caret&, const std::pair<k::Position, k::Position>&, bool) {
}

/// @see CaretStateListener#overtypeModeChanged
void LocaleSensitiveCaretShaper::overtypeModeChanged(const Caret&) {
	if(updater() != nullptr)
		updater()->update();
}

/// @see CaretShapeListener#selectionShapeChanged
void LocaleSensitiveCaretShaper::selectionShapeChanged(const Caret&) {
}

/// @see CaretShaper#shape
void LocaleSensitiveCaretShaper::shape(unique_ptr<Image>& image, Point& alignmentPoint) const BOOST_NOEXCEPT {
	return shapeCaret(updater()->caret(), true, image, alignmentPoint);
}

/// @see CaretShapeProvider#uninstall
void LocaleSensitiveCaretShaper::uninstall() {
	assert(updater() != nullptr);
	updater()->caret().removeStateListener(*this);
	updater()->caret().removeInputPropertyListener(*this);
	DefaultCaretShaper::uninstall();
}
