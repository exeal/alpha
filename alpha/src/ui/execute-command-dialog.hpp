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
		/**
		 * "Execute Command" dialog box.
		 * @deprecated
		 */
		class ExecuteCommandDlg : public manah::win32::ui::FixedIDDialog<IDD_DLG_EXECUTECOMMAND> {
			// コンストラクタ
		public:
			explicit ExecuteCommandDlg(HFONT ioFont);

			// メッセージハンドラ
		protected:
			void	onClose(bool& continueDialog);						// WM_CLOSE
			bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			void	onInitDialog(HWND focusWindow, bool& focusDefault);	// WM_INITDIALOG
			void	onCancel(bool& continueDialog);						// IDCANCEL
			void	onOK(bool& continueDialog);							// IDOK

		private:
			enum ControlState {WAIT_FOR_NEW_COMMAND, EXECUTING, INPUT_IDLE};
			void	appendInput(const char* first, const char* last);
			void	appendOutput(const char* first, const char* last);
			bool	execute(const std::wstring& commandLine, bool consoleProgram);
			bool	rescueUser();
			void	switchControls(ControlState state);

			// データメンバ
		private:
			HFONT ioFont_;
			std::wstring cmdLine_;
			std::queue<std::string> inputQueue_;
			bool executing_;	// コマンド実行中
			bool interrupted_;

			manah::win32::ui::ComboBox commandCombobox_;
			manah::win32::ui::Edit outputTextbox_;
			manah::win32::ui::Edit inputTextbox_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_COMBO_COMMAND, commandCombobox_)
				MANAH_BIND_CONTROL(IDC_EDIT_OUTPUT, outputTextbox_)
				MANAH_BIND_CONTROL(IDC_EDIT_INPUT, inputTextbox_)
			MANAH_END_CONTROL_BINDING()
		};
	}
}

#endif /* !ALPHA_EXECUTE_COMMAND_DIALOG_HPP */
