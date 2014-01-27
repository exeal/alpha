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
#ifdef ASCENSION_WINDOW_SYSTEM_GTK
#	include <gtkmm/settings.h>
#endif

namespace ascension {
	namespace viewers {

		// DefaultCaretShaper /////////////////////////////////////////////////////////////////////////////////////////

		/// Constructor.
		DefaultCaretShaper::DefaultCaretShaper() BOOST_NOEXCEPT : updater_(nullptr) {
		}

		/// @see CaretListener#caretMoved
		void DefaultCaretShaper::caretMoved(const Caret& caret, const kernel::Region& regionBeforeMotion) {
			if(updater_ != nullptr) {
				assert(&updater_->caret() == &caret);	// sanity check...
				if(kernel::line(caret) != regionBeforeMotion.second.line)
					updater_->update();
			}
		}

		/// @see graphics#font#ComputedBlockFlowDirectionListener#computedBlockFlowDirectionChanged
		void DefaultCaretShaper::computedBlockFlowDirectionChanged(presentation::BlockFlowDirection used) {
			if(updater_ != nullptr)
				updater_->update();
		}

		/// @see CaretShaper#install
		void DefaultCaretShaper::install(CaretShapeUpdater& updater) BOOST_NOEXCEPT {
			updater_ = &updater;
			caretMotionConnection_ = updater.caret().motionSignal().connect(
				std::bind(&DefaultCaretShaper::caretMoved, this, std::placeholders::_1, std::placeholders::_2));
			updater_->caret().textViewer().textRenderer().addComputedBlockFlowDirectionListener(*this);
			updater_->caret().textViewer().textRenderer().layouts().addVisualLinesListener(*this);
		}

		namespace {
			inline std::uint32_t packColor(const graphics::Color& color) BOOST_NOEXCEPT {
				return (0xff << 12) | (color.red() << 8) | (color.green() << 4) | color.blue();
			}

			/**
			 * Creates the image for solid (rectangular) caret.
			 * @param width The width of the rectangle in pixels
			 * @param height The height of the rectangle in pixels
			 * @param color The color
			 * @return The image
			 */
			inline std::unique_ptr<graphics::Image> createSolidCaretImage(std::uint16_t width, std::uint16_t height, const graphics::Color& color) {
				const graphics::Image::Format format = graphics::Image::ARGB32;
				const std::uint32_t size = graphics::Image::stride(width, format) * height;
				std::unique_ptr<std::uint8_t[]> pattern(new std::uint8_t[size]);
				std::uninitialized_fill(reinterpret_cast<std::uint32_t*>(pattern.get()), reinterpret_cast<std::uint32_t*>(pattern.get() + size), packColor(color));
				return std::unique_ptr<graphics::Image>(new graphics::Image(std::move(pattern),
					graphics::geometry::BasicDimension<std::uint32_t>(graphics::geometry::_dx = width, graphics::geometry::_dy = height), format));
			}

			inline std::uint16_t systemDefinedCaretMeasure() {
#if defined(ASCENSION_OS_WINDOWS)
				DWORD width;
				if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
					width = 1;	// NT4 does not support SPI_GETCARETWIDTH
				return static_cast<std::uint16_t>(width);
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
			inline std::unique_ptr<graphics::Image> createRTLCaretImage(std::uint16_t extent, const graphics::Color& color) {
				const std::uint32_t white = 0, black = packColor(color);
				const graphics::Image::Format format = graphics::Image::ARGB32;
				const std::uint32_t measure = 5;	// width
				const std::uint32_t size = graphics::Image::stride(measure, format);
				std::unique_ptr<std::uint8_t[]> pattern(new std::uint8_t[size]);
				assert(extent > 3);
				std::uint32_t* const pattern32 = reinterpret_cast<std::uint32_t*>(pattern.get());
				std::uninitialized_fill(pattern32, reinterpret_cast<std::uint32_t*>(pattern.get() + size), white);
				pattern32[0] = pattern32[1] = pattern32[2] = pattern32[6] = pattern32[7] = pattern32[12] = black;
				for(std::uint16_t i = 0; i < extent; ++i) {
					pattern32[i * measure + 3] = black;
//					if(bold)
//						pattern[i * measure + 4] = black;
				}
				return std::unique_ptr<graphics::Image>(new graphics::Image(std::move(pattern),
					graphics::geometry::BasicDimension<std::uint32_t>(graphics::geometry::_dx = measure, graphics::geometry::_dy = extent), format));
			}

			/**
			 * Creates the bitmap for Thai or Lao caret.
			 * @param extent The extent (height) of the image in pixels
			 * @param color The color
			 * @return The bitmap
			 */
			inline std::unique_ptr<graphics::Image> createTISCaretImage(std::uint16_t extent, const graphics::Color& color) {
				const std::uint32_t white = 0, black = packColor(color);
				const graphics::Image::Format format = graphics::Image::ARGB32;
				const std::uint32_t measure = std::max<std::uint16_t>(extent / 8, 3);	// width
				const std::uint32_t size = graphics::Image::stride(measure, format);
				std::unique_ptr<std::uint8_t[]> pattern(new std::uint8_t[size]);
				assert(extent > 3);
				std::uint32_t* const pattern32 = reinterpret_cast<std::uint32_t*>(pattern.get());
				std::uninitialized_fill(pattern32, reinterpret_cast<std::uint32_t*>(pattern.get() + size), white);
				for(std::uint16_t y = 0; y < extent - 1; ++y) {
					pattern32[y * measure] = black;
//					if(bold)
//						pattern32[y * measure + 1] = black;
				}
//				if(bold)
//					for(std::uint16_t x = 2; x < measure; ++x)
//						pattern32[measure * (extent - 2) + x] = black;
				for(std::uint16_t x = 0; x < measure; ++x)
					pattern32[measure * (extent - 1) + x] = black;
				return std::unique_ptr<graphics::Image>(new graphics::Image(std::move(pattern),
					graphics::geometry::BasicDimension<std::uint32_t>(graphics::geometry::_dx = measure, graphics::geometry::_dy = extent), format));
			}

			void shapeCaret(const Caret& caret, bool localeSensitive, std::unique_ptr<graphics::Image>& image, graphics::geometry::BasicPoint<std::uint32_t>& alignmentPoint) {
				const bool overtype = caret.isOvertypeMode() && isSelectionEmpty(caret);
				const graphics::font::TextRenderer& renderer = caret.textViewer().textRenderer();

				if(const graphics::font::TextLayout* const layout = renderer.layouts().at(kernel::line(caret))) {
					boost::optional<graphics::Rectangle> bounds(currentCharacterLogicalBounds(caret));
					assert(bounds);
					if(!localeSensitive || !overtype) {
						const std::uint16_t advance = systemDefinedCaretMeasure();
						presentation::FlowRelativeFourSides<graphics::Scalar> temp(
							mapPhysicalToFlowRelative(layout->writingMode(), graphics::PhysicalFourSides<graphics::Scalar>(*bounds)));
						temp.end() = temp.start() + advance;
						boost::geometry::assign(*bounds, graphics::geometry::make<graphics::Rectangle>(presentation::mapFlowRelativeToPhysical(layout->writingMode(), temp)));
					}

					if(localeSensitive) {
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
						const bool inputMethodIsOpen = static_cast<Glib::ustring>(
							const_cast<TextViewer&>(caret.textViewer()).get_settings()->property_gtk_im_module()) != nullptr;
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
						win32::Handle<HIMC>::Type imc(win32::inputMethod(caret.textViewer()));
						const bool inputMethodIsOpen = win32::boole(::ImmGetOpenStatus(imc.get()));
#else
						ASCENSION_CANT_DETECT_PLATFORM();
#endif
						if(inputMethodIsOpen) {
							static const graphics::Color red(0x80, 0x00, 0x00);
							image = createSolidCaretImage(
								static_cast<std::uint16_t>(graphics::geometry::dx(*bounds)),
								static_cast<std::uint16_t>(graphics::geometry::dy(*bounds)), red);
							return;
//						} else if(isHorizontal(layout->writingMode().blockFlowDirection)) {
//							const WORD language = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
//							if(isRTLLanguage(language)) {	// RTL
//								image = createRTLCaretImage(extent, black);
//								return;
//							} else if(isTISLanguage(language)) {	// Thai relations
//								image = createTISCaretImage(extent, black);
//								return;
//							}
						}
					}
					image = createSolidCaretImage(static_cast<std::uint16_t>(graphics::geometry::dx(*bounds)),
						static_cast<std::uint16_t>(graphics::geometry::dy(*bounds)), graphics::Color::OPAQUE_BLACK);
					boost::geometry::assign(alignmentPoint, graphics::geometry::negate(graphics::geometry::topLeft(*bounds)));
				} else {
					image = createSolidCaretImage(0, 0, graphics::Color::OPAQUE_BLACK);
					boost::geometry::assign_zero(alignmentPoint);
				}
			}
		}

		/// @see CaretShaper#shape
		void DefaultCaretShaper::shape(std::unique_ptr<graphics::Image>& image, graphics::geometry::BasicPoint<std::uint32_t>& alignmentPoint) const BOOST_NOEXCEPT {
			return shapeCaret(updater_->caret(), false, image, alignmentPoint);
		}

		/// @see CaretShaper#uninstall
		void DefaultCaretShaper::uninstall() BOOST_NOEXCEPT {
			caretMotionConnection_.disconnect();
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


		// LocaleSensitiveCaretShaper /////////////////////////////////////////////////////////////////////////////////

		namespace {
		} // namespace @0

		/// @see CaretListener#caretMoved
		void LocaleSensitiveCaretShaper::caretMoved(const Caret& caret, const kernel::Region& regionBeforeMotion) {
			if(updater() != nullptr && caret.isOvertypeMode())
				updater()->update();
			else
				DefaultCaretShaper::caretMoved(caret, regionBeforeMotion);
		}

		/// @see TextViewerInputStatusListener#inputLocaleChanged
		void LocaleSensitiveCaretShaper::inputLocaleChanged(const Caret&) BOOST_NOEXCEPT {
			if(updater() != nullptr)
				updater()->update();
		}

		/// @see InputPropertyListener#inputMethodOpenStatusChanged
		void LocaleSensitiveCaretShaper::inputMethodOpenStatusChanged(const Caret&) BOOST_NOEXCEPT {
			if(updater() != nullptr)
				updater()->update();
		}

		/// @see CaretShaper#install
		void LocaleSensitiveCaretShaper::install(CaretShapeUpdater& updater) {
			DefaultCaretShaper::install(updater);
			caretOvertypeModeChangedConnection_ = updater.caret().overtypeModeChangedSignal().connect(
				std::bind(&LocaleSensitiveCaretShaper::overtypeModeChanged, this, std::placeholders::_1));
			inputLocaleChangedConnection_ = updater.caret().inputLocaleChangedSignal().connect(
				std::bind(&LocaleSensitiveCaretShaper::inputLocaleChanged, this, std::placeholders::_1));
			inputMethodOpenStatusChangedConnection_ = updater.caret().inputMethodOpenStatusChangedSignal().connect(
				std::bind(&LocaleSensitiveCaretShaper::inputMethodOpenStatusChanged, this, std::placeholders::_1));
		}

		/// @see CaretStateListener#overtypeModeChanged
		void LocaleSensitiveCaretShaper::overtypeModeChanged(const Caret&) {
			if(updater() != nullptr)
				updater()->update();
		}

		/// @see CaretShaper#shape
		void LocaleSensitiveCaretShaper::shape(std::unique_ptr<graphics::Image>& image, graphics::geometry::BasicPoint<std::uint32_t>& alignmentPoint) const BOOST_NOEXCEPT {
			return shapeCaret(updater()->caret(), true, image, alignmentPoint);
		}

		/// @see CaretShapeProvider#uninstall
		void LocaleSensitiveCaretShaper::uninstall() {
			assert(updater() != nullptr);
			caretOvertypeModeChangedConnection_.disconnect();
			inputLocaleChangedConnection_.disconnect();
			inputMethodOpenStatusChangedConnection_.disconnect();
			DefaultCaretShaper::uninstall();
		}
	}
}
