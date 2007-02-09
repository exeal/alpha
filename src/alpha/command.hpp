/**
 * @file command.hpp
 * @author exeal
 * @date 2004-2007
 */

#ifndef ALPHA_COMMAND_HPP
#define ALPHA_COMMAND_HPP

#include "application.hpp"
#include "ankh.hpp"
#include "temporary-macro.hpp"
#include "../manah/win32/file.hpp"
#include "../manah/win32/ui/common-controls.hpp"
#include <map>
#define _COM_NO_STANDARD_GUIDS_
#include "msxml3.tlh"	// for MSXML2::ISAXAttributes


namespace alpha {
	class Alpha;

	namespace command {

		/// Alpha の全コマンドを管理
		class CommandManager {
		public:
			// 列挙
			enum IconState {ICONSTATE_NORMAL, ICONSTATE_DISABLED, ICONSTATE_HOT};
			// コンストラクタ
			explicit CommandManager(Alpha& app) throw();
			~CommandManager() throw();
			// メソッド
			bool									createImageList(const std::wstring& directory);
			bool									executeCommand(CommandID id, bool userContext);
			std::wstring							getCaption(CommandID id) const;
			std::wstring							getDescription(CommandID id) const;
			std::size_t								getIconIndex(CommandID id) const throw();
			const manah::windows::ui::ImageList&	getImageList(IconState state) const throw();
			CommandID								getLastCommand() const throw();
			std::wstring							getName(CommandID id) const;
			const TemporaryMacro&					getTemporaryMacro() const throw();
			bool									isChecked(CommandID id) const;
			bool									isEnabled(CommandID id, bool userContext) const;
			bool									isRecordable(CommandID id) const;

		private:
			Alpha& app_;
			TemporaryMacro temporaryMacro_;
			manah::windows::ui::ImageList icons_[3];
			std::map<CommandID, std::size_t> iconIndices_;
			CommandID lastCommandID_;
			static const COLORREF ICON_MASK_COLOR = RGB(0xFF, 0x00, 0xFF);
		};

		/// Alpha のコマンド
		class Command : manah::Noncopyable {
		public:
			/// デストラクタ
			virtual ~Command() throw() {}
			/// コマンドを実行する
			virtual bool execute() = 0;
			/// 組み込みコマンドの識別子を返す
			/// @throw std::logic_error	組みコマンドでない場合スロー
			virtual CommandID getID() const = 0;
			/// 組み込みコマンドかを調べる
			virtual bool isBuiltIn() const = 0;
		};

		/// キー割り当て可能コマンド
		class KeyAssignableCommand : virtual public Command {
		public:
			/// デストラクタ
			virtual ~KeyAssignableCommand() throw() {}
			/// 複製
			virtual  void copy(KeyAssignableCommand*& p) const = 0;
		};

		/// 永続記憶可能コマンド
		class SerializableCommand : virtual public Command {
		public:
			/// デストラクタ
			virtual ~SerializableCommand() throw() {}
			/// 複製
			virtual void copy(SerializableCommand*& p) const = 0;
			/// XML に保存する場合のテキスト片を返す
			virtual void getXMLOutput(std::wostringstream& os) const = 0;
		};

		/// 組み込みコマンド
		class BuiltInCommand : virtual public KeyAssignableCommand, virtual public SerializableCommand {
		public:
			explicit BuiltInCommand(CommandID id) : id_(id) {}
			void copy(KeyAssignableCommand*& p) const {p = new BuiltInCommand(id_);}
			void copy(SerializableCommand*& p) const {p = new BuiltInCommand(id_);}
			bool execute() {return Alpha::getInstance().commandManager_->executeCommand(id_, true);}
			CommandID getID() const {return id_;}
			void getXMLOutput(std::wostringstream& os) const {os << L"<built-in identifier=\"" << id_ << L"\" />\n";}
			bool isBuiltIn() const {return true;}
			static BuiltInCommand* parseXMLInput(
					const wchar_t* elementName, std::size_t elementNameLength, MSXML2::ISAXAttributes& attributes) {
				if(elementNameLength == 16 && std::wcsncmp(elementName, L"built-in", elementNameLength) == 0) {
					wchar_t* p;
					int cch;
					if(SUCCEEDED(attributes.getValueFromQName(
							reinterpret_cast<ushort*>(L"identifier"), 10, reinterpret_cast<ushort**>(&p), &cch)))
						return new BuiltInCommand(static_cast<CommandID>(std::wcstoul(p, 0, 10)));
				}
				return 0;
			}
		private:
			CommandID id_;
		};

		/// スクリプトコマンド
		class ScriptletCommand : virtual public KeyAssignableCommand {
		public:
			explicit ScriptletCommand(IDispatch& function) : function_(function) {function.AddRef();}
			~ScriptletCommand() {function_.Release();}
			void copy(KeyAssignableCommand*& p) const {p = new ScriptletCommand(function_);}
			bool execute() {return SUCCEEDED(ankh::ScriptSystem::callAnonymousFunction(function_));}
			CommandID getID() const {throw std::logic_error("This command is not built-in.");}
			bool isBuiltIn() const {return false;}
		private:
			IDispatch& function_;
		};

		/// 文字入力コマンド
		class CharacterInputCommand : virtual public SerializableCommand {
		public:
			explicit CharacterInputCommand(ascension::CodePoint cp) : cp_(cp) {}
			void copy(SerializableCommand*& p) const {p = new CharacterInputCommand(cp_);}
			bool execute() {return Alpha::getInstance().getBufferList().getActiveView().getCaret().inputCharacter(cp_, false, false);}
			CommandID getID() const {throw std::logic_error("This command is not built-in.");}
			void getXMLOutput(std::wostringstream& os) const {os << L"<char-input code-point=\"" << cp_ << L"\" />\n";}
			bool isBuiltIn() const {return false;}
			static CharacterInputCommand* parseXMLInput(
					const wchar_t* elementName, std::size_t elementNameLength, MSXML2::ISAXAttributes& attributes) {
				if(elementNameLength == 18 && std::wcsncmp(elementName, L"char-input", elementNameLength) == 0) {
					wchar_t* p;
					int cch;
					if(SUCCEEDED(attributes.getValueFromQName(
							reinterpret_cast<ushort*>(L"code-point"), 10, reinterpret_cast<ushort**>(&p), &cch)))
						return new CharacterInputCommand(std::wcstoul(p, 0, 10));
				}
				return 0;
			}
		private:
			ascension::CodePoint cp_;
		};

		/// テキスト入力コマンド
		class TextInputCommand : virtual public SerializableCommand {
		public:
			TextInputCommand(const ascension::String& text, bool asRectangle) : text_(text), asRectangle_(asRectangle) {}
			explicit TextInputCommand(bool asRectangle) : asRectangle_(asRectangle) {}
			void copy(SerializableCommand*& p) const {p = new TextInputCommand(text_, asRectangle_);}
			bool execute() {
				ascension::viewers::Caret& caret = Alpha::getInstance().getBufferList().getActiveView().getCaret();
				asRectangle_ ? caret.insertBox(text_) : caret.insert(text_);
				return true;}
			CommandID getID() const {throw std::logic_error("This command is not built-in.");}
			void getXMLOutput(std::wostringstream& os) const {
				os << L"<text-input";
				if(asRectangle_)
					os << L" rectangle=\"true\" ";
				os << L"><![CDATA[" << text_ << L"]]></text-input>\n";
			}
			bool isBuiltIn() const {return false;}
			static TextInputCommand* parseXMLInput(
					const wchar_t* elementName, std::size_t elementNameLength, MSXML2::ISAXAttributes& attributes) {
				if(elementNameLength == 18 && std::wcsncmp(elementName, L"text-input", elementNameLength) == 0) {
					wchar_t* p;
					int cch;
					if(SUCCEEDED(attributes.getValueFromQName(
							reinterpret_cast<ushort*>(L"rectangle"), 8, reinterpret_cast<ushort**>(&p), &cch))
							&& cch == 4 && std::wcsncmp(p, L"true", cch) == 0)
						return new TextInputCommand(true);
					else
						return new TextInputCommand(false);
				}
				return 0;
			}
			void setText(const ascension::String& text) {text_.assign(text);}
		private:
			ascension::String text_;
			bool asRectangle_;
		};

		/**
		 * コマンドに対応するアイコンのイメージリスト中の位置を返す
		 * @param id コマンド ID
		 * @return インデックス。アイコンが無い場合は -1
		 */
		inline std::size_t CommandManager::getIconIndex(CommandID id) const throw() {
			std::map<CommandID, std::size_t>::const_iterator it;
			return (iconIndices_.end() == (it = iconIndices_.find(id))) ? -1 : it->second;
		}

		/**
		 * イメージリストを返す
		 * @param state アイコンの状態
		 * @return イメージリスト
		 */
		inline const manah::windows::ui::ImageList& CommandManager::getImageList(CommandManager::IconState state) const throw() {return icons_[state];}

		/// キーボードマクロオブジェクトを返す
		inline const TemporaryMacro& CommandManager::getTemporaryMacro() const throw() {return temporaryMacro_;}

		/// 最後に実行したコマンドの識別値を返す
		inline CommandID CommandManager::getLastCommand() const throw() {return lastCommandID_;}

	} // namespace command
} // namespace alpha

#endif /* COMMAND_HPP_ */
