/**
 * @file buffer-bar.cpp
 * Implements @c BufferBar class.
 */

#include "application.hpp"
#include "buffer.hpp"
#include "buffer-bar.hpp"
#include "buffer-list.hpp"
#include "editor-window.hpp"
#include <gtkmm/toggletoolbutton.h>

namespace alpha {
	namespace ui {
		/**
		 * @param bufferList
		 */
		BufferBar::BufferBar(BufferList& bufferList) : bufferList_(bufferList) {
			set_show_arrow(true);
			set_toolbar_style(Gtk::TOOLBAR_BOTH_HORIZ);

			bufferList_.bufferAboutToBeRemoved().connect(std::bind(&BufferBar::bufferAboutToBeRemoved, this, std::placeholders::_1, std::placeholders::_2));
			bufferList_.bufferAdded().connect(std::bind(&BufferBar::bufferAdded, this, std::placeholders::_1, std::placeholders::_2));
			bufferList_.displayNameChanged().connect(std::bind(&BufferBar::bufferDisplayNameChanged, this, std::placeholders::_1));
		}

		void BufferBar::bufferAboutToBeRemoved(BufferList&, Buffer& buffer) {
			if(const boost::optional<std::size_t> position = bufferList_.find(buffer)) {
				if(Gtk::ToolItem* const button = get_nth_item(boost::get(position)))
					button->remove();
			}
		}

		void BufferBar::bufferAdded(BufferList&, Buffer& buffer) {
			Gtk::ToggleToolButton newButton(buffer.name());
			newButton.set_expand(true);
			newButton.set_homogeneous(false);
//			if(buffer.textFile().isBoundToFile())
//				newButton.set_tooltip_text(buffer.textFile().fileName());
			newButton.set_use_underline(false);
			newButton.signal_clicked().connect(sigc::bind<const Buffer&>(&BufferBar::buttonClicked, buffer));
		}

		void BufferBar::bufferDisplayNameChanged(const Buffer& buffer) {
			if(const boost::optional<std::size_t> position = bufferList_.find(buffer)) {
				if(Gtk::ToolItem* const button = get_nth_item(boost::get(position))) {
					button->set_name(buffer.name());
//					button->set_tooltip_text(buffer.textFile().isBoundToFile() ? buffer.textFile().fileName() : buffer.name());
				}
			}
		}

		void BufferBar::bufferSelectionChanged(EditorPanes& panes) {
			// check the button associated with the selected buffer
			if(const boost::optional<std::size_t> activePosition = bufferList_.find(panes.selectedBuffer())) {
				for(int i = 0, c = get_n_items(); i < c; ++i) {
					if(Gtk::ToolItem* const button = get_nth_item(i))
						button->set_state((i == boost::get(activePosition)) ? Gtk::STATE_ACTIVE : Gtk::STATE_NORMAL);
				}
			}
		}
	}
}
