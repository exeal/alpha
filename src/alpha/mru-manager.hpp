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
	/// 最近使ったファイルの情報
	struct MRU {
		std::basic_string<WCHAR> fileName;			///< ファイル名
		ascension::encodings::CodePage codePage;	///< コードページ
	};

	/// 最近使ったファイルの管理
	class MRUManager : public manah::Noncopyable {
	public:
		// コンストラクタ
		MRUManager(std::size_t limit, int startID, bool ownerDrawMenu = false);
		// 属性
		std::size_t						getCount() const throw();
		const MRU&						getFileInfoAt(std::size_t index) const;
		const manah::windows::ui::Menu&	getPopupMenu() const throw();
		void							setLimit(std::size_t newLimit);
		// 操作
		void	add(const std::basic_string<WCHAR>& fileName, ascension::encodings::CodePage cp);
		void	clear();
		void	remove(std::size_t index);
	private:
		void	updateMenu();

	private:
		const int startID_;							// メニュー ID の先頭の値
		std::list<MRU> fileNames_;					// フルパスのリスト
		manah::windows::ui::Menu	popupMenu_;		// ポップアップメニュー
		std::size_t limitCount_;					// 項目数の上限 (4以上16以下)
		bool ownerDraw_;							// メニューをオーナードローにするか
	};


	/// 項目を全て削除する
	inline void MRUManager::clear() {fileNames_.clear(); updateMenu();}

	/// リストの項目数を返す
	inline size_t MRUManager::getCount() const throw() {return fileNames_.size();}

	/// ポップアップメニューを返す
	inline const manah::windows::ui::Menu& MRUManager::getPopupMenu() const throw() {return popupMenu_;}

}

#endif /* !ALPHA_MRU_MANAGER_HPP */
