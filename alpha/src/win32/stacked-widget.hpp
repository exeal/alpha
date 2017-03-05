/**
 * @file win32/stacked-widget.hpp
 * Defines win32#StackedWidget class.
 * @author exeal
 * @date 2017-01-26 Created.
 */

#ifndef ASCENSION_STACKED_WIDGET_HPP
#define ASCENSION_STACKED_WIDGET_HPP
#include <ascension/viewer/widgetapi/widget-proxy.hpp>
#include <ascension/win32/window/custom-control.hpp>

namespace alpha {
	namespace win32 {
		class StackedWidget : public ascension::win32::CustomControl<StackedWidget> {
		public:
			StackedWidget();
			virtual ~StackedWidget() BOOST_NOEXCEPT;

			/// @name Child Widgets
			/// @{
			void addWidget(ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> widget);
			std::size_t numberOfWidgets() const;
			void removeWidget(ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> widget);
			/// @}

			/// @name Current Widget
			/// @{
			ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> currentWidget() const;
			void setCurrentWidget(ascension::viewer::widgetapi::Proxy<ascension::viewer::widgetapi::Widget> widget);
			/// @}

			/// @name Homogeneousness
			/// @{
			BOOST_CONSTEXPR bool isHorizontallyHomogeneous() const BOOST_NOEXCEPT;
			BOOST_CONSTEXPR bool isVerticallyHomogeneous() const BOOST_NOEXCEPT;
			void setHorizontallyHomogeneous(bool set = true) BOOST_NOEXCEPT;
			void setVerticallyHomogeneous(bool set = true) BOOST_NOEXCEPT;
			/// @}

			static void windowClass(ascension::win32::WindowClass& out) BOOST_NOEXCEPT;

		protected:
			// ascension.win32.CustomControl
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) override;

		private:
			template<typename Function> void foreachChildren(Function function) const;
			static BOOL CALLBACK processChildWindow(HWND window, LPARAM lp);
		private:
			bool horizontallyHomogeneous_, verticallyHomogeneous_;
		};

		template<typename Function>
		inline void StackedWidget::foreachChildren(Function function) const {
			std::function<bool(ascension::win32::Handle<HWND>)> f = function;
			::EnumChildWindows(handle().get(), &processChildWindow, reinterpret_cast<LPARAM>(&f));
		}

		/**
		 * Returns @c true if the @c StackWidget is horizontally homogeneous.
		 * @see #isVerticallyHomogeneous, #setHorizontallyHomogeneous
		 */
		inline BOOST_CONSTEXPR bool StackedWidget::isHorizontallyHomogeneous() const BOOST_NOEXCEPT {
			return horizontallyHomogeneous_;
		}

		/**
		 * Returns @c true if the @c StackWidget is vertically homogeneous.
		 * @see #isHorizontallyHomogeneous, #setVerticallyHomogeneous
		 */
		inline BOOST_CONSTEXPR bool StackedWidget::isVerticallyHomogeneous() const BOOST_NOEXCEPT {
			return verticallyHomogeneous_;
		}

		/**
		 * Sets the @c StackedWidget to be horizontally homogeneous or not.
		 * @param set The new setting
		 * @see #isHorizontallyHomogeneous, #setVerticallyHomogeneous
		 */
		inline void StackedWidget::setHorizontallyHomogeneous(bool set /* = true */) BOOST_NOEXCEPT {
			horizontallyHomogeneous_ = set;
		}

		/**
		 * Sets the @c StackedWidget to be vertically homogeneous or not.
		 * @param set The new setting
		 * @see isVerticallyHomogeneous#, #setHorizontallyHomogeneous
		 */
		inline void StackedWidget::setVerticallyHomogeneous(bool set /* = true */) BOOST_NOEXCEPT {
			verticallyHomogeneous_ = set;
		}
	}
}

#endif // !ASCENSION_STACKED_WIDGET_HPP
