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
#include <list>


namespace alpha {
	class Alpha;
	class EditorView;

	namespace ui {
		/// [検索と置換] ダイアログ
		class SearchDlg : public manah::windows::ui::Layered<manah::windows::ui::FixedIDDialog<IDD_DLG_SEARCH> > {
		public:
			// コンストラクタ
			explicit SearchDlg(Alpha& app);
			// 属性
			ascension::String	getFindText() const;
			void				getHistory(
									std::list<ascension::String>& findWhats,
									std::list<ascension::String>& replaceWiths) const;
			ascension::String	getReplaceText() const;
			void				setHistory(
									const std::list<ascension::String>& findWhats,
									const std::list<ascension::String>& replaceWiths);
			// 操作
			void	addToHistory(const ascension::String& text, bool replace);
			void	clearHistory(bool replace);
			void	updateOptions();
		private:
			void	updateHistory(bool replace);
			// メッセージハンドラ
		protected:
			void	onActivate(UINT state, HWND previousWindow, bool minimized);	// WM_ACTIVATE
			void	onCancel();														// IDCANCEL
			void	onClose();														// WM_CLOSE
			bool	onCommand(WORD id, WORD notifyCode, HWND control);				// WM_COMMAND
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);				// WM_INITDIALOG

			//データメンバ
		private:
			Alpha& app_;
			std::list<ascension::String> findWhats_;
			std::list<ascension::String> replaceWiths_;
			bool updateHistoryOnNextActivation_[2];
			manah::windows::ui::Menu optionMenu_;
			// コントロール
			manah::windows::ui::ComboBox findWhatCombobox_;
			manah::windows::ui::ComboBox replaceWithCombobox_;
			manah::windows::ui::ComboBox searchTypeCombobox_;
			manah::windows::ui::ComboBox caseSensitivityCombobox_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_COMBO_FINDWHAT, findWhatCombobox_)
				BIND_CONTROL(IDC_COMBO_REPLACEWITH, replaceWithCombobox_)
				BIND_CONTROL(IDC_COMBO_SEARCHTYPE, searchTypeCombobox_)
				BIND_CONTROL(IDC_COMBO_CASEFOLDING, caseSensitivityCombobox_)
			END_CONTROL_BINDING()
		};

	}
}

#endif /* !ALPHA_SEARCH_DIALOG_HPP */
