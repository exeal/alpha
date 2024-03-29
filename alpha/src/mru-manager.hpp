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
	/// Most recently used files.
	class MRUManager {
		MANAH_NONCOPYABLE_TAG(MRUManager);
	public:
		// constructor
		MRUManager(std::size_t limit, int startID);
		// attributes
		const std::basic_string<WCHAR>&	at(std::size_t index) const;
		std::size_t						numberOfFiles() const throw();
		const manah::win32::ui::Menu&	popupMenu() const throw();
		void							setLimit(std::size_t newLimit);
		// operations
		void	add(const std::basic_string<WCHAR>& fileName);
		void	clear();
		void	remove(std::size_t index);
		// persistent
		void	load();
		void	save();
	private:
		void	updateMenu();

	private:
		const int startID_;									// メニュー ID の先頭の値
		std::list<std::basic_string<WCHAR> > fileNames_;	// フルパスのリスト
		manah::win32::ui::PopupMenu	popupMenu_;				// ポップアップメニュー
		std::size_t limitCount_;							// 項目数の上限 (4 以上 16 以下)
	};


	/// Removes all items.
	inline void MRUManager::clear() {fileNames_.clear(); updateMenu();}

	/// Returns the number of files.
	inline size_t MRUManager::numberOfFiles() const throw() {return fileNames_.size();}

	/// Returns the popup menu.
	inline const manah::win32::ui::Menu& MRUManager::popupMenu() const throw() {return popupMenu_;}

}

#endif /* !ALPHA_MRU_MANAGER_HPP */
