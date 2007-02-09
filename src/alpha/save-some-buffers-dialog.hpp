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
		/// �ۑ����̃t�@�C��
		struct DirtyFile {
			unsigned int index;					///< �ԍ� (�_�C�A���O�N���X�͎g�p���Ȃ�)
			std::basic_string<WCHAR> fileName;	///< �t�@�C����
			bool save;							///< �ۑ����邩�ǂ���
		};

		/// [�ۑ�����Ă��Ȃ��o�b�t�@] �_�C�A���O
		class SaveSomeBuffersDlg : public manah::windows::ui::FixedIDDialog<IDD_DLG_SAVESOMEBUFFERS> {
			// ���b�Z�[�W�n���h��
		protected:
			bool	onCommand(WORD id, WORD notifyCode, HWND control);
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);
			void	onOK();

			// �f�[�^�����o
		public:
			std::vector<DirtyFile> files_;
			manah::windows::ui::ListBox bufferListbox_;
			manah::windows::ui::LinkLabel selectAllLink_;
			manah::windows::ui::LinkLabel clearAllLink_;
			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_LIST_BUFFERS, bufferListbox_)
			END_CONTROL_BINDING()
		};
	}
}

#endif /* !MANAH_SAVE_SOME_BUFFERS_DIALOG_HPP */
