/**
 * @file buffer.cpp
 * @author exeal
 * @date 2003-2006 was AlphaDoc.cpp and BufferList.cpp
 * @date 2006-2015
 */

#include "buffer.hpp"
#include "buffer-list.hpp"
#include "editor-panes.hpp"
#include "function-pointer.hpp"
//#include "command.hpp"
#include "../resource/messages.h"
#include <ascension/kernel/point.hpp>
#include <ascension/rules/lexical-partitioner.hpp>
#include <ascension/rules/transition-rule.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/python/stl_iterator.hpp>
#include <shlwapi.h>				// PathXxxx, StrXxxx, ...
#include <dlgs.h>

namespace alpha {
	namespace {
/*		class FileIOCallback : virtual public IFileIOListener {
		public:
			FileIOCallback(alpha::Alpha& app, bool forLoading, const WCHAR* fileName, CodePage encoding) throw()
					: app_(app), forLoading_(forLoading), fileName_(fileName),
					encoding_(encoding), retryWithOtherCodePage_(false) {}
			bool doesUserWantToChangeCodePage() const throw() {
				return retryWithOtherCodePage_;
			}
			bool unconvertableCharacterFound() {
				const DWORD id = (encoding_ < 0x10000) ?
					(encoding_ + MSGID_ENCODING_START) : (encoding_ - 60000 + MSGID_EXTENDED_ENCODING_START);
				const int answer = app_.messageBox(forLoading_ ?
					MSG_IO__UNCONVERTABLE_NATIVE_CHAR : MSG_IO__UNCONVERTABLE_UCS_CHAR, MB_YESNOCANCEL | MB_ICONEXCLAMATION,
					MARGS % fileName_ % Alpha::getInstance().loadMessage(id).c_str());
				if(answer == IDYES)
					retryWithOtherCodePage_ = true;
				return answer == IDNO;
			}
			IFileIOProgressListener* queryProgressCallback() {return 0;}
		private:
			Alpha& app_;
			const bool forLoading_;
			const WCHAR* const fileName_;
			const CodePage encoding_;
			bool retryWithOtherCodePage_;
		};
*/
		void saveBuffer(Buffer& buffer, const std::string& encoding = std::string(), ascension::text::Newline newlines = ascension::text::Newline::USE_INTRINSIC_VALUE,
				ascension::encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy = ascension::encoding::Encoder::DONT_SUBSTITUTE, bool writeUnicodeByteOrderMark = false) {
			if(buffer.textFile().isBoundToFile() && !buffer.isModified())
				return;

			ascension::kernel::fileio::WritingFormat format;
			format.encoding = !encoding.empty() ? encoding : buffer.textFile().encoding();
			format.encodingSubstitutionPolicy = encodingSubstitutionPolicy;
			format.newline = newlines.isLiteral() ? newlines : buffer.textFile().newline();
			format.unicodeByteOrderMark = writeUnicodeByteOrderMark;
			buffer.textFile().write(format, 0);
		}
	} // namespace @0


	// Buffer /////////////////////////////////////////////////////////////////////////////////////////////////////////

	/**
	 * Constructor.
	 * @param name The name of the buffer
	 * @throw std#invalid_argument @a name is empty
	 */
	Buffer::Buffer(const Glib::ustring& name) : name_(BufferList::instance().makeUniqueName(name)) {
		presentation_ = std::make_shared<ascension::presentation::Presentation>(*this);
		textFile_.reset(new ascension::kernel::fileio::TextFileDocumentInput(*this));
	}

	/// Returns the @c NameChangedSignal signal connector.
	ascension::SignalConnector<Buffer::NameChangedSignal> Buffer::nameChangedSignal() BOOST_NOEXCEPT {
		return nameChangedSignal_;
	}

	/**
	 * Changes the name of the buffer.
	 * @param newName The new name
	 * @param unique
	 */
	void Buffer::rename(const Glib::ustring& newName, bool unique /* = false */) {
		if(unique && BufferList::instance().forName(newName) != boost::python::object()) {
			const Glib::ustring message("Buffer name `" + newName + "' is in use");
			::PyErr_SetString(PyExc_ValueError, message.c_str());
			boost::python::throw_error_already_set();
		}
		name_ = BufferList::instance().makeUniqueName(newName);
		nameChangedSignal_(*this);
	}

#ifndef ALPHA_NO_AMBIENT
	namespace {
		class RegexTransitionRuleAdapter : public ascension::rules::RegexTransitionRule {
		public:
			RegexTransitionRuleAdapter(ascension::kernel::ContentType contentType,
				ascension::kernel::ContentType destination, const ascension::String& pattern, bool caseSensitive)
				: ascension::rules::RegexTransitionRule(contentType, destination,
					ascension::regex::Pattern::compile(pattern, caseSensitive ? 0 : ascension::regex::Pattern::CASE_INSENSITIVE)) {
			}
		};

		class DocumentPartitionerProxy {
		public:
			virtual void set(Buffer& buffer) = 0;
		};

		class LexicalPartitionerProxy : public DocumentPartitionerProxy, private boost::noncopyable {
		public:
			explicit LexicalPartitionerProxy(boost::python::object rules) {
				try {
					std::unique_ptr<ascension::rules::LexicalPartitioner> partitioner(new ascension::rules::LexicalPartitioner());
					std::forward_list<std::unique_ptr<const ascension::rules::TransitionRule>> cppRules;
					for(boost::python::stl_input_iterator<ascension::rules::TransitionRule*> i(rules), e; i != e; ++i)
						cppRules.push_front((*i)->clone());
					partitioner->setRules(cppRules);
					impl_.reset(partitioner.release());
				} catch(std::bad_alloc&) {
					::PyErr_NoMemory();
					boost::python::throw_error_already_set();
				}
			}
			void set(Buffer& buffer) {
				if(impl_.get() == 0) {
					::PyErr_SetString(PyExc_ValueError, "This ambient.rules.LexicalPartitioner had already been deleted.");
					boost::python::throw_error_already_set();
				}
				buffer.setPartitioner(std::move(impl_));
			}
		private:
			std::unique_ptr<ascension::kernel::DocumentPartitioner> impl_;
		};
/*
		template<typename T>
		struct LambdaPointer : LambdaPointer<decltype(&T::operator())> {};

		template<typename C, typename R, typename A1, typename A2>
		struct LambdaPointer<R(C::*)(A1, A2) const> {
			typedef R(*type)(A1, A2);
		};

		template<typename F>
		typename LambdaPointer<F>::type makeLambdaPointer(F f) {
			return f;
		}
*/	}

	ALPHA_EXPOSE_PROLOGUE(1)
		ambient::Interpreter& interpreter = ambient::Interpreter::instance();
		boost::python::scope temp(interpreter.toplevelPackage());

		boost::python::enum_<ascension::kernel::locations::CharacterUnit>("CharacterUnit")
			.value("utf16_code_unit", ascension::kernel::locations::UTF16_CODE_UNIT)
			.value("utf32_code_unit", ascension::kernel::locations::UTF32_CODE_UNIT)
			.value("grapheme_cluster", ascension::kernel::locations::GRAPHEME_CLUSTER)
			.value("glyph_cluster", ascension::kernel::locations::GLYPH_CLUSTER);

		boost::python::enum_<ascension::encoding::Encoder::SubstitutionPolicy>("EncodingSubstitutionPolicy")
			.value("dont_substitute", ascension::encoding::Encoder::DONT_SUBSTITUTE)
			.value("replace_unmappable_characters", ascension::encoding::Encoder::REPLACE_UNMAPPABLE_CHARACTERS)
			.value("ignore_unmappable_characters", ascension::encoding::Encoder::IGNORE_UNMAPPABLE_CHARACTERS);

/*		boost::python::enum_<f::IOException::Type>("FileIoError")
			.value("ok", static_cast<f::IOException::Type>(-1))
			.value("file_not_found", f::IOException::FILE_NOT_FOUND)
			.value("invalid_encoding", f::IOException::INVALID_ENCODING)
			.value("invalid_newline", f::IOException::INVALID_NEWLINE)
			.value("unmappable_character", f::IOException::UNMAPPABLE_CHARACTER)
			.value("malformed_input", f::IOException::MALFORMED_INPUT)
//			.value("out_of_memory", f::IOException::OUT_OF_MEMORY)
			.value("huge_file", f::IOException::HUGE_FILE)
//			.value("read_only_mode", f::IOException::READ_ONLY_MODE)
			.value("unwritable_file", f::IOException::UNWRITABLE_FILE)
			.value("cannot_create_temporary_file", f::IOException::CANNOT_CREATE_TEMPORARY_FILE)
			.value("lost_disk_file", f::IOException::LOST_DISK_FILE)
			.value("platform_dependent_error", f::IOException::PLATFORM_DEPENDENT_ERROR);
*/
		boost::python::enum_<ascension::kernel::fileio::TextFileDocumentInput::LockType>("FileLockMode")
			.value("no_lock", ascension::kernel::fileio::TextFileDocumentInput::NO_LOCK)
			.value("shared_lock", ascension::kernel::fileio::TextFileDocumentInput::SHARED_LOCK)
			.value("exclusive_lock", ascension::kernel::fileio::TextFileDocumentInput::EXCLUSIVE_LOCK);

		boost::python::class_<ascension::text::Newline>("Newline", boost::python::init<>())
			.def_readonly("line_feed", &ascension::text::Newline::LINE_FEED)
			.def_readonly("carriage_return", &ascension::text::Newline::CARRIAGE_RETURN)
			.def_readonly("carriage_return_followed_by_line_feed", &ascension::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED)
			.def_readonly("next_line", &ascension::text::Newline::NEXT_LINE)
			.def_readonly("line_separator", &ascension::text::Newline::LINE_SEPARATOR)
			.def_readonly("paragraph_separator", &ascension::text::Newline::PARAGRAPH_SEPARATOR)
			.def_readonly("use_intrinsic_value", &ascension::text::Newline::USE_INTRINSIC_VALUE)
			.def_readonly("use_document_input", &ascension::text::Newline::USE_DOCUMENT_INPUT);

		boost::python::enum_<ascension::kernel::fileio::UnexpectedFileTimeStampDirector::Context>("UnexpectedFileTimeStampContext")
			.value("first_modification", ascension::kernel::fileio::UnexpectedFileTimeStampDirector::FIRST_MODIFICATION)
			.value("overwrite_file", ascension::kernel::fileio::UnexpectedFileTimeStampDirector::OVERWRITE_FILE)
			.value("client_invocation", ascension::kernel::fileio::UnexpectedFileTimeStampDirector::CLIENT_INVOCATION);

		boost::python::class_<ascension::Direction>("Direction", boost::python::no_init)
//			.def("__getattr__", &attrOfDirection)/*
			.def_readonly("forward", &ascension::Direction::forward())
			.def_readonly("backward", &ascension::Direction::backward());

		boost::python::class_<ascension::kernel::Position>("Position", boost::python::init<>())
			.def(boost::python::init<ascension::Index, ascension::Index>())
			.def_readwrite("line", &ascension::kernel::Position::line)
			.def_readwrite("offset_in_line", &ascension::kernel::Position::offsetInLine);

		boost::python::class_<ascension::kernel::Region>("Region", boost::python::init<>())
			.def(boost::python::init<const ascension::kernel::Position&, const ascension::kernel::Position&>())
			.def(boost::python::init<const ascension::kernel::Position&>())
			.def_readwrite("first", &ascension::kernel::Region::first)
			.def_readwrite("second", &ascension::kernel::Region::second)
			.def<ascension::kernel::Position& (ascension::kernel::Region::*)(void)>("beginning", &ascension::kernel::Region::beginning, boost::python::return_value_policy<boost::python::reference_existing_object>())
			.def("encompasses", &ascension::kernel::Region::encompasses)
			.def<ascension::kernel::Position& (ascension::kernel::Region::*)(void)>("end", &ascension::kernel::Region::end, boost::python::return_value_policy<boost::python::reference_existing_object>())
			.def("includes", &ascension::kernel::Region::includes)
			.def("intersection", &ascension::kernel::Region::getIntersection, boost::python::return_value_policy<boost::python::return_by_value>())
			.def("intersects_with", &ascension::kernel::Region::intersectsWith)
			.def("is_empty", &ascension::kernel::Region::isEmpty)
			.def("is_normalized", &ascension::kernel::Region::isNormalized)
			.def("normalize", &ascension::kernel::Region::normalize, boost::python::return_value_policy<boost::python::reference_existing_object>())
			.def("union", &ascension::kernel::Region::getUnion, boost::python::return_value_policy<boost::python::return_by_value>());

		boost::python::class_<ascension::kernel::Bookmarker, boost::noncopyable>("_Bookmarker", boost::python::no_init)
			.def("__len__", &ascension::kernel::Bookmarker::numberOfMarks)
			.def("clear", &ascension::kernel::Bookmarker::clear)
			.def("is_marked", &ascension::kernel::Bookmarker::isMarked)
			.def("mark", &ascension::kernel::Bookmarker::mark, (boost::python::arg("line"), boost::python::arg("set") = true))
			.def("next", &ascension::kernel::Bookmarker::next, (boost::python::arg("from"), boost::python::arg("direction"), boost::python::arg("wrap_around") = true, boost::python::arg("marks") = 1))
			.def("toggle", &ascension::kernel::Bookmarker::toggle);

		boost::python::class_<Buffer, boost::noncopyable>("_Buffer", boost::python::no_init)
			.add_property("accessible_region", &Buffer::accessibleRegion)
			.add_property("bookmarker", boost::python::make_function<
				ascension::kernel::Bookmarker& (Buffer::*)(void), boost::python::return_value_policy<boost::python::reference_existing_object>
				>(&Buffer::bookmarker, boost::python::return_value_policy<boost::python::reference_existing_object>()))
			.add_property("encoding",
				ambient::makeFunctionPointer([](const Buffer& buffer) {
					return buffer.textFile().encoding();
				}),
				ambient::makeFunctionPointer([](Buffer& buffer, const std::string& encoding) {
					buffer.textFile().setEncoding(encoding);
				}))
			.add_property("location", ambient::makeFunctionPointer([](const Buffer& buffer) {
				return buffer.textFile().location();
			}))
			.add_property("name", boost::python::make_function(&Buffer::name, boost::python::return_internal_reference<>()))
			.add_property("newline", 
				ambient::makeFunctionPointer([](const Buffer& buffer) {
					return buffer.textFile().newline();
				}),
				ambient::makeFunctionPointer([](Buffer& buffer, ascension::text::Newline newline) {
					buffer.textFile().setNewline(newline);
				}))
			.add_property("number_of_lines", &Buffer::numberOfLines)
			.add_property("number_of_redoable_changes", &Buffer::numberOfRedoableChanges)
			.add_property("number_of_undoable_changes", &Buffer::numberOfUndoableChanges)
			.add_property("read_only", &Buffer::isReadOnly, &Buffer::setReadOnly)
			.add_property("records_changes", &Buffer::isRecordingChanges, &Buffer::recordChanges)
			.add_property("region", &Buffer::region)
			.add_property("revision_number", &Buffer::revisionNumber)
			.add_property("unicode_byte_order_mark", ambient::makeFunctionPointer([](const Buffer& buffer) {
				return buffer.textFile().unicodeByteOrderMark();
			}))
			.def("begin_compound_change", &Buffer::beginCompoundChange)
			.def("bind_file", ambient::makeFunctionPointer([](Buffer& buffer, const boost::filesystem::path::string_type& fileName) {
				buffer.textFile().bind(fileName);
			}))
			.def("clear_undo_buffer", &Buffer::clearUndoBuffer)
			.def("close", ambient::makeFunctionPointer([](Buffer& buffer) {
				BufferList::instance().close(buffer);
			}))
			.def("end_compound_change", &Buffer::endCompoundChange)
			.def<void (*)(ascension::kernel::Document&, const ascension::kernel::Region&)>("erase", &ascension::kernel::erase)
			.def("insert", ambient::makeFunctionPointer([](Buffer& buffer, const ascension::kernel::Position& at, const ascension::String& text) -> ascension::kernel::Position {
				ascension::kernel::Position temp;
				ascension::kernel::insert(buffer, at, text, &temp);
				return temp;
			}))
			.def("insert_file_contents", &ascension::kernel::fileio::insertFileContents)
			.def("insert_undo_boundary", &Buffer::insertUndoBoundary)
			.def("is_bound_to_file", ambient::makeFunctionPointer([](const Buffer& buffer) {
				return buffer.textFile().isBoundToFile();
			}))
			.def("is_compound_changing", &Buffer::isCompoundChanging)
			.def("is_modified", &Buffer::isModified)
			.def("is_narrowed", &Buffer::isNarrowed)
			.def("is_selected", ambient::makeFunctionPointer([](const Buffer& buffer) {
				return &buffer == &EditorPanes::instance().selectedBuffer();
			}))
			.def("length", &Buffer::length, boost::python::arg("newline") = ascension::text::Newline::USE_INTRINSIC_VALUE)
			.def("line", &Buffer::line, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("lock_file", ambient::makeFunctionPointer([](Buffer& buffer, ascension::kernel::fileio::TextFileDocumentInput::LockType type, bool onlyAsEditing) {
				ascension::kernel::fileio::TextFileDocumentInput::LockMode mode;
				mode.type = type;
				mode.onlyAsEditing = onlyAsEditing;
				buffer.textFile().lockFile(mode);
			}))
			.def("mark_unmodified", &Buffer::markUnmodified)
			.def("narrow_to_region", &Buffer::narrowToRegion)
			.def("redo", &Buffer::redo, boost::python::arg("n") = 1)
			.def("rename", &Buffer::rename, boost::python::arg("new_name"), boost::python::arg("unique") = false)
			.def("replace", ambient::makeFunctionPointer([](Buffer& buffer, const ascension::kernel::Region& region, const ascension::String& text) -> ascension::kernel::Position {
				ascension::kernel::Position temp;
				buffer.replace(region, text, &temp);
				return temp;
			}))
			.def("reset_content", &Buffer::resetContent)
			.def("revert_to_file",
				ambient::makeFunctionPointer([](Buffer& buffer, const std::string& encoding, ascension::encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy) {
					buffer.textFile().revert(encoding, encodingSubstitutionPolicy);
				}),
				(boost::python::arg("encoding") = std::string(), boost::python::arg("encoding_substitution_policy") = ascension::encoding::Encoder::DONT_SUBSTITUTE))
			.def("unbind_file", ambient::makeFunctionPointer([](Buffer& buffer) {
				buffer.textFile().unbind();
			}))
			.def("unlock_file", ambient::makeFunctionPointer([](Buffer& buffer) {
				buffer.textFile().unlockFile();
			}))
			.def("save", &saveBuffer,
				(boost::python::arg("encoding") = std::string(), boost::python::arg("newlines") = ascension::text::Newline::USE_INTRINSIC_VALUE,
				boost::python::arg("encoding_substitution_policy") = ascension::encoding::Encoder::DONT_SUBSTITUTE,
				boost::python::arg("write_unicode_byte_order_mark") = false))
			.def("set_partitioner", ambient::makeFunctionPointer([](Buffer& buffer, DocumentPartitionerProxy* partitioner) {
				(partitioner != nullptr) ? partitioner->set(buffer) : buffer.setPartitioner(std::unique_ptr<ascension::kernel::DocumentPartitioner>());
			}))
			.def("set_modified", &Buffer::setModified)
			.def("undo", &Buffer::undo, boost::python::arg("n") = 1)
			.def("widen", &Buffer::widen)
			.def("write_region",
				ambient::makeFunctionPointer([](const Buffer& buffer, const ascension::kernel::Region& region, const boost::filesystem::path::string_type& fileName,
						bool append, const std::string& encoding, ascension::text::Newline newlines,
						ascension::encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy, bool writeUnicodeByteOrderMark) {
					ascension::kernel::fileio::WritingFormat format;
					format.encoding = encoding;
					format.newline = newlines;
					format.encodingSubstitutionPolicy = encodingSubstitutionPolicy;
					format.unicodeByteOrderMark = writeUnicodeByteOrderMark;
					ascension::kernel::fileio::writeRegion(buffer, region, fileName, format, append);
				}),
				(boost::python::arg("region"), boost::python::arg("filename"), boost::python::arg("append") = false,
				boost::python::arg("encoding") = std::string(), boost::python::arg("newlines") = ascension::text::Newline::USE_INTRINSIC_VALUE,
				boost::python::arg("encoding_substitution_policy") = ascension::encoding::Encoder::DONT_SUBSTITUTE,
				boost::python::arg("write_unicode_byte_order_mark") = false));
		boost::python::class_<ascension::kernel::DocumentPartitioner, boost::noncopyable>("_DocumentPartitioner", boost::python::no_init);

		interpreter.installException<ascension::kernel::fileio::UnmappableCharacterException>(
			"UnmappableCharacterError", boost::python::object(boost::python::borrowed(PyExc_IOError)));
		interpreter.installException<ascension::text::MalformedInputException<const std::uint8_t*>>(
			"MalformedInputError", boost::python::object(boost::python::borrowed(PyExc_IOError)));

		boost::python::register_exception_translator<boost::filesystem::filesystem_error>(
			[](const boost::filesystem::filesystem_error& e) {
#if BOOST_OS_WINDOWS
				::PyErr_SetFromWindowsErr(e.code().value());
#else
				assert(errno == e.code().value());
				::PyErr_SetFromErrno(PyExc_SystemError);
#endif
			}
		);
/*
		boost::python::scope ruleModule(interpreter.module("rules"));
		boost::python::class_<a::rules::TransitionRule, boost::noncopyable>("_TransitionRule", py::no_init)
			.add_property("content_type", &a::rules::TransitionRule::contentType)
			.add_property("destination", &a::rules::TransitionRule::destination);
		boost::python::class_<a::rules::LiteralTransitionRule, py::bases<a::rules::TransitionRule> >(
			"LiteralTransitionRule", py::init<k::ContentType, k::ContentType, wstring, a::CodePoint, bool>((
				py::arg("content_type"), py::arg("destination"), py::arg("pattern"),
				py::arg("escape_character") = static_cast<long>(a::NONCHARACTER), py::arg("case_sensitive") = true)));
		boost::python::class_<RegexTransitionRuleAdapter, py::bases<a::rules::TransitionRule> >(
			"RegexTransitionRule", py::init<k::ContentType, k::ContentType, wstring, bool>((
				py::arg("content_type"), py::arg("destination"), boost::python::arg("pattern"), boost::python::arg("case_sensitive") = true)));
		boost::python::class_<DocumentPartitionerProxy, boost::noncopyable>("_DocumentPartitioner", boost::python::no_init);
		boost::python::class_<LexicalPartitionerProxy, boost::python::bases<DocumentPartitionerProxy>,
			boost::noncopyable>("LexicalPartitioner", boost::python::init<boost::python::object>());
*/	ALPHA_EXPOSE_EPILOGUE()
#endif // !ALPHA_NO_AMBIENT
}
