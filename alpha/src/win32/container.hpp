/**
 * @file win32/container.hpp
 * Defines @c win32#Container class.
 * @author exeal
 * @date 2017-03-27 Created.
 */

#ifndef ALPHA_WIN32_CONTAINER_HPP
#define ALPHA_WIN32_CONTAINER_HPP
#include <ascension/win32/window/custom-control.hpp>
#include <list>

namespace alpha {
	namespace win32 {
		class Container : public ascension::win32::CustomControl<Container> {
		public:
			/// Packing options in main flow direction.
			enum MainFlowPackingOption {
				SHRINK,			///< Nothing.
				EXPAND_SPACE,	///< Expands the spaces around the widget according to the container.
				EXPAND_WIDGET	///< Expands the widget according to the container.
			};
			/// Packing options in sub flow direction.
			enum SubFlowPackingOption {
				FILL,				///< Maximizes the widget to fill the space.
				ALIGN_TOP_LEFT,		///< Aligns the widget top or left in the container without resize.
				ALIGN_BOTTOM_RIGHT	///< Aligns the widget bottom or right in the container without resize.
			};
			/// Packing options.
			typedef std::pair<MainFlowPackingOption, SubFlowPackingOption> PackingOptions;
			/// Spaces.
			struct Spaces {
				unsigned int before;	///< The space before the widget in user units.
				unsigned int after;		///< The space after the widget in user units.
				Spaces() : before(0u), after(0u) {}
				Spaces(unsigned int before, unsigned int after) : before(before), after(after) {}
			};

			virtual ~Container() BOOST_NOEXCEPT;

			/// @name Packing
			/// @{
			void pushBack(std::shared_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces = Spaces());
			void pushBack(std::unique_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces = Spaces());
			void pushFront(std::shared_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces = Spaces());
			void pushFront(std::unique_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces = Spaces());
			void remove(ascension::win32::Window& child);
			void updatePlacement();
			/// @}

			/// @name Padding
			/// @{
			int padding() const BOOST_NOEXCEPT;
			void setPadding(int newPadding);
			/// @}

			/// @name Homogeneous
			/// @{
			bool isHomogeneous() const BOOST_NOEXCEPT;
			void setHomogeneous(bool homogeneous = true);
			/// @}

		protected:
			explicit Container(bool horizontal);
			// ascension.win32.CustomControl
			virtual LRESULT processMessage(ascension::win32::WindowMessageEvent& event) override;
			virtual void windowClass(ascension::win32::WindowClass& out) const BOOST_NOEXCEPT override;

		private:
			struct Child {
				std::shared_ptr<ascension::win32::Window> widget;
				PackingOptions packingOptions;
				Spaces spaces;
			};
			std::list<Child>::iterator find(const ascension::win32::Window& widget);
			void push(std::shared_ptr<ascension::win32::Window> child, const PackingOptions& options, const Spaces& spaces, bool back);
			std::list<Child> children_;
			int padding_;
			const bool horizontal_;
			bool homogeneous_;
		};

		class HorizontalContainer : public Container {
		public:
			HorizontalContainer();
		};

		class VerticalContainer : public Container {
		public:
			VerticalContainer();
		};
	}
}

#endif // !ALPHA_WIN32_CONTAINER_HPP
