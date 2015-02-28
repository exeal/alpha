/**
 * @file search.hpp
 * @author exeal
 * @date 2003-2007 (was search-dialog.hpp)
 * @date 2008-2009
 */

#ifndef ALPHA_SEARCH_HPP
#define ALPHA_SEARCH_HPP
#include "resource.h"
#include <manah/win32/ui/dialog.hpp>
#include <manah/win32/ui/standard-controls.hpp>
#include <manah/win32/ui/menu.hpp>
#include <ascension/searcher.hpp>

// fwd. decl.
namespace ascension {
	namespace viewer {class TextViewer;}
}

namespace alpha {
	class EditorView;

	std::size_t bookmarkMatchLines(const ascension::kernel::Region& region, bool interactive);

	namespace ui {
		/// "Search and Replace" dialog box.
		class SearchDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_SEARCH> {
		public:
			SearchDialog();
			// attributes
			std::wstring activePattern() const /*throw()*/;
			std::wstring activeReplacement() const /*throw()*/;
			// operations
			bool repeatSearch(ascension::Direction direction, bool noerror = true, long n = 1);
			bool search(const ascension::String& pattern, ascension::Direction direction, bool noerror = true, long n = 1);
			void replaceAll(bool interactive);

		private:
			INT_PTR processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam);
			void rebuildPattern();
			void updateConditions();
		private:
			void onCancel(bool& continueDialog);						// IDCANCEL
			void onClose(bool& continueDialog);							// WM_CLOSE
			bool onCommand(WORD id, WORD notifyCode, HWND control);		// WM_COMMAND
			void onInitDialog(HWND focusWindow, bool& focusDefault);	// WM_INITDIALOG
		private:
			bool initializesPatternFromEditor_;
			// widgets
			manah::win32::ui::ComboBox patternCombobox_;
			manah::win32::ui::ComboBox replacementCombobox_;
			manah::win32::ui::ComboBox searchTypeCombobox_;
			manah::win32::ui::ComboBox wholeMatchCombobox_;
			manah::win32::ui::ComboBox collationWeightCombobox_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_COMBO_FINDWHAT, patternCombobox_)
				MANAH_BIND_CONTROL(IDC_COMBO_REPLACEWITH, replacementCombobox_)
				MANAH_BIND_CONTROL(IDC_COMBO_SEARCHTYPE, searchTypeCombobox_)
				MANAH_BIND_CONTROL(IDC_COMBO_WHOLEMATCH, wholeMatchCombobox_)
				MANAH_BIND_CONTROL(IDC_COMBO_COLLATIONWEIGHT, collationWeightCombobox_)
			MANAH_END_CONTROL_BINDING()
		};

		/// Implements interactive replacement callback for Alpha.
		class InteractiveReplacementCallback : virtual public ascension::searcher::IInteractiveReplacementCallback {
		public:
			InteractiveReplacementCallback();
			~InteractiveReplacementCallback() throw();
			void setTextViewer(ascension::viewer::TextViewer& textViewer) throw();
		private:
			ascension::searcher::IInteractiveReplacementCallback::Action
				queryReplacementAction(const ascension::kernel::Region& matchedRegion, bool canUndo);
			void replacementEnded(std::size_t numberOfMatches, std::size_t numberOfReplacements);
			void replacementStarted(const ascension::kernel::Document& document, const ascension::kernel::Region& scope);
		private:
			HMENU menu_;
			ascension::viewer::TextViewer* textViewer_;
		};
	}
}

#endif // !ALPHA_SEARCH_HPP
