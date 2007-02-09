/**
 * @file bookmark-dialog.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_BOOKMARK_DIALOG_HPP
#define ALPHA_BOOKMARK_DIALOG_HPP
#include "resource.h"
#include "ascension/common.hpp"
#include "../manah/win32/ui/dialog.hpp"
#include <map>


namespace alpha {
	class Alpha;
	class Buffer;

	namespace ui {
		///	[�u�b�N�}�[�N] �_�C�A���O
		class BookmarkDlg : public manah::windows::ui::Layered<manah::windows::ui::FixedIDDialog<IDD_DLG_BOOKMARKS> > {
		public:
			// �R���X�g���N�^
			BookmarkDlg(Alpha& app);
			// ���\�b�h
			void	updateList();

		private:
			void	getItemInfo(int index, Buffer*& buffer, ascension::length_t& line) const;
			void	onBtnDelete();

			// ���b�Z�[�W�n���h��
		protected:
			void	onClose();											// WM_CLOSE
			bool	onCommand(WORD id, WORD notifyCode, HWND control);	// WM_COMMAND
			bool	onInitDialog(HWND focusWindow, LPARAM initParam);	// WM_INITDIALOG
			bool	onNotify(int id, LPNMHDR nmhdr);					// WM_NOTIFY
			void	onOK();												// IDOK

			// �f�[�^�����o
		private:
			Alpha& app_;
			std::map<Buffer*, std::size_t> bufferIndices_;	// �o�b�t�@ -> �o�b�t�@�ԍ�
			manah::windows::ui::ListCtrl bookmarksList_;

			BEGIN_CONTROL_BINDING()
				BIND_CONTROL(IDC_LIST_BOOKMARKS, bookmarksList_)
			END_CONTROL_BINDING()
		};
	}
}

#endif /* !ALPHA_BOOKMARK_DIALOG_HPP */
