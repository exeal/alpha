/**
 * @file content-assist-gtk.cpp
 * Implements @c DefaultContentAssistant and @c DefaultContentAssistant#CompletionProposalsPopup
 * classes on gtkmm window system.
 * @author exeal
 * @date 2013-10-23 Created.
 * @date 2014
 */

#include <ascension/platforms.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)

#include <ascension/content-assist/default-content-assistant.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>
#include <boost/foreach.hpp>


namespace ascension {
	namespace contentassist {
		// DefaultContentAssistant.CompletionProposalsPopup ///////////////////////////////////////////////////////////

		DefaultContentAssistant::CompletionProposalsPopup::ColumnRecord::ColumnRecord() {
			add(icon);
			add(displayString);
		}

		DefaultContentAssistant::CompletionProposalsPopup::CompletionProposalsPopup(
				viewers::TextViewer& parent, ContentAssistant::CompletionProposalsUI& ui) BOOST_NOEXCEPT : ui_(ui) {
			add(view_);
			set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

			view_.set_can_focus(false);
			view_.set_headers_visible(false);
			view_.set_model(model_ = Gtk::ListStore::create(columns_));
			view_.set_reorderable(false);
			if(const Glib::RefPtr<Gtk::TreeSelection> selection = view_.get_selection())
				selection->set_mode(Gtk::SELECTION_SINGLE);
		}

		void DefaultContentAssistant::CompletionProposalsPopup::end() {
			viewers::widgetapi::hide(*this);
			model_->clear();
		}

		void DefaultContentAssistant::CompletionProposalsPopup::resetContent(std::shared_ptr<const CompletionProposal> proposals[], std::size_t numberOfProposals) {
			decltype(proposals_) newProposals;
//			std::copy(proposals, proposals + numberOfProposals, std::back_inserter(newProposals));
			newProposals.reserve(numberOfProposals);

			Glib::RefPtr<Gtk::ListStore> newModel(Gtk::ListStore::create(columns_));
			for(std::size_t i = 0; i < numberOfProposals; ++i) {
				// TODO: display icons.
				const String displayString = proposals[i]->displayString();
				if(!displayString.empty()) {
					Gtk::TreeModel::Row newRow(*newModel->append());
					newRow[columns_.displayString] = toGlibUstring(displayString);
					if(proposals[i]->icon())
						newRow[columns_.icon] = proposals[i]->icon();
				}
			}
			view_.set_model(model_ = newModel);
			std::swap(proposals_, newProposals);
		}

		std::shared_ptr<const CompletionProposal> DefaultContentAssistant::CompletionProposalsPopup::selectedProposal() const {
			if(const Glib::RefPtr<Gtk::TreeSelection> selection = const_cast<Gtk::TreeView&>(view_).get_selection()) {
				if(const Gtk::TreeModel::iterator selectedItem = selection->get_selected())
					return (*selectedItem)[columns_.proposalObject];
			}
			return std::shared_ptr<const CompletionProposal>();
		}

		void DefaultContentAssistant::CompletionProposalsPopup::selectProposal(std::shared_ptr<const CompletionProposal> selection) {
			if(const Glib::RefPtr<Gtk::TreeSelection> s = view_.get_selection()) {
				s->unselect_all();
				BOOST_FOREACH(auto i, model_->children()) {
					if(static_cast<std::shared_ptr<const CompletionProposal>>((*i)[columns_.proposalObject]) == selection) {
						s->select(i);
						break;
					}
				}
			}
		}

		void DefaultContentAssistant::CompletionProposalsPopup::setWritingMode(const presentation::WritingMode& writingMode) {
			view_.set_direction((writingMode.inlineFlowDirection == presentation::LEFT_TO_RIGHT) ? Gtk::TEXT_DIR_LTR : Gtk::TEXT_DIR_RTL);
			// TODO: Change the orientation.
		}


		// DefaultContentAssistant ////////////////////////////////////////////////////////////////////////

		/// @see CompletionProposalsUI#nextPage
		void DefaultContentAssistant::nextPage(int pages) {
			GdkEventKey e;
			memset(&e, 0, sizeof(decltype(e)));
			e.type = GDK_KEY_PRESS;
			e.send_event = true;
			e.keyval = GDK_KEY_Page_Down;
			while(pages > 0) {
				proposalsPopup_->event(reinterpret_cast<GdkEvent*>(&e));
				--pages;
			}
			e.keyval = GDK_KEY_Page_Up;
			while(pages < 0) {
				proposalsPopup_->event(reinterpret_cast<GdkEvent*>(&e));
				++pages;
			}
		}

		/// @see CompletionProposalsUI#nextProposal
		void DefaultContentAssistant::nextProposal(int proposals) {
			GdkEventKey e;
			memset(&e, 0, sizeof(decltype(e)));
			e.type = GDK_KEY_PRESS;
			e.send_event = true;
			e.keyval = GDK_KEY_Down;
			while(proposals > 0) {
				proposalsPopup_->event(reinterpret_cast<GdkEvent*>(&e));
				--proposals;
			}
			e.keyval = GDK_KEY_Up;
			while(proposals < 0) {
				proposalsPopup_->event(reinterpret_cast<GdkEvent*>(&e));
				++proposals;
			}
		}
	}
}

#endif // ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
