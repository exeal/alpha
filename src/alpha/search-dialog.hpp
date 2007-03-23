/**
 * @file search-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_SEARCH_DIALOG_HPP
#define ALPHA_SEARCH_DIALOG_HPP
#include "resource.h"
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
#include "../manah/win32/ui/menu.hpp"
#include "ascension/searcher.hpp"

namespace alpha {
	class EditorView;

	namespace ui {
		/// "Search and Replace" dialog box.
		class SearchDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_SEARCH> {
		public:
			std::wstring	getActivePattern() const throw();
			std::wstring	getActiveReplacement() const throw();
			void			setOptions();

		private:
			INT_PTR	processWindowMessage(UINT message, WPARAM wParam, LPARAM lParam);
			void	updateOptions();
		private:
			void	onCancel(bool& continueDialog);									// IDCANCEL
			void	onClose(bool& continueDialog);									// WM_CLOSE
			bool	onCommand(WORD id, WORD notifyCode, HWND control);				// WM_COMMAND
			void	onInitDialog(HWND focusWindow, bool& focusDefault);				// WM_INITDIALOG
		private:
			// ÉRÉìÉgÉçÅ[Éã
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

	}
}

#endif /* !ALPHA_SEARCH_DIALOG_HPP */
