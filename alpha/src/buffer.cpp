/**
 * @file buffer.cpp
 * @author exeal
 * @date 2003-2006 was AlphaDoc.cpp and BufferList.cpp
 * @date 2006-2013
 */

#include "buffer.hpp"
//#include "editor-window.hpp"
//#include "command.hpp"
#include "../resource/messages.h"
#include <ascension/kernel/point.hpp>
#include <ascension/rules.hpp>
#include <boost/python/stl_iterator.hpp>
#include <shlwapi.h>				// PathXxxx, StrXxxx, ...
#include <dlgs.h>
using namespace alpha;
using namespace ambient;
using namespace std;
namespace a = ascension;
namespace k = ascension::kernel;
namespace py = boost::python;

namespace {
/*	class FileIOCallback : virtual public IFileIOListener {
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
	};*/
	void saveBuffer(Buffer& buffer, const string& encoding = string(), a::text::Newline newlines = a::text::Newline::USE_INTRINSIC_VALUE,
			a::encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy = a::encoding::Encoder::DONT_SUBSTITUTE, bool writeUnicodeByteOrderMark = false) {
		if(buffer.textFile().isBoundToFile() && !buffer.isModified())
			return;

		k::fileio::WritingFormat format;
		format.encoding = !encoding.empty() ? encoding : buffer.textFile().encoding();
		format.encodingSubstitutionPolicy = encodingSubstitutionPolicy;
		format.newline = newlines.isLiteral() ? newlines : buffer.textFile().newline();
		format.unicodeByteOrderMark = writeUnicodeByteOrderMark;
		buffer.textFile().write(format, 0);
	}
} // namespace @0


// Buffer ///////////////////////////////////////////////////////////////////

/// Default constructor.
Buffer::Buffer() {
	presentation_.reset(new a::presentation::Presentation(*this));
	textFile_.reset(new k::fileio::TextFileDocumentInput(*this));
}

/**
 * Returns the name of the buffer.
 * @see BufferList#getDisplayName
 */
k::fileio::PathString Buffer::name() const /*throw()*/ {
	static const wstring untitled(Alpha::instance().loadMessage(MSG_BUFFER__UNTITLED));
	return textFile_->isBoundToFile() ? ::PathFindFileNameW(textFile_->location().c_str()) : untitled;
	
}

/// Returns the presentation object of Ascension.
a::presentation::Presentation& Buffer::presentation() BOOST_NOEXCEPT {
	return *presentation_;
}

/// Returns the presentation object of Ascension.
const a::presentation::Presentation& Buffer::presentation() const BOOST_NOEXCEPT {
	return *presentation_;
}

namespace {
	class RegexTransitionRuleAdapter : public a::rules::RegexTransitionRule {
	public:
		RegexTransitionRuleAdapter(k::ContentType contentType,
			k::ContentType destination, const wstring& pattern, bool caseSensitive)
			: a::rules::RegexTransitionRule(contentType, destination,
				a::regex::Pattern::compile(pattern, caseSensitive ? 0 : a::regex::Pattern::CASE_INSENSITIVE)) {
		}
	};

	class DocumentPartitionerProxy {
	public:
		virtual void set(Buffer& buffer) = 0;
	};

	class LexicalPartitionerProxy : public DocumentPartitionerProxy {
		ASCENSION_NONCOPYABLE_TAG(LexicalPartitionerProxy);
	public:
		explicit LexicalPartitionerProxy(py::object rules) {
			try {
				auto_ptr<a::rules::LexicalPartitioner> partitioner(new a::rules::LexicalPartitioner());
				partitioner->setRules(py::stl_input_iterator<a::rules::TransitionRule*>(rules), py::stl_input_iterator<a::rules::TransitionRule*>());
				impl_.reset(partitioner.release());
			} catch(bad_alloc&) {
				::PyErr_NoMemory();
				py::throw_error_already_set();
			}
		}
		void set(Buffer& buffer) {
			if(impl_.get() == 0) {
				::PyErr_SetString(PyExc_ValueError, "This ambient.rules.LexicalPartitioner had already been deleted.");
				py::throw_error_already_set();
			}
			buffer.setPartitioner(move(impl_));
		}
	private:
		unique_ptr<k::DocumentPartitioner> impl_;
	};
}

ALPHA_EXPOSE_PROLOGUE(1)
	Interpreter& interpreter = Interpreter::instance();
	py::scope temp(interpreter.toplevelPackage());

	py::enum_<k::locations::CharacterUnit>("CharacterUnit")
		.value("utf16_code_unit", k::locations::UTF16_CODE_UNIT)
		.value("utf32_code_unit", k::locations::UTF32_CODE_UNIT)
		.value("grapheme_cluster", k::locations::GRAPHEME_CLUSTER)
		.value("glyph_cluster", k::locations::GLYPH_CLUSTER);

	py::enum_<a::encoding::Encoder::SubstitutionPolicy>("EncodingSubstitutionPolicy")
		.value("dont_substitute", a::encoding::Encoder::DONT_SUBSTITUTE)
		.value("replace_unmappable_characters", a::encoding::Encoder::REPLACE_UNMAPPABLE_CHARACTERS)
		.value("ignore_unmappable_characters", a::encoding::Encoder::IGNORE_UNMAPPABLE_CHARACTERS);

/*	py::enum_<f::IOException::Type>("FileIoError")
		.value("ok", static_cast<f::IOException::Type>(-1))
		.value("file_not_found", f::IOException::FILE_NOT_FOUND)
		.value("invalid_encoding", f::IOException::INVALID_ENCODING)
		.value("invalid_newline", f::IOException::INVALID_NEWLINE)
		.value("unmappable_character", f::IOException::UNMAPPABLE_CHARACTER)
		.value("malformed_input", f::IOException::MALFORMED_INPUT)
//		.value("out_of_memory", f::IOException::OUT_OF_MEMORY)
		.value("huge_file", f::IOException::HUGE_FILE)
//		.value("read_only_mode", f::IOException::READ_ONLY_MODE)
		.value("unwritable_file", f::IOException::UNWRITABLE_FILE)
		.value("cannot_create_temporary_file", f::IOException::CANNOT_CREATE_TEMPORARY_FILE)
		.value("lost_disk_file", f::IOException::LOST_DISK_FILE)
		.value("platform_dependent_error", f::IOException::PLATFORM_DEPENDENT_ERROR);
*/
	py::enum_<k::fileio::TextFileDocumentInput::LockType>("FileLockMode")
		.value("no_lock", k::fileio::TextFileDocumentInput::NO_LOCK)
		.value("shared_lock", k::fileio::TextFileDocumentInput::SHARED_LOCK)
		.value("exclusive_lock", k::fileio::TextFileDocumentInput::EXCLUSIVE_LOCK);

	py::class_<a::text::Newline>("Newline", py::init<>())
		.def_readonly("line_feed", &a::text::Newline::LINE_FEED)
		.def_readonly("carriage_return", &a::text::Newline::CARRIAGE_RETURN)
		.def_readonly("carriage_return_followed_by_line_feed", &a::text::Newline::CARRIAGE_RETURN_FOLLOWED_BY_LINE_FEED)
		.def_readonly("next_line", &a::text::Newline::NEXT_LINE)
		.def_readonly("line_separator", &a::text::Newline::LINE_SEPARATOR)
		.def_readonly("paragraph_separator", &a::text::Newline::PARAGRAPH_SEPARATOR)
		.def_readonly("use_intrinsic_value", &a::text::Newline::USE_INTRINSIC_VALUE)
		.def_readonly("use_document_input", &a::text::Newline::USE_DOCUMENT_INPUT);

	py::enum_<k::fileio::UnexpectedFileTimeStampDirector::Context>("UnexpectedFileTimeStampContext")
		.value("first_modification", k::fileio::UnexpectedFileTimeStampDirector::FIRST_MODIFICATION)
		.value("overwrite_file", k::fileio::UnexpectedFileTimeStampDirector::OVERWRITE_FILE)
		.value("client_invocation", k::fileio::UnexpectedFileTimeStampDirector::CLIENT_INVOCATION);

	py::class_<a::Direction>("Direction", py::no_init)
//		.def("__getattr__", &attrOfDirection)/*
		.def_readonly("forward", &a::Direction::FORWARD)
		.def_readonly("backward", &a::Direction::BACKWARD);

	py::class_<k::Position>("Position", py::init<>())
		.def(py::init<a::Index, a::Index>())
		.def_readwrite("line", &k::Position::line)
		.def_readwrite("offset_in_line", &k::Position::offsetInLine);

	py::class_<k::Region>("Region", py::init<>())
		.def(py::init<const k::Position&, const k::Position&>())
		.def(py::init<const k::Position&>())
		.def_readwrite("first", &k::Region::first)
		.def_readwrite("second", &k::Region::second)
		.def<k::Position& (k::Region::*)(void)>("beginning", &k::Region::beginning, py::return_value_policy<py::reference_existing_object>())
		.def("encompasses", &k::Region::encompasses)
		.def<k::Position& (k::Region::*)(void)>("end", &k::Region::end, py::return_value_policy<py::reference_existing_object>())
		.def("includes", &k::Region::includes)
		.def("intersection", &k::Region::getIntersection, py::return_value_policy<py::return_by_value>())
		.def("intersects_with", &k::Region::intersectsWith)
		.def("is_empty", &k::Region::isEmpty)
		.def("is_normalized", &k::Region::isNormalized)
		.def("normalize", &k::Region::normalize, py::return_value_policy<py::reference_existing_object>())
		.def("union", &k::Region::getUnion, py::return_value_policy<py::return_by_value>());

	py::class_<k::Bookmarker, boost::noncopyable>("_Bookmarker", py::no_init)
		.def("__len__", &k::Bookmarker::numberOfMarks)
		.def("clear", &k::Bookmarker::clear)
		.def("is_marked", &k::Bookmarker::isMarked)
		.def("mark", &k::Bookmarker::mark, (py::arg("line"), py::arg("set") = true))
		.def("next", &k::Bookmarker::next, (py::arg("from"), py::arg("direction"), py::arg("wrap_around") = true, py::arg("marks") = 1))
		.def("toggle", &k::Bookmarker::toggle);

	py::class_<Buffer, boost::noncopyable>("_Buffer", py::no_init)
		.add_property("accessible_region", &Buffer::accessibleRegion)
		.add_property("bookmarker", py::make_function<
			k::Bookmarker& (Buffer::*)(void), py::return_value_policy<py::reference_existing_object>
			>(&Buffer::bookmarker, py::return_value_policy<py::reference_existing_object>()))
		.add_property("encoding",
			[](const Buffer& buffer) {return buffer.textFile().encoding();},
			[](Buffer& buffer, const string& encoding) {buffer.textFile().setEncoding(encoding);})
		.add_property("location", [](const Buffer& buffer) {return buffer.textFile().location();})
		.add_property("name", &Buffer::name)
		.add_property("newline", 
			[](const Buffer& buffer) {return buffer.textFile().newline();},
			[](Buffer& buffer, a::text::Newline newline) {buffer.textFile().setNewline(newline);})
		.add_property("number_of_lines", &Buffer::numberOfLines)
		.add_property("number_of_redoable_changes", &Buffer::numberOfRedoableChanges)
		.add_property("number_of_undoable_changes", &Buffer::numberOfUndoableChanges)
		.add_property("read_only", &Buffer::isReadOnly, &Buffer::setReadOnly)
		.add_property("records_changes", &Buffer::isRecordingChanges, &Buffer::recordChanges)
		.add_property("region", &Buffer::region)
		.add_property("revision_number", &Buffer::revisionNumber)
		.add_property("unicode_byte_order_mark", [](const Buffer& buffer) {return buffer.textFile().unicodeByteOrderMark();})
		.def("begin_compound_change", &Buffer::beginCompoundChange)
		.def("bind_file", [](Buffer& buffer, const wstring& fileName) {buffer.textFile().bind(fileName);})
		.def("clear_undo_buffer", &Buffer::clearUndoBuffer)
		.def("close", [](Buffer& buffer) {BufferList::instance().close(buffer);})
		.def("end_compound_change", &Buffer::endCompoundChange)
		.def<void (*)(k::Document&, const k::Region&)>("erase", &k::erase)
		.def("insert", [](Buffer& buffer, const k::Position& at, const a::String& text) -> k::Position {
			k::Position temp;
			k::insert(buffer, at, text, &temp);
			return temp;
		})
		.def("insert_file_contents", &k::fileio::insertFileContents)
		.def("insert_undo_boundary", &Buffer::insertUndoBoundary)
		.def("is_bound_to_file", [](const Buffer& buffer) {return buffer.textFile().isBoundToFile();})
		.def("is_compound_changing", &Buffer::isCompoundChanging)
		.def("is_modified", &Buffer::isModified)
		.def("is_narrowed", &Buffer::isNarrowed)
		.def("is_selected", [](const Buffer& buffer) {return &buffer == &EditorWindows::instance().selectedBuffer();})
		.def("length", &Buffer::length, py::arg("newline") = a::text::Newline::USE_INTRINSIC_VALUE)
		.def("line", &Buffer::line, py::return_value_policy<py::copy_const_reference>())
		.def("lock_file", [](Buffer& buffer, k::fileio::TextFileDocumentInput::LockType type, bool onlyAsEditing) {
			k::fileio::TextFileDocumentInput::LockMode mode;
			mode.type = type;
			mode.onlyAsEditing = onlyAsEditing;
			buffer.textFile().lockFile(mode);
		})
		.def("mark_unmodified", &Buffer::markUnmodified)
		.def("narrow_to_region", &Buffer::narrowToRegion)
		.def("redo", &Buffer::redo, py::arg("n") = 1)
		.def("replace", [](Buffer& buffer, const k::Region& region, const a::String& text) -> k::Position {
			k::Position temp;
			buffer.replace(region, text, &temp);
			return temp;
		})
		.def("reset_content", &Buffer::resetContent)
		.def("revert_to_file",
			[](Buffer& buffer, const string& encoding, a::encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy) {
				buffer.textFile().revert(encoding, encodingSubstitutionPolicy);
			},
			(py::arg("encoding") = string(), py::arg("encoding_substitution_policy") = a::encoding::Encoder::DONT_SUBSTITUTE))
		.def("unbind_file", [](Buffer& buffer) {buffer.textFile().unbind();})
		.def("unlock_file", [](Buffer& buffer) {buffer.textFile().unlockFile();})
		.def("save", &saveBuffer,
			(py::arg("encoding") = string(), py::arg("newlines") = a::text::Newline::USE_INTRINSIC_VALUE,
			py::arg("encoding_substitution_policy") = a::encoding::Encoder::DONT_SUBSTITUTE,
			py::arg("write_unicode_byte_order_mark") = false))
		.def("set_partitioner", [](Buffer& buffer, DocumentPartitionerProxy* partitioner) {
			(partitioner != nullptr) ? partitioner->set(buffer) : buffer.setPartitioner(unique_ptr<k::DocumentPartitioner>());})
		.def("set_modified", &Buffer::setModified)
		.def("undo", &Buffer::undo, py::arg("n") = 1)
		.def("widen", &Buffer::widen)
		.def("write_region",
			[](const Buffer& buffer, const k::Region& region, const wstring& fileName,
					bool append, const string& encoding, a::text::Newline newlines,
					a::encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy, bool writeUnicodeByteOrderMark) {
				k::fileio::WritingFormat format;
				format.encoding = encoding;
				format.newline = newlines;
				format.encodingSubstitutionPolicy = encodingSubstitutionPolicy;
				format.unicodeByteOrderMark = writeUnicodeByteOrderMark;
				k::fileio::writeRegion(buffer, region, fileName, format, append);
			},
			(py::arg("region"), py::arg("filename"), py::arg("append") = false,
			py::arg("encoding") = string(), py::arg("newlines") = a::text::Newline::USE_INTRINSIC_VALUE,
			py::arg("encoding_substitution_policy") = a::encoding::Encoder::DONT_SUBSTITUTE,
			py::arg("write_unicode_byte_order_mark") = false));
	py::class_<k::DocumentPartitioner, boost::noncopyable>("_DocumentPartitioner", py::no_init);

	interpreter.installException("UnmappableCharacterError", py::object(py::borrowed(PyExc_IOError)));
	interpreter.installException("MalformedInputError", py::object(py::borrowed(PyExc_IOError)));

	py::register_exception_translator<k::fileio::UnmappableCharacterException>(
		CppStdExceptionTranslator<k::fileio::UnmappableCharacterException>(interpreter.exceptionClass("UnmappableCharacterError")));
	py::register_exception_translator<a::text::MalformedInputException<const uint8_t*>>(
		CppStdExceptionTranslator<a::text::MalformedInputException<const uint8_t*>>(interpreter.exceptionClass("MalformedInputError")));
	py::register_exception_translator<k::fileio::IOException>([](const k::fileio::IOException& e) {::PyErr_SetFromWindowsErr(e.code().value());});
/*
	py::scope ruleModule(interpreter.module("rules"));
	py::class_<a::rules::TransitionRule, boost::noncopyable>("_TransitionRule", py::no_init)
		.add_property("content_type", &a::rules::TransitionRule::contentType)
		.add_property("destination", &a::rules::TransitionRule::destination);
	py::class_<a::rules::LiteralTransitionRule, py::bases<a::rules::TransitionRule> >(
		"LiteralTransitionRule", py::init<k::ContentType, k::ContentType, wstring, a::CodePoint, bool>((
			py::arg("content_type"), py::arg("destination"), py::arg("pattern"),
			py::arg("escape_character") = static_cast<long>(a::NONCHARACTER), py::arg("case_sensitive") = true)));
	py::class_<RegexTransitionRuleAdapter, py::bases<a::rules::TransitionRule> >(
		"RegexTransitionRule", py::init<k::ContentType, k::ContentType, wstring, bool>((
			py::arg("content_type"), py::arg("destination"), py::arg("pattern"), py::arg("case_sensitive") = true)));
	py::class_<DocumentPartitionerProxy, boost::noncopyable>("_DocumentPartitioner", py::no_init);
	py::class_<LexicalPartitionerProxy, py::bases<DocumentPartitionerProxy>,
		boost::noncopyable>("LexicalPartitioner", py::init<py::object>());
*/ALPHA_EXPOSE_EPILOGUE()
