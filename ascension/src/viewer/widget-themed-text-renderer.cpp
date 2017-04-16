/**
 * @file widget-themed-text-renderer.cpp
 * Implements @c WidgetThemedTextRenderer class.
 * @author exeal
 * @date 2016-07-16 Created.
 */

#include <ascension/corelib/native-conversion.hpp>
#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/widget-themed-text-renderer.hpp>
#if (ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)) && BOOST_OS_WINDOWS
#	include <ascension/graphics/rendering-context.hpp>
#endif
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/system-default-font.hpp>
#endif

namespace ascension {
	namespace viewer {
		/**
		 * @param textViewer The text viewer
		 * @param initialSize
		 */
		WidgetThemedTextRenderer::WidgetThemedTextRenderer(TextViewer& textViewer, const graphics::Dimension& initialSize)
				: graphics::font::StandardTextRenderer(*document(textViewer), initialSize), textViewer_(textViewer) {
		}

		namespace {
			graphics::Color backgroundColor(const TextViewer& textViewer) BOOST_NOEXCEPT {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				if(const auto context = textViewer.get_style_context())
					return fromNative<graphics::Color>(context->get_background_color());	// deprecated api
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				auto brush(win32::borrowed(reinterpret_cast<HBRUSH>(::GetClassLongPtrW(textViewer.handle().get(), GCLP_HBRBACKGROUND))));
				if(brush.get() != nullptr) {
					LOGBRUSH lb;
					if(::GetObject(brush.get(), sizeof(decltype(lb)), &lb) != 0 && lb.lbStyle == BS_SOLID)
						return fromNative<graphics::Color>(lb.lbColor);
				}
				return fromNative<graphics::Color>(::GetSysColor(COLOR_WINDOW));
#endif
				return graphics::Color::OPAQUE_WHITE;
			}
		}

		/// @see TextRenderer#actualLineBackgroundColor
		graphics::Color WidgetThemedTextRenderer::actualLineBackgroundColor(const graphics::font::TextLayout&) const BOOST_NOEXCEPT {
			return backgroundColor(textViewer_);
		}

		/// @see TextRenderer#blockFlowDirection
		presentation::BlockFlowDirection WidgetThemedTextRenderer::blockFlowDirection() const BOOST_NOEXCEPT {
			return presentation::HORIZONTAL_TB;
		}

		/// @see StandardTextRenderer#buildStylesForLineLayout
		std::tuple<
			const presentation::ComputedTextToplevelStyle&,
			const presentation::ComputedTextLineStyle&,
			std::unique_ptr<presentation::ComputedStyledTextRunIterator>,
			const presentation::ComputedTextRunStyle&
		> WidgetThemedTextRenderer::buildStylesForLineLayout(Index line, const graphics::RenderingContext2D& renderingContext) const {
			static presentation::ComputedTextToplevelStyle toplevel((presentation::SpecifiedTextToplevelStyle()));
			static presentation::ComputedTextLineStyle lines((presentation::SpecifiedTextLineStyle()));
			static presentation::ComputedTextRunStyle runs;

			boost::fusion::at_key<presentation::styles::WritingMode>(toplevel) = blockFlowDirection();
			boost::fusion::at_key<presentation::styles::Direction>(lines) = inlineFlowDirection();
			boost::fusion::at_key<presentation::styles::TextOrientation>(lines) = textOrientation();
			boost::fusion::at_key<presentation::styles::TextAlignment>(lines) = static_cast<BOOST_SCOPED_ENUM_NATIVE(graphics::font::TextAlignment)>(boost::underlying_cast<int>(textAnchor()));
			{
				boost::optional<graphics::Color> color;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				if(const auto context = textViewer_.get_style_context())
					color = fromNative<graphics::Color>(context->get_color());
#endif
				if(color == boost::none)
					color = graphics::SystemColors::get(graphics::SystemColors::WINDOW_TEXT);
				boost::fusion::at_key<presentation::styles::Color>(runs.colors) = boost::get_optional_value_or(color, graphics::Color::OPAQUE_BLACK);
			}
			boost::fusion::at_key<presentation::styles::BackgroundColor>(runs.backgroundsAndBorders) = backgroundColor(textViewer_);
			{
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				if(const auto context = textViewer.get_style_context())
					font = fromNative<graphics::Color>(context->get_font());
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				LOGFONTW lf;
				auto fontHandle = reinterpret_cast<HFONT>(::SendMessageW(textViewer_.handle().get(), WM_GETFONT, 0, 0));
				if(fontHandle != nullptr) {
					if(::GetObjectW(fontHandle, sizeof(LOGFONTW), &lf) == 0)
						fontHandle = nullptr;
				}
				if(fontHandle == nullptr)
					win32::systemDefaultFont(lf);
				const auto font(fromNative<graphics::font::FontDescription>(lf));
#endif
				boost::fusion::at_key<presentation::styles::FontFamily>(runs.fonts).clear();
				boost::fusion::at_key<presentation::styles::FontFamily>(runs.fonts).push_back(font.family().name());
				boost::fusion::at_key<presentation::styles::FontWeight>(runs.fonts) = font.properties().weight;
				boost::fusion::at_key<presentation::styles::FontStretch>(runs.fonts) = font.properties().stretch;
				boost::fusion::at_key<presentation::styles::FontStyle>(runs.fonts) = font.properties().style;
				boost::fusion::at_key<presentation::styles::FontSize>(runs.fonts) = presentation::styles::Length(static_cast<presentation::styles::Number>(font.pointSize()), presentation::styles::Length::POINTS);
			}
			boost::fusion::at_key<presentation::styles::Direction>(runs.writingModes) = inlineFlowDirection();
#if 0
			return std::make_tuple(toplevel, lines, std::unique_ptr<presentation::ComputedStyledTextRunIterator>(), runs);
#else
			return std::tuple<
				const presentation::ComputedTextToplevelStyle&,
				const presentation::ComputedTextLineStyle&,
				std::unique_ptr<presentation::ComputedStyledTextRunIterator>,
				const presentation::ComputedTextRunStyle&
			>(toplevel, lines, std::unique_ptr<presentation::ComputedStyledTextRunIterator>(), runs);
#endif
		}

		/// @see TextRenderer#inlineFlowDirection
		presentation::ReadingDirection WidgetThemedTextRenderer::inlineFlowDirection() const BOOST_NOEXCEPT {
			widgetapi::Proxy<const widgetapi::Widget>::pointer widget = &textViewer_;
			while(widget != nullptr) {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				switch(widget->get_direction()) {
					case Gtk::TEXT_DIR_LTR:
						return presentation::LEFT_TO_RIGHT;
					case Gtk::TEXT_DIR_RTL:
						return presentation::RIGHT_TO_LEFT;
				}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				switch(widget->layoutDirection()) {
					case Qt::LeftToRight:
						return presentation::LEFT_TO_RIGHT;
					case Qt::RightToLeft:
						return presentation::RIGHT_TO_LEFT;
				}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				const auto style = ::GetWindowLongPtrW(widget->handle().get(), GWL_EXSTYLE);
				if(style != 0) {
					static_assert(WS_EX_LTRREADING == 0, "");
//					static_assert(WS_EX_LAYOUTLTR == 0, "");
					presentation::ReadingDirection direction = presentation::LEFT_TO_RIGHT;
					if((style & WS_EX_RTLREADING) != 0)
						direction = !direction;
					if((style & WS_EX_LAYOUTRTL) != 0)
						direction = !direction;
					return direction;
				}
#endif
				widget = widgetapi::parentWidget(*widget);
			}

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			switch(Gtk::Widget::get_default_direction()) {
				case Gtk::TEXT_DIR_LTR:
					return presentation::LEFT_TO_RIGHT;
				case Gtk::TEXT_DIR_RTL:
					return presentation::RIGHT_TO_LEFT;
			}
#endif
			return presentation::LEFT_TO_RIGHT;
		}

		/// @see TextRenderer#newDefaultFont
		std::shared_ptr<const graphics::font::Font> WidgetThemedTextRenderer::newDefaultFont() const BOOST_NOEXCEPT {
#if ASCENSION_SELECTS_SHAPING_ENGINE(CAIRO)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_GRAPHICS)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(CORE_TEXT)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(DIRECT_WRITE)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(HARFBUZZ)
#elif ASCENSION_SELECTS_SHAPING_ENGINE(PANGO)
#	if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			if(auto context = textViewer_.get_pango_context())
				return std::make_shared<graphics::font::Font>(context->load_font(context->get_font_description()));
#	else
			ASCENSION_CANT_DETECT_PLATFORM()
#	endif
#elif ASCENSION_SELECTS_SHAPING_ENGINE(QT)
			return std::shared_ptr<const graphics::font::Font>(std::shared_ptr<const QFont>(&textViewer_.font(), boost::null_deleter()));
#elif ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)
#	if BOOST_OS_WINDOWS
			if(const auto context = widgetapi::createRenderingContext(textViewer_))
				return context->font();
#	else
			ASCENSION_CANT_DETECT_PLATFORM()
#	endif
#elif ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDIPLUS)
#endif
			return std::shared_ptr<const graphics::font::Font>();
		}

		/// @see TextRenderer#textAnchor
		graphics::font::TextAnchor WidgetThemedTextRenderer::textAnchor() const BOOST_NOEXCEPT {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			assert(blockFlowDirection() == presentation::HORIZONTAL_TB);
			switch(textViewer_.get_halign()) {
				case Gtk::ALIGN_FILL:
				case Gtk::ALIGN_START:
				default:
					return graphics::font::TextAnchor::START;
				case Gtk::ALIGN_END:
					return graphics::font::TextAnchor::END;
				case Gtk::ALIGN_CENTER:
					return graphics::font::TextAnchor::MIDDLE;
			}
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			assert(blockFlowDirection() == presentation::HORIZONTAL_TB);
			const auto style = ::GetWindowLongPtrW(textViewer_.handle().get(), GWL_EXSTYLE);
			bool normal = true;
			if((style & WS_EX_RIGHT) != 0)
				normal = !normal;
			if((style & WS_EX_RTLREADING) != 0)
				normal = !normal;
			if((style & WS_EX_LAYOUTRTL) != 0)
				normal = !normal;
			return normal ? graphics::font::TextAnchor::START : graphics::font::TextAnchor::END;
#endif
		}

		/// @see TextRenderer#textOrientation
		presentation::TextOrientation WidgetThemedTextRenderer::textOrientation() const BOOST_NOEXCEPT {
			return presentation::MIXED;
		}
	}
}
