/**
 * @file save-some-buffers-dialog.hpp
 * @author exeal
 * @date 2003-2009
 */

#ifndef ALPHA_SAVE_SOME_BUFFERS_DIALOG_HPP
#define ALPHA_SAVE_SOME_BUFFERS_DIALOG_HPP
#include <manah/win32/ui/dialog.hpp>
#include <manah/win32/ui/standard-controls.hpp>
#include <manah/win32/ui/link-label.hpp>
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
		class SaveSomeBuffersDialog : public manah::win32::ui::FixedIDDialog<IDD_DLG_SAVESOMEBUFFERS> {
		protected:
			bool onCommand(WORD id, WORD notifyCode, HWND control);
			void onInitDialog(HWND focusWindow, bool& focusDefault);
			void onOK(bool& continueDialog);
		public:
			std::vector<DirtyFile> files_;
			manah::win32::Borrowed<manah::win32::ui::ListBox> bufferListbox_;
			manah::win32::Borrowed<manah::win32::ui::LinkLabel> selectAllLink_;
			manah::win32::Borrowed<manah::win32::ui::LinkLabel> clearAllLink_;
			MANAH_BEGIN_CONTROL_BINDING()
				MANAH_BIND_CONTROL(IDC_LIST_BUFFERS, bufferListbox_)
			MANAH_END_CONTROL_BINDING()
		};
	}
}

#endif // !ALPHA_SAVE_SOME_BUFFERS_DIALOG_HPP
