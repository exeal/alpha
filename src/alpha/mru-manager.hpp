/**
 * @file mru-manager.hpp
 * @author exeal
 * @date 2003-2007
 */

#ifndef ALPHA_MRU_MANAGER_HPP
#define ALPHA_MRU_MANAGER_HPP

#include <list>
#include "../manah/win32/ui/menu.hpp"
#include "ascension/encoder.hpp"


namespace alpha {
	/// �ŋߎg�����t�@�C���̏��
	struct MRU {
		std::basic_string<WCHAR> fileName;			///< �t�@�C����
		ascension::encodings::CodePage codePage;	///< �R�[�h�y�[�W
	};

	/// �ŋߎg�����t�@�C���̊Ǘ�
	class MRUManager : public manah::Noncopyable {
	public:
		// �R���X�g���N�^
		MRUManager(std::size_t limit, int startID, bool ownerDrawMenu = false);
		// ����
		std::size_t						getCount() const throw();
		const MRU&						getFileInfoAt(std::size_t index) const;
		const manah::windows::ui::Menu&	getPopupMenu() const throw();
		void							setLimit(std::size_t newLimit);
		// ����
		void	add(const std::basic_string<WCHAR>& fileName, ascension::encodings::CodePage cp);
		void	clear();
		void	remove(std::size_t index);
	private:
		void	updateMenu();

	private:
		const int startID_;							// ���j���[ ID �̐擪�̒l
		std::list<MRU> fileNames_;					// �t���p�X�̃��X�g
		manah::windows::ui::Menu	popupMenu_;		// �|�b�v�A�b�v���j���[
		std::size_t limitCount_;					// ���ڐ��̏�� (4�ȏ�16�ȉ�)
		bool ownerDraw_;							// ���j���[���I�[�i�[�h���[�ɂ��邩
	};


	/// ���ڂ�S�č폜����
	inline void MRUManager::clear() {fileNames_.clear(); updateMenu();}

	/// ���X�g�̍��ڐ���Ԃ�
	inline size_t MRUManager::getCount() const throw() {return fileNames_.size();}

	/// �|�b�v�A�b�v���j���[��Ԃ�
	inline const manah::windows::ui::Menu& MRUManager::getPopupMenu() const throw() {return popupMenu_;}

}

#endif /* !ALPHA_MRU_MANAGER_HPP */
