/**
 * @file execute-command-dialog.hpp
 * @author exeal
 * @date 2003-2005
 */

#ifndef ALPHA_EXECUTE_COMMAND_DIALOG_HPP
#define ALPHA_EXECUTE_COMMAND_DIALOG_HPP
#include "resource.h"
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
#include <queue>


namespace alpha {
	class Alpha;

	namespace ui {
		///	[コマンドの実行] ダイアログ
		class ExecuteCommandDlg : public manah::windows::ui::FixedIDDialog<IDD_DLG_EXECUTECOMMAND> {
			// コンストラクタ
		public:
			ExecuteCommandDlg(Alpha& app, HFONT ioFont);

			// メッセージハンドラ
		protected:
			void	onClose();											// WM_CLOSE
			bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);	// WM_INITDIALOG
			void	onCancel();											// IDCANCEL
			void	onOK();												// IDOK

		private:
			enum ControlState {WAIT_FOR_NEW_COMMAND, EXECUTING, INPUT_IDLE};
			void	appendInput(const char* first, const char* last);
			void	appendOutput(const char* first, const char* last);
			bool	execute(const std::wstring& commandLine, bool consoleProgram);
			bool	rescueUser();
			void	switchControls(ControlState state);

			// データメンバ
		private:
			Alpha& app_;
			HFONT ioFont_;
			std::wstring cmdLine_;
			std::queue<std::string> inputQueue_;
			bool executing_;	// コマンド実行中
			bool interrupted_;

			manah::windows::ui::ComboBox commandCombobox_;
			manah::windows::ui::Edit outputTextbox_;
			manah::windows::ui::Edit inputTextbox_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_COMBO_COMMAND, commandCombobox_)
				BIND_CONTROL(IDC_EDIT_OUTPUT, outputTextbox_)
				BIND_CONTROL(IDC_EDIT_INPUT, inputTextbox_)
			END_CONTROL_BINDING()
		};
	}
}

#endif /* !ALPHA_EXECUTE_COMMAND_DIALOG_HPP */
