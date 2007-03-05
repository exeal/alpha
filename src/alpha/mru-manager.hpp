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
	/// A most recently used file.
	struct MRU {
		std::basic_string<WCHAR> fileName;			///< The file name.
		ascension::encodings::CodePage codePage;	///< The code page.
	};

	/// Most recently used files.
	class MRUManager : public manah::Noncopyable {
	public:
		// constructor
		MRUManager(std::size_t limit, int startID);
		// attributes
		std::size_t						getCount() const throw();
		const MRU&						getFileInfoAt(std::size_t index) const;
		const manah::win32::ui::Menu&	getPopupMenu() const throw();
		void							setLimit(std::size_t newLimit);
		// operations
		void	add(const std::basic_string<WCHAR>& fileName, ascension::encodings::CodePage cp);
		void	clear();
		void	remove(std::size_t index);
	private:
		void	updateMenu();

	private:
		const int startID_;						// ���j���[ ID �̐擪�̒l
		std::list<MRU> fileNames_;				// �t���p�X�̃��X�g
		manah::win32::ui::PopupMenu	popupMenu_;	// �|�b�v�A�b�v���j���[
		std::size_t limitCount_;				// ���ڐ��̏�� (4�ȏ�16�ȉ�)
	};


	/// Removes all items.
	inline void MRUManager::clear() {fileNames_.clear(); updateMenu();}

	/// Returns the number of files.
	inline size_t MRUManager::getCount() const throw() {return fileNames_.size();}

	/// Returns the popup menu.
	inline const manah::win32::ui::Menu& MRUManager::getPopupMenu() const throw() {return popupMenu_;}

}

#endif /* !ALPHA_MRU_MANAGER_HPP */
