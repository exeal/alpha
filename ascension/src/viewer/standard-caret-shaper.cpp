/**
 * @file standard-caret-shaper.cpp
 * @author exeal
 * @date 2003-2006 was EditView.cpp and EditViewWindowMessages.cpp
 * @date 2006-2011 was viewer.cpp
 * @date 2011-09-25 separated from viewer.cpp
 * @date 2013-04-21 separated from caret-shaper.cpp
 * @date 2017-01-18 Renamed from default-caret-shaper.cpp.
 */

#include <ascension/graphics/font/line-layout-vector.hpp>
#include <ascension/graphics/font/text-layout.hpp>
#include <ascension/graphics/font/text-renderer.hpp>
#include <ascension/graphics/geometry/point-xy.hpp>
#include <ascension/graphics/geometry/rectangle-corners.hpp>
#include <ascension/graphics/geometry/algorithms/scale.hpp>
#include <ascension/graphics/geometry/algorithms/size.hpp>
#include <ascension/graphics/image.hpp>
#include <ascension/presentation/writing-mode-mappings.hpp>
#include <ascension/viewer/caret.hpp>
#include <ascension/viewer/standard-caret-shaper.hpp>
#include <ascension/viewer/text-area.hpp>
#include <ascension/viewer/text-viewer.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/settings.h>
#endif

namespace ascension {
	namespace viewer {
		namespace {
#if BOOST_OS_WINDOWS
			/// Returns @c true if the specified language is RTL.
			inline BOOST_CONSTEXPR bool isRtlLanguage(LANGID id) BOOST_NOEXCEPT {
				return id == LANG_ARABIC || id == LANG_FARSI || id == LANG_HEBREW || id == LANG_SYRIAC || id == LANG_URDU;
			}

			/// Returns @c true if the specified language is Thai or Lao.
			inline BOOST_CONSTEXPR bool isTisLanguage(LANGID id) BOOST_NOEXCEPT {
#ifndef LANG_LAO
#	define LANGID LANG_LAO 0x54
#endif // !LANG_LAO
				return id == LANG_THAI || id == LANG_LAO;
			}
#endif // BOOST_OS_WINDOWS

			/// @internal Returns measure in piexels of the system setting.
			inline std::uint32_t systemDefinedCaretMeasure() {
#if BOOST_OS_WINDOWS
				DWORD width;
				if(::SystemParametersInfo(SPI_GETCARETWIDTH, 0, &width, 0) == 0)
					width = 1;	// NT4 does not support SPI_GETCARETWIDTH
				return static_cast<std::uint32_t>(width);
#else
				// TODO: Write codes in other platforms.
				return 1;
#endif
			}
		}

		/// @see Caret#MotionSignal
		void StandardCaretShaper::caretMoved(const Caret& caret, const SelectedRegion& regionBeforeMotion) {
			if(caret.isOvertypeMode() || kernel::line(caret) != kernel::line(insertionPosition(caret.document(), regionBeforeMotion.caret())))
				signalStaticShapeChanged(caret);
		}

		namespace {
			inline BOOST_CONSTEXPR std::uint32_t packColor(const graphics::Color& color) BOOST_NOEXCEPT {
				return (0xff << 12) | (color.red() << 8) | (color.green() << 4) | color.blue();
			}

			/**
			 * @internal Creates the image for solid (rectangular) caret.
			 * @param bounds The size of the rectangle in pixels
			 * @param color The color
			 * @return The image
			 */
			inline std::shared_ptr<const graphics::Image> createSolidCaretImage(const graphics::geometry::BasicDimension<std::uint32_t>& bounds, const graphics::Color& color) {
				static std::list<std::tuple<
					const std::decay<decltype(bounds)>::type,
					const std::decay<decltype(color)>::type,
					const std::shared_ptr<const graphics::Image>
				>> cache;
				static const graphics::Image::Format format = graphics::Image::ARGB32;
				const std::uint32_t stride = graphics::Image::stride(graphics::geometry::dx(bounds), format);

				// check cache
				const std::decay<decltype(bounds)>::type alignedBounds(graphics::geometry::_dx = stride, graphics::geometry::_dy = graphics::geometry::dy(bounds));
				for(auto i(std::begin(cache)), e(std::end(cache)); i != e; ++i) {
					if(std::get<0>(*i) == alignedBounds && std::get<1>(*i) == color) {
						if(i != std::begin(cache)) {	// bring to front
							const auto temp(*i);
							cache.erase(i);
							cache.push_front(*i);
						}
						return std::get<2>(*std::begin(cache));
					}
				}

				const std::uint32_t size = stride * graphics::geometry::dy(bounds);
				std::unique_ptr<std::uint8_t[]> pattern(new std::uint8_t[size]);
				std::uninitialized_fill(reinterpret_cast<std::uint32_t*>(pattern.get()), reinterpret_cast<std::uint32_t*>(pattern.get() + size), packColor(color));
				std::iterator_traits<decltype(std::begin(cache))>::value_type newEntry(
					std::make_tuple(
						alignedBounds, color,
						std::make_shared<const graphics::Image>(std::move(pattern), bounds, format)));
				cache.push_front(newEntry);
				if(cache.size() > 32)
					cache.pop_back();
				return std::get<2>(newEntry);
			}

			/**
			 * @internal Creates a solid (rectangular) caret shape with the given color and measure.
			 * @param caret The caret to shape
			 * @param color The color of the image
			 * @param measure The measure of image in pixels. If @c boost#none, the system setting is used
			 * @return The shape
			 */
			CaretShaper::Shape createSolidShape(const Caret& caret, const boost::optional<graphics::Color>& color, const boost::optional<std::uint32_t>& measure) {
				const bool overtype = caret.isOvertypeMode() && isSelectionEmpty(caret);
				const auto renderer(caret.textArea().textRenderer());
				boost::geometry::model::box<boost::geometry::model::d2::point_xy<std::int32_t>> bounds;

				if(const graphics::font::TextLayout* const layout = renderer->layouts().at(kernel::line(caret))) {
					boost::optional<graphics::Rectangle> characterBounds(currentCharacterLogicalBounds(caret));
					assert(characterBounds);

					boost::optional<std::uint32_t> advance(measure);
					if(advance == boost::none && (!caret.isOvertypeMode() || !isSelectionEmpty(caret)))
						advance = systemDefinedCaretMeasure();
					if(advance != boost::none) {
						const presentation::WritingMode writingMode(graphics::font::writingMode(*layout));
						presentation::FlowRelativeFourSides<std::int32_t> abstractBounds;
						graphics::PhysicalFourSides<std::int32_t> physicalBounds(bounds);
						presentation::mapDimensions(writingMode, presentation::_from = physicalBounds, presentation::_to = abstractBounds);
						abstractBounds.end() = abstractBounds.start() + boost::get(advance);
						presentation::mapDimensions(writingMode, presentation::_from = abstractBounds, presentation::_to = physicalBounds);
						boost::geometry::assign(bounds, graphics::geometry::make<graphics::Rectangle>(physicalBounds));
					}
				}
				else
					boost::geometry::assign_zero(bounds);

				// create an image
				CaretShaper::Shape shape;
				shape.image = createSolidCaretImage(
					static_cast<graphics::geometry::BasicDimension<std::uint32_t>>(graphics::geometry::size(bounds)),
					boost::get_optional_value_or(color, graphics::Color::OPAQUE_BLACK));
				graphics::geometry::scale(
					graphics::geometry::_from = graphics::geometry::topLeft(bounds), graphics::geometry::_to = shape.alignmentPoint,
					graphics::geometry::_sx = -1, graphics::geometry::_sy = -1);
				return shape;
			}
		}	// namespace @0

		/// @see Caret#InputModeChangedSignal
		void StandardCaretShaper::inputModeChanged(const Caret& caret, Caret::InputModeChangedSignalType) BOOST_NOEXCEPT {
			signalStaticShapeChanged(caret);
		}

		/// @see CaretShaper#install
		void StandardCaretShaper::install(Caret& caret) BOOST_NOEXCEPT {
			assert(caretMotionConnections_.find(&caret) == std::end(caretMotionConnections_));
			caretMotionConnections_.insert(std::make_pair(&caret,
				caret.motionSignal().connect(
					std::bind(&StandardCaretShaper::caretMoved, this, std::placeholders::_1, std::placeholders::_2))));

			assert(inputModeChangedConnections_.find(&caret) == std::end(inputModeChangedConnections_));
			inputModeChangedConnections_.insert(std::make_pair(&caret,
				caret.inputModeChangedSignal().connect(
					std::bind(&StandardCaretShaper::inputModeChanged, this, std::placeholders::_1, std::placeholders::_2))));
		}

		namespace {
			/**
			 * Creates the bitmap for RTL caret.
			 * @param extent The extent (height) of the image in pixels
			 * @param color The color
			 * @return The bitmap
			 */
			inline std::unique_ptr<graphics::Image> createRtlCaretImage(std::uint16_t extent, const graphics::Color& color) {
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
			inline std::unique_ptr<graphics::Image> createTisCaretImage(std::uint16_t extent, const graphics::Color& color) {
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
		}

		/// @see CaretShaper#shape
		CaretShaper::Shape StandardCaretShaper::shape(const Caret& caret, const boost::optional<kernel::Position>& position) const BOOST_NOEXCEPT {
#if 0
#	if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			const bool inputMethodIsOpen = static_cast<Glib::ustring>(
				const_cast<TextViewer&>(caret.textArea().textViewer()).get_settings()->property_gtk_im_module()) != nullptr;
#	elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			auto imc(win32::inputMethod(const_cast<TextViewer&>(caret.textArea().textViewer())));
			const bool inputMethodIsOpen = win32::boole(::ImmGetOpenStatus(imc.get()));
#	else
			ASCENSION_CANT_DETECT_PLATFORM();
#	endif
			if(inputMethodIsOpen) {
				static const graphics::Color red(0x80, 0x00, 0x00);
				return createSolidShape(caret, red, boost::none);
			}
#endif
#if 0
			if(presentation::isHorizontal(caret.textArea().textRenderer().computedBlockFlowDirection())) {
				const WORD language = PRIMARYLANGID(LOWORD(::GetKeyboardLayout(::GetCurrentThreadId())));
				if(isRtlLanguage(language))	// RTL
					return createRtlCaretShape(caret, boost::none, boost::none);
				else if(isTisLanguage(language))	// Thai relations
					return createTisCaretShape(caret, boost::none, boost::none);
			}
#endif
			return createSolidShape(caret, boost::none, boost::none);
		}

		/// @see CaretShaper#uninstall
		void StandardCaretShaper::uninstall(Caret& caret) BOOST_NOEXCEPT {
			const auto i(caretMotionConnections_.find(&caret));
			if(i != std::end(caretMotionConnections_)) {
				std::get<1>(*i).disconnect();
				caretMotionConnections_.erase(i);
			}
			const auto j(inputModeChangedConnections_.find(&caret));
			if(j != std::end(inputModeChangedConnections_)) {
				std::get<1>(*j).disconnect();
				inputModeChangedConnections_.erase(j);
			}
		}
	}
}
