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

	class Alpha;
	class EditorView;

	namespace ui {
		/// [検索と置換] ダイアログ
		class SearchDialog : public manah::win32::ui::Layered<manah::win32::ui::FixedIDDialog<IDD_DLG_SEARCH> > {
		public:
			explicit SearchDialog(Alpha& app);
			std::wstring	getActivePattern() const throw();
			std::wstring	getActiveReplacement() const throw();
			void			setOptions();

		protected:
			void	onActivate(UINT state, HWND previousWindow, bool minimized);	// WM_ACTIVATE
			void	onCancel();														// IDCANCEL
			void	onClose();														// WM_CLOSE
			bool	onCommand(WORD id, WORD notifyCode, HWND control);				// WM_COMMAND
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);				// WM_INITDIALOG
		private:
			void	updateOptions();
		private:
			Alpha& app_;
			// コントロール
			manah::win32::ui::ComboBox patternCombobox_;
			manah::win32::ui::ComboBox replacementCombobox_;
			manah::win32::ui::ComboBox searchTypeCombobox_;
			manah::win32::ui::ComboBox wholeMatchCombobox_;
			manah::win32::ui::ComboBox collationWeightCombobox_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_COMBO_FINDWHAT, patternCombobox_)
				BIND_CONTROL(IDC_COMBO_REPLACEWITH, replacementCombobox_)
				BIND_CONTROL(IDC_COMBO_SEARCHTYPE, searchTypeCombobox_)
				BIND_CONTROL(IDC_COMBO_WHOLEMATCH, wholeMatchCombobox_)
				BIND_CONTROL(IDC_COMBO_COLLATIONWEIGHT, collationWeightCombobox_)
			END_CONTROL_BINDING()
		};

	}
}

#endif /* !ALPHA_SEARCH_DIALOG_HPP */
