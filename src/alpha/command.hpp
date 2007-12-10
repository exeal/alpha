/**
 * @file command.hpp
 * @author exeal
 * @date 2004-2007
 */

#ifndef ALPHA_COMMAND_HPP
#define ALPHA_COMMAND_HPP

#include "application.hpp"
#include "ankh/core.hpp"
#include "temporary-macro.hpp"
#include "../manah/win32/file.hpp"
#include "../manah/win32/ui/common-controls.hpp"
#include <map>
#define _COM_NO_STANDARD_GUIDS_
#include "msxml4.tlh"	// MSXML2.ISAXAttributes


namespace alpha {
	class Alpha;

	namespace command {

		/// Manages all commands in Alpha.
		class CommandManager {
		public:
			// enumeration
			enum IconState {ICONSTATE_NORMAL, ICONSTATE_DISABLED, ICONSTATE_HOT};
			// constructor
			CommandManager() throw();
			// display names
			std::wstring	caption(CommandID id) const;
			std::wstring	description(CommandID id) const;
			std::wstring	menuName(command::CommandID id) const;
			std::wstring	name(CommandID id) const;
			// icons
			bool								createImageList(const std::wstring& directory);
			std::size_t							iconIndex(CommandID id) const throw();
			const manah::win32::ui::ImageList&	imageList(IconState state) const throw();
			// states
			bool		isChecked(CommandID id) const;
			bool		isEnabled(CommandID id, bool userContext) const;
			bool		isRecordable(CommandID id) const;
			CommandID	lastCommand() const throw();
			// attribute
			const TemporaryMacro&	temporaryMacro() const throw();
			// operation
			bool	executeCommand(CommandID id, bool userContext);

		private:
			TemporaryMacro temporaryMacro_;
			manah::win32::ui::ImageList icons_[3];
			std::map<CommandID, std::size_t> iconIndices_;
			CommandID lastCommandID_;
			static const COLORREF ICON_MASK_COLOR = RGB(0xFF, 0x00, 0xFF);
		};

		/// A command.
		class Command {
		public:
			/// Destructor.
			virtual ~Command() throw() {}
			/// Executes the command.
			virtual bool execute() = 0;
			/// Returns the identifier of the built-in command.
			/// @throw std#logic_error the command is not built-in
			virtual CommandID getID() const = 0;
			/// Returns true of the command is built-in.
			virtual bool isBuiltIn() const = 0;
		};

		/// A command can assign to a key combination.
		class KeyAssignableCommand : virtual public Command {
		public:
			/// Destructor.
			virtual ~KeyAssignableCommand() throw() {}
			/// Duplicates the object.
			virtual  void copy(KeyAssignableCommand*& p) const = 0;
		};

		/// A command can save.
		class SerializableCommand : virtual public Command {
		public:
			/// Destructor.
			virtual ~SerializableCommand() throw() {}
			/// Duplicates the object.
			virtual void copy(SerializableCommand*& p) const = 0;
			/// Returns the XML string to persist.
			virtual void getXMLOutput(std::wostringstream& os) const = 0;
		};

		/// A built-in command.
		class BuiltInCommand : virtual public KeyAssignableCommand, virtual public SerializableCommand {
		public:
			explicit BuiltInCommand(CommandID id) : id_(id) {}
			void copy(KeyAssignableCommand*& p) const {p = new BuiltInCommand(id_);}
			void copy(SerializableCommand*& p) const {p = new BuiltInCommand(id_);}
			bool execute() {return Alpha::instance().commandManager_->executeCommand(id_, true);}
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

		/// A script command.
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

		/// A character input command.
		class CharacterInputCommand : virtual public SerializableCommand {
		public:
			explicit CharacterInputCommand(ascension::CodePoint cp) : cp_(cp) {}
			void copy(SerializableCommand*& p) const {p = new CharacterInputCommand(cp_);}
			bool execute() {return Alpha::instance().bufferList().activeView().caret().inputCharacter(cp_, false, false);}
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

		/// A string input command.
		class TextInputCommand : virtual public SerializableCommand {
		public:
			TextInputCommand(const ascension::String& text, bool asRectangle) : text_(text), asRectangle_(asRectangle) {}
			explicit TextInputCommand(bool asRectangle) : asRectangle_(asRectangle) {}
			void copy(SerializableCommand*& p) const {p = new TextInputCommand(text_, asRectangle_);}
			bool execute() {
				ascension::viewers::Caret& caret = Alpha::instance().bufferList().activeView().caret();
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
		 * Returns the index of the icon assoicated with the specified command.
		 * @param id the identifier of the command
		 * @return the index or -1 if not found
		 */
		inline std::size_t CommandManager::iconIndex(CommandID id) const throw() {
			std::map<CommandID, std::size_t>::const_iterator it;
			return (iconIndices_.end() == (it = iconIndices_.find(id))) ? -1 : it->second;
		}

		/**
		 * Returns the icons.
		 * @param state the state of the icons to retrieve
		 * @return the image list contains icons
		 */
		inline const manah::win32::ui::ImageList& CommandManager::imageList(CommandManager::IconState state) const throw() {return icons_[state];}

		/// Returns the identifier of the command most recently used.
		inline CommandID CommandManager::lastCommand() const throw() {return lastCommandID_;}

		/// Returns the temporary macro manager.
		inline const TemporaryMacro& CommandManager::temporaryMacro() const throw() {return temporaryMacro_;}

	} // namespace command
} // namespace alpha

#endif /* !ALPHA_COMMAND_HPP */
