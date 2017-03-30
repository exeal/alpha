/**
 * @file win32/container.cpp
 * Implements @c win32#Container class.
 * @author exeal
 * @date 2017-03-27 Created.
 */

#include <ascension/graphics/geometry/algorithms/inflate.hpp>
#include <ascension/graphics/geometry/algorithms/make.hpp>
#include <ascension/graphics/geometry/rectangle-odxdy.hpp>
#include <ascension/graphics/geometry/rectangle-sides.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>
#include <boost/foreach.hpp>
#include "win32/container.hpp"

namespace alpha {
	namespace win32 {
		/**
		 * Creates a @c Container instance.
		 * @param horizontal
		 */
		Container::Container(bool horizontal) : padding_(0), horizontal_(horizontal), homogeneous_(false) {}

		/// @internal
		std::list<Container::Child>::iterator Container::find(const ascension::win32::Window& widget) {
			const auto e(std::end(children_));
			for(auto i(std::begin(children_)); i != e; ++i) {
				if(i->widget.get() == &widget)
					return i;
			}
			return e;
		}

		/// Returns @c true if the @c Container is homogeneous.
		bool Container::isHomogeneous() const BOOST_NOEXCEPT {
			return homogeneous_;
		}

		/**
		 * Returns the padding in user units.
		 * @see #setPadding
		 */
		int Container::padding() const BOOST_NOEXCEPT {
			return padding_;
		}

		/// @see ascension#win32#CustomControl#processMessage
		LRESULT Container::processMessage(UINT message, WPARAM wp, LPARAM lp, bool& consumed) {
			if(message == WM_SIZE)
				updatePlacement();
			return ascension::win32::CustomControl<Container>::processMessage(message, wp, lp, consumed);
		}

		/// @internal Implementes @c #pushBack and @c #pushFront methods.
		void Container::push(std::shared_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces, bool back) {
			if(child.get() == nullptr)
				throw ascension::NullPointerException("child");
			if(find(*child) != std::end(children_))
				throw std::invalid_argument("child");
			Child newChild;
			newChild.widget = child;
			newChild.packingOptions = options;
			newChild.spaces = spaces;
			if(back)
				children_.push_back(std::move(newChild));
			else
				children_.push_front(std::move(newChild));
		}

		/**
		 *
		 * @param child The child widget to push
		 * @param options The packing options
		 * @param spaces The additional spaces around the @a child
		 * @throw ascension#NullPointerException @a child is @c null
		 * @throw std#invalid_argument @a child is already pushed
		 */
		void Container::pushBack(std::shared_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces /* = Spaces() */) {
			return push(child, options, spaces, true);
		}

		/// @copydoc
		void Container::pushBack(std::unique_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces /* = Spaces() */) {
			return push(std::move(child), options, spaces, true);
		}

		/**
		 *
		 * @param child The child widget to push
		 * @param options The packing options
		 * @param spaces The additional spaces around the @a child
		 * @throw ascension#NullPointerException @a child is @c null
		 * @throw std#invalid_argument @a child is already pushed
		 */
		void Container::pushFront(std::shared_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces /* = Spaces() */) {
			return push(child, options, spaces, false);
		}

		/// @copydoc
		void Container::pushFront(std::unique_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces /* = Spaces() */) {
			return push(std::move(child), options, spaces, false);
		}

		/**
		 * Makes the @c Container homogeneous.
		 * @param homogeneous
		 */
		void Container::setHomogeneous(bool homogeneous /* = true */) {
			homogeneous_ = homogeneous;
			updatePlacement();
		}

		/***/
		void Container::setPadding(int newPadding) {
			padding_ = newPadding;
			updatePlacement();
		}

		///
		void Container::updatePlacement() {
			auto containerBounds(ascension::viewer::widgetapi::bounds(*this, false));
			ascension::graphics::geometry::inflate(containerBounds, -static_cast<ascension::graphics::Scalar>(padding()));

			ascension::graphics::Scalar fixedSize = 0;
			std::size_t numberOfVariableChildren = 0;
			BOOST_FOREACH(const Child& child, children_) {
				if(!ascension::viewer::widgetapi::isRealized(*child.widget))
					continue;
				fixedSize += child.spaces.before;
				const auto packingOption = std::get<0>(child.packingOptions);
				if(packingOption == SHRINK || packingOption == EXPAND_SPACE) {
					const auto size(ascension::viewer::widgetapi::bounds(*child.widget, true));
					fixedSize += horizontal_ ? ascension::graphics::geometry::dx(size) : ascension::graphics::geometry::dy(size);
				}
				if(packingOption == EXPAND_SPACE || packingOption == EXPAND_WIDGET)
					++numberOfVariableChildren;
				fixedSize += child.spaces.after;
			}

			const auto containerSize = horizontal_ ? ascension::graphics::geometry::dx(containerBounds) : ascension::graphics::geometry::dy(containerBounds);
			int space = std::max(static_cast<int>(containerSize) - static_cast<int>(fixedSize), 0);
			if(numberOfVariableChildren > 0)
				space /= numberOfVariableChildren;
			int position = static_cast<int>(horizontal_ ? ascension::graphics::geometry::left(containerBounds) : ascension::graphics::geometry::top(containerBounds));
			BOOST_FOREACH(const Child& child, children_) {
				auto childBounds(ascension::viewer::widgetapi::bounds(*child.widget, true));
				auto start = static_cast<ascension::graphics::Scalar>(position + child.spaces.before);
				if(std::get<0>(child.packingOptions) == EXPAND_SPACE)
					start += space / 2;
				auto end = (std::get<0>(child.packingOptions) == EXPAND_WIDGET) ? start + space
					: (horizontal_ ? ascension::graphics::geometry::dx(childBounds) : ascension::graphics::geometry::dy(childBounds));
				if(horizontal_)
					childBounds = ascension::graphics::geometry::make<ascension::graphics::Rectangle>((
						ascension::graphics::geometry::_left = start,
						ascension::graphics::geometry::_right = end,
						ascension::graphics::geometry::_top = ascension::graphics::geometry::top(containerBounds),
						ascension::graphics::geometry::_bottom = ascension::graphics::geometry::bottom(containerBounds)));
				else
					childBounds = ascension::graphics::geometry::make<ascension::graphics::Rectangle>((
						ascension::graphics::geometry::_left = ascension::graphics::geometry::left(containerBounds),
						ascension::graphics::geometry::_right = ascension::graphics::geometry::right(containerBounds),
						ascension::graphics::geometry::_top = start,
						ascension::graphics::geometry::_bottom = end));
				ascension::viewer::widgetapi::setBounds(*child.widget, childBounds);
				if(std::get<0>(child.packingOptions) == EXPAND_SPACE)
					end += space / 2;
				position = static_cast<int>(end) + child.spaces.after;
			}
		}

		/// @see ascension#win32#CustomControl#windowClass
		void Container::windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT {
			out.name = L"alpha.Container";
			out.styles = CS_HREDRAW | CS_VREDRAW;
		}

		/// Creates a @c HorizontalContainer instance.
		HorizontalContainer::HorizontalContainer() : Container(true) {}

		/// Creates a @c VerticalContainer instance.
		VerticalContainer::VerticalContainer() : Container(false) {}
	}
}
