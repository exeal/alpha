/**
 * @file widget-themed-text-renderer.cpp
 * Implements @c WidgetThemedTextRenderer class.
 * @author exeal
 * @date 2016-07-16 Created.
 */

#include <ascension/presentation/styled-text-run-iterator.hpp>
#include <ascension/presentation/text-line-style.hpp>
#include <ascension/presentation/text-run-style.hpp>
#include <ascension/presentation/text-toplevel-style.hpp>
#include <ascension/viewer/text-viewer.hpp>
#include <ascension/viewer/widget-themed-text-renderer.hpp>
#include <ascension/graphics/native-conversion.hpp>
#if (ASCENSION_SELECTS_SHAPING_ENGINE(UNISCRIBE) || ASCENSION_SELECTS_SHAPING_ENGINE(WIN32_GDI)) && BOOST_OS_WINDOWS
#	include <ascension/graphics/rendering-context.hpp>
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

		/// @see TextRenderer#actualLineBackgroundColor
		graphics::Color WidgetThemedTextRenderer::actualLineBackgroundColor(const graphics::font::TextLayout&) const BOOST_NOEXCEPT {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			if(const auto context = textViewer_.get_style_context())
				return graphics::fromNative<graphics::Color>(context->get_background_color());	// deprecated api
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			win32::Handle<HBRUSH> brush(reinterpret_cast<HBRUSH>(::GetClassLongPtrW(textViewer_.handle(), GCW_HBRBACKGROUND)));
			if(brush.get() != nullptr) {
				LOGBRUSH lb;
				if(::GetObject(brush.get(), sizeof(decltype(lb)), &lb) != 0 && lb.lbStyle == BS_SOLID)
					return graphics::fromNative<graphics::Color>(lb.lbColor);
			}
			return ::GetSystemColor(COLOR_WINDOW);
#endif
			return graphics::Color::OPAQUE_WHITE;
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
			static const presentation::ComputedTextToplevelStyle toplevel((presentation::SpecifiedTextToplevelStyle()));
			static const presentation::ComputedTextLineStyle lines((presentation::SpecifiedTextLineStyle()));
			static const presentation::ComputedTextRunStyle runs;
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
				const auto style = ::GetWindowLongPtrW(widget->handle(), GWL_EXSTYLE);
				if(style != 0) {
					static_assert(WS_EX_LTRREADING == 0, "");
					static_assert(WS_EX_LAYOUTLTR == 0, "");
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
			const auto = ::GetWindowLongPtrW(textViewer_.handle(), GWL_EXSTYLE);
			return (style & WS_EX_RIGHT) != 0;
#endif
		}

		/// @see TextRenderer#textOrientation
		presentation::TextOrientation WidgetThemedTextRenderer::textOrientation() const BOOST_NOEXCEPT {
			return presentation::MIXED;
		}
	}
}
