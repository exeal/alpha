/**
 * @file scrollable.hpp
 * @author exeal
 * @date 2011-09-27
 */

#ifndef ASCENSION_SCROLLABLE_HPP
#define ASCENSION_SCROLLABLE_HPP
#include <ascension/corelib/range.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gtkmm/scrollable.h>
#endif

namespace ascension {
	namespace viewers {
		namespace widgetapi {

			struct ScrollCommands {
				enum Value {
					SINGLE_STEP_INCREMENT,
					SINGLE_STEP_DECREMENT,
					PAGE_STEP_INCREMENT,
					PAGE_STEP_DECREMENT,
					MAXIMIZE,
					MINIMIZE
				};
			};

			template<typename Position>
			class ScrollProperties {
			public:
				typedef Position ScrollPosition;
			public:
				virtual void command(ScrollCommands::Value ) = 0;
				virtual boost::integer_range<ScrollPosition> range() const = 0;
				virtual ScrollPosition pageStep() const = 0;
				virtual ScrollPosition position() const = 0;
				virtual void setRange(const boost::integer_range<ScrollPosition>& newRange) = 0;
				virtual void setPageStep(ScrollPosition newPageStep) = 0;
				virtual void setPosition(ScrollPosition newPosition) = 0;
				virtual void setSingleStep(ScrollPosition newSingleStep) = 0;
				virtual ScrollPosition singleStep() const = 0;
			};

#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
			typedef Gtk::Scrollable NativeScrollableWidget;	// Gtk.Viewport is preferred?
			typedef double NativeScrollPosition;
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
			typedef QScrollArea NativeScrollableWidget;
			typedef int NativeScrollPosition;
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
			typedef NSScrollView NativeScrollableWidget;
			typedef CGFloat NativeScrollPosition;
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			typedef win32::Window NativeScrollableWidget;
			typedef int NativeScrollPosition;
#endif
			static_assert(std::is_signed<NativeScrollPosition>::value,
				"ascension.viewers.widgetapi.NativeScrollPosition should be signed.");
#if 0
			class ScrollableWidget : public ScrollProperties<int>, public Widget {
			public:
				enum ScrollBarPolicy {
					ALWAYS_HIDDEN,
					ALWAYS_VISIBLE,
					VISIBLE_AS_NEEDED
				};
			public:
				ScrollableWidget(Widget* parent = 0, Style styles = WIDGET);
				virtual ScrollProperties<ScrollPosition>& horizontalScrollBar() const = 0;
				virtual ScrollBarPolicy horizontalScrollBarPolicy() const = 0;
				virtual void setHorizontalScrollBarPolicy(ScrollBarPolicy policy) = 0;
				virtual void setVerticalScrollBarPolicy(ScrollBarPolicy policy) = 0;
				virtual ScrollProperties<ScrollPosition>& verticalScrollBar() const = 0;
				virtual ScrollBarPolicy verticalScrollBarPolicy() const = 0;
			};
#endif
		}
	}
}

#endif // !ASCENSION_SCROLLABLE_HPP
