/**
 * @file save-some-buffers-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_SAVE_SOME_BUFFERS_DIALOG_HPP
#define ALPHA_SAVE_SOME_BUFFERS_DIALOG_HPP
#include "../manah/win32/ui/dialog.hpp"
#include "../manah/win32/ui/standard-controls.hpp"
#include "../manah/win32/ui/link-label.hpp"
#include <vector>

namespace alpha {
	namespace ui {
		/// 保存候補のファイル
		struct DirtyFile {
			unsigned int index;					///< 番号 (ダイアログクラスは使用しない)
			std::basic_string<WCHAR> fileName;	///< ファイル名
			bool save;							///< 保存するかどうか
		};

		/// [保存されていないバッファ] ダイアログ
		class SaveSomeBuffersDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_SAVESOMEBUFFERS> {
		protected:
			bool	onCommand(WORD id, WORD notifyCode, HWND control);
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);
			void	onOK();
		public:
			std::vector<DirtyFile> files_;
			manah::win32::ui::ListBox bufferListbox_;
			manah::win32::ui::LinkLabel selectAllLink_;
			manah::win32::ui::LinkLabel clearAllLink_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_LIST_BUFFERS, bufferListbox_)
			END_CONTROL_BINDING()
		};
	}
}

#endif /* !MANAH_SAVE_SOME_BUFFERS_DIALOG_HPP */
