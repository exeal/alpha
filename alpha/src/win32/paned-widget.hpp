/**
 * @file win32/paned-widget.hpp
 * Defines @c win32#PanedWidget class.
 * @author exeal
 * @date 2017-01-30 Created.
 */

#ifndef ASCENSION_WIN32_PANED_WIDGET_HPP
#define ASCENSION_WIN32_PANED_WIDGET_HPP
#include <ascension/win32/window/custom-control.hpp>
#include <boost/any.hpp>

namespace alpha {
	namespace win32 {
		class PanedWidget : public ascension::win32::CustomControl<PanedWidget> {
		public:
			PanedWidget();
			virtual ~PanedWidget() BOOST_NOEXCEPT;

			/// @name Children
			/// @{
			template<std::size_t position> boost::any& child() BOOST_NOEXCEPT;
			template<std::size_t position> const boost::any& child() const BOOST_NOEXCEPT;
			template<std::size_t position> void resetChild();
			template<std::size_t position, typename Child> void resetChild(std::shared_ptr<Child> newChild);
			/// @}

			/// @name Orientation
			/// @{
			BOOST_CONSTEXPR bool isHorizontal() const BOOST_NOEXCEPT;
			BOOST_CONSTEXPR bool isVertical() const BOOST_NOEXCEPT;
			void setHorizontal() BOOST_NOEXCEPT;
			void setVertical() BOOST_NOEXCEPT;
			/// @}

			/// @name Gap Width
			/// @{
			BOOST_CONSTEXPR unsigned int gap() const BOOST_NOEXCEPT;
			void setGap(unsigned int newGap) BOOST_NOEXCEPT;
			/// @}

			static void windowClass(ascension::win32::WindowClass& out) BOOST_NOEXCEPT;

		protected:
			// ascension.win32.CustomControl
			virtual LRESULT processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) override;

		private:
			template<std::size_t> BOOST_CONSTEXPR bool hasChild() const BOOST_NOEXCEPT;
			std::size_t numberOfChildren() const BOOST_NOEXCEPT;
			void onLButtonDblClk(UINT modifiers, const POINT& location, bool& consumed);
			void onSetCursor(HWND window, UINT hit, UINT message, bool& consumed);
			void onSize(UINT type, int width, int height, bool& consumed);
			void updateChildrenPlacement();
		private:
			std::pair<ascension::win32::Window*, ascension::win32::Window*> children_;
			std::pair<boost::any, boost::any> typedChildren_;
			bool horizontal_;
			float firstChildSizeRatio_;	// (first child size) / (paned widget size) in height or width
			unsigned int gap_;
		};

		/**
		 * Returns the child widget.
		 * @tparam position The position. Either zero or one
		 * @return The child widget as @c boost#any
		 * @note The widget which was added by @c #setChild method is stored as a @c std#shared_ptr&lt;T&gt;. @c T is
		 *       the type specified by the template parameter of @c #setChild call
		 */
		template<std::size_t position>
		inline boost::any& PanedWidget::child() BOOST_NOEXCEPT {
			return std::get<position>(typedChildren_);
		}

		/// @overload
		template<std::size_t position>
		inline const boost::any& PanedWidget::child() const BOOST_NOEXCEPT {
			return std::get<position>(typedChildren_);
		}

		/**
		 * Returns the gap setting in user units.
		 * @see #setGap
		 */
		inline BOOST_CONSTEXPR unsigned int PanedWidget::gap() const BOOST_NOEXCEPT {
			return gap_;
		}

		/**
		 * @internal Returns @c true if the @c PanedWidget has a child at the specified position
		 * @tparam position The position to query
		 * @return The result
		 */
		template<std::size_t position>
		inline BOOST_CONSTEXPR bool PanedWidget::hasChild() const BOOST_NOEXCEPT {
			return std::get<position>(children_) != nullptr;
		}

		/**
		 * Returns the widget has horizontal orientation.
		 * @see #isVertical, #setHorizontal
		 */
		inline BOOST_CONSTEXPR bool PanedWidget::isHorizontal() const BOOST_NOEXCEPT {
			return horizontal_;
		}

		/**
		 * Returns the widget has vertical orientation.
		 * @see #isHorizontal, #setVertical
		 */
		inline BOOST_CONSTEXPR bool PanedWidget::isVertical() const BOOST_NOEXCEPT {
			return !isHorizontal();
		}

		/// Returns the number of child widgets in the @c PanedWidget.
		inline std::size_t PanedWidget::numberOfChildren() const BOOST_NOEXCEPT {
			std::size_t n = 0;
			if(hasChild<0>())
				++n;
			if(hasChild<1>())
				++n;
			return n;
		}

		/**
		 * Sets/adds the specified child widget.
		 * @tparam position The position. Either zero or one
		 * @param newChild The child widget to set/add
		 */
		template<std::size_t position, typename Child>
		inline void PanedWidget::resetChild(std::shared_ptr<Child> newChild) {
			if(newChild.get() == nullptr)
				return resetChild<position>();
			if(hasChild<position>())
				::SetParent(std::get<position>(children_)->handle().get(), nullptr);
			const auto n = numberOfChildren();
			std::get<position>(typedChildren_) = newChild;
			std::get<position>(children_) = newChild.get();
			::SetParent(std::get<position>(children_)->handle().get(), handle().get());
			if(numberOfChildren() != n)
				updateChildrenPlacement();
		}

		/**
		* Removes the specified child widget from this @c PanedWidget.
		* @tparam position The position. Either zero or one
		*/
		template<std::size_t position>
		inline void PanedWidget::resetChild() {
			if(hasChild<position>()) {
				::SetParent(std::get<position>(children_)->handle().get(), nullptr);
				std::get<position>(children_) = nullptr;
				std::get<position>(typedChildren_).clear();
				updateChildrenPlacement();
			}
		}
	}
}

#endif // !ASCENSION_WIN32_PANED_WIDGET_HPP
