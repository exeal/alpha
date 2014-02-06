/**
 * @file fileio.hpp
 * Defines @c ascension#kernel#fileio namespace.
 * @author exeal
 * @date 2009 separated from document.hpp
 * @date 2009-2014
 */

#ifndef ASCENSION_FILEIO_HPP
#define ASCENSION_FILEIO_HPP

#include <ascension/config.hpp>				// ASCENSION_NO_GREP
#include <ascension/platforms.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/corelib/encoder.hpp>	// encoding.Encoder.*
#if defined(ASCENSION_OS_WINDOWS)
#	include <ascension/win32/windows.hpp>
#elif defined(ASCENSION_OS_POSIX)
#	include <dirent.h>						// DIR
#endif
#include <boost/filesystem/path.hpp>
#include <boost/range/const_iterator.hpp>
#include <array>
#if !defined(ASCENSION_NO_GREP) && defined(ASCENSION_ABANDONED_AT_VERSION_08)
#	include <stack>
#endif

namespace ascension {
	namespace kernel {

		/// Provides features about file-bound document.
		namespace fileio {
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			/**
			 * Character type for file names. This is equivalent to
			 * @c ASCENSION_FILE_NAME_CHARACTER_TYPE configuration symbol.
			 */
			typedef ASCENSION_FILE_NAME_CHARACTER_TYPE PathCharacter;
			/// String type for file names.
			typedef std::basic_string<PathCharacter> PathString;
			/// String reference type for file names.
			typedef boost::basic_string_ref<PathCharacter, std::char_traits<PathCharacter>> PathStringPiece;
#endif

			/// Used by functions and methods write to files. 
			struct WritingFormat {
				/// The the encoding name.
				std::string encoding;
				/// The newline.
				text::Newline newline;
				/// The substituion policy of encoding.
				encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy;
				/// Set @c true to write a UTF byte order signature. This member is ignored if the
				/// encoding was not Unicode.
				bool unicodeByteOrderMark;
			};

			/**
			 * Defines the type of objects thrown as exceptions to report I/O errors from functions and methods
			 * described in @c fileio namespace.
			 * @note @c fileio does not use @c boost#filesystem#filesystem_error class.
			 */
			class IOException : public std::ios_base::failure {
			public:
				explicit IOException(const boost::filesystem::path& fileName);
				IOException(const boost::filesystem::path& fileName, const std::error_code::value_type code);
				~IOException() BOOST_NOEXCEPT;
				const boost::filesystem::path& fileName() const BOOST_NOEXCEPT;
			public:
				static bool isFileNotFound(const IOException& e);
				static bool isPermissionDenied(const IOException& e);
			private:
				const boost::filesystem::path fileName_;
			};

			/**
			 * The encoding failed for unmappable character.
			 * @see encoding#Encoder#UNMAPPABLE_CHARACTER, text#MalformedInputException
			 */
			class UnmappableCharacterException : public std::ios_base::failure {
			public:
				UnmappableCharacterException();
			};

			class TextFileDocumentInput;

			/**
			 * Interface for objects which are interested in getting informed about changes of
			 * @c TextFileDocumentInput.
			 * @see TextFileDocumentInput#addListener, TextFileDocumentInput#removeListener
			 */
			class FilePropertyListener {
			private:
				/// The encoding or newline of the bound file was changed.
				virtual void fileEncodingChanged(const TextFileDocumentInput& textFile) = 0;
				/// The the name of the bound file was changed.
				virtual void fileNameChanged(const TextFileDocumentInput& textFile) = 0;
				friend class TextFileDocumentInput;
			};

			/// Interface for objects which should handle the unexpected time stamp of the file.
			class UnexpectedFileTimeStampDirector {
			public:
				/// Context.
				enum Context {
					FIRST_MODIFICATION,	///< The call is for the first modification of the document.
					OVERWRITE_FILE,		///< The call is for overwriting the file.
					CLIENT_INVOCATION	///< The call was invoked by @c Document#checkTimeStamp.
				};
			private:
				/**
				 * Handles.
				 * @param document the document
				 * @param context the context
				 * @retval true	the process will be continued and the internal time stamp will be updated
				 * @retval false the process will be aborted
				 */
				virtual bool queryAboutUnexpectedDocumentFileTimeStamp(Document& document, Context context) BOOST_NOEXCEPT = 0;
				friend class TextFileDocumentInput;
			};
#if 0
			/// Interface for objects which are interested in getting informed about progression of file IO.
			class FileIOProgressMonitor {
			public:
				enum ProcessType {};
			private:
				/**
				 * This method will be called when 
				 * @param type the type of the precess
				 * @param processedAmount the amount of the data had processed
				 * @param totalAmount the total amount of the data to process
				 */
				virtual void onProgress(ProcessType type, ULONGLONG processedAmount, ULONGLONG totalAmount) = 0;
				/// Returns the internal number of lines.
				virtual Index queryIntervalLineCount() const = 0;
				/// Releases the object.
				virtual void release() = 0;
				friend class Document;
			};
#endif
			/**
			 * @c std#basic_streambuf implementation of the text file with encoding conversion.
			 * @note This class is not intended to be subclassed.
			 */
			class TextFileStreamBuffer : public std::basic_streambuf<Char>, private boost::noncopyable {
			public:
				TextFileStreamBuffer(const boost::filesystem::path& fileName,
					std::ios_base::openmode mode, const std::string& encoding,
					encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					bool writeUnicodeByteOrderMark);
				~TextFileStreamBuffer();
				TextFileStreamBuffer* close();
				TextFileStreamBuffer* closeAndDiscard();
				std::string encoding() const BOOST_NOEXCEPT;
				const boost::filesystem::path& fileName() const BOOST_NOEXCEPT;
				bool isOpen() const BOOST_NOEXCEPT;
				std::ios_base::openmode mode() const BOOST_NOEXCEPT;
				bool unicodeByteOrderMark() const BOOST_NOEXCEPT;
			private:
				void buildEncoder(const std::string& encoding, bool detectEncoding);
				void buildInputMapping();
				TextFileStreamBuffer* closeFile() BOOST_NOEXCEPT;
				void openForReading(const std::string& encoding);
				void openForWriting(const std::string& encoding, bool writeUnicodeByteOrderMark);
				// std.basic_streambuf
				int_type overflow(int_type c /* = traits_type::eof() */);
				int_type pbackfail(int_type c /* = traits_type::eof() */);
				int sync();
				int_type underflow();
			private:
				typedef std::basic_streambuf<Char> Base;
#ifdef ASCENSION_OS_WINDOWS
				HANDLE fileHandle_, fileMapping_;
#else // ASCENSION_OS_POSIX
				int fileDescriptor_;
#endif
				const boost::filesystem::path fileName_;
				std::ios_base::openmode mode_;
				struct InputMapping {
					boost::iterator_range<const Byte*> buffer;
					const Byte* current;
					InputMapping() BOOST_NOEXCEPT : buffer(nullptr, nullptr), current(nullptr) {}
				} inputMapping_;
#ifdef ASCENSION_OS_WINDOWS
				LARGE_INTEGER originalFileEnd_;
#else // ASCENSION_OS_POSIX
				off_t originalFileEnd_;
#endif
				std::unique_ptr<encoding::Encoder> encoder_;
				std::array<Char, 8192> ucsBuffer_;
			};

			class TextFileDocumentInput : public DocumentInput, private boost::noncopyable {
			public:
				/// Lock types for opened file.
				enum LockType {
					NO_LOCK,		///< Does not lock or unlock.
					SHARED_LOCK,	///< Uses shared lock.
					EXCLUSIVE_LOCK	///< Uses exclusive lock.
				};
				/// Lock mode for opened file.
				struct LockMode {
					LockType type;		///< The type of the lock.
					bool onlyAsEditing;	///< @c true if the lock will not be performed unless modification occurs.
				};
				/// Option for @c TextFileDocumentInput#write method.
				struct WritingOption {
					bool createBackup;	///< Set @c true to creates backup files (not implemented).
				};
			public:
				explicit TextFileDocumentInput(Document& document);
				~TextFileDocumentInput() BOOST_NOEXCEPT;
				bool checkTimeStamp();
				const Document& document() const BOOST_NOEXCEPT;

				/// @name Listeners
				/// @{
				void addListener(FilePropertyListener& listener);
				void removeListener(FilePropertyListener& listener);
				/// @}

				/// @name Bound File
				/// @{
				void bind(const boost::filesystem::path& fileName);
				boost::filesystem::path fileName() const BOOST_NOEXCEPT;
				bool isBoundToFile() const BOOST_NOEXCEPT;
				void lockFile(const LockMode& mode);
				LockType lockType() const BOOST_NOEXCEPT;
				void revert(const std::string& encoding,
					encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					UnexpectedFileTimeStampDirector* unexpectedTimeStampDirector = nullptr);
				void unbind() BOOST_NOEXCEPT;
				void unlockFile();
				void write(const WritingFormat& format, const WritingOption* options = nullptr);
				/// @}

				/// @name Encodings
				/// @{
				TextFileDocumentInput& setEncoding(const std::string& encoding);
				TextFileDocumentInput& setNewline(const text::Newline& newline);
				bool unicodeByteOrderMark() const BOOST_NOEXCEPT;
				/// @}

				// DocumentInput
				std::string encoding() const BOOST_NOEXCEPT;
				static_assert(std::is_same<DocumentInput::LocationType, boost::filesystem::path::string_type>::value, "");
				DocumentInput::LocationType location() const BOOST_NOEXCEPT;
				text::Newline newline() const BOOST_NOEXCEPT;
			private:
				void documentModificationSignChanged(const Document& document);
				bool verifyTimeStamp(bool internal, std::time_t& newTimeStamp) BOOST_NOEXCEPT;
				// DocumentInput
				bool isChangeable(const Document& document) const BOOST_NOEXCEPT;
				void postFirstDocumentChange(const Document& document) BOOST_NOEXCEPT;
			private:
				std::shared_ptr<TextFileDocumentInput> weakSelf_;	// for Document.setInput call
				class FileLocker;
				std::unique_ptr<FileLocker> fileLocker_;
				Document& document_;
				boost::signals2::scoped_connection documentModificationSignChangedConnection_;
				boost::filesystem::path fileName_;
				std::string encoding_;
				bool unicodeByteOrderMark_;
				text::Newline newline_;
				std::size_t savedDocumentRevision_;
				boost::optional<std::time_t> userLastWriteTime_, internalLastWriteTime_;
				LockMode desiredLockMode_;
				ascension::detail::Listeners<FilePropertyListener> listeners_;
				UnexpectedFileTimeStampDirector* timeStampDirector_;
			};

#if !defined(ASCENSION_NO_GREP) && defined(ASCENSION_ABANDONED_AT_VERSION_08)
			class DirectoryIteratorBase : private boost::noncopyable {
			public:
				virtual ~DirectoryIteratorBase() BOOST_NOEXCEPT;
				/**
				 * Returns the current entry name.
				 * @throw NoSuchElementException the iteration has already ended
				 */
				virtual const PathString& current() const = 0;
				/**
				 * Returns the directory name this iterator traverses. This value does not end
				 * with path-separator.
				 */
				virtual const PathString& directory() const BOOST_NOEXCEPT = 0;
				/**
				 * Returns @c false if the iterator is end.
				 */
				virtual bool hasNext() const BOOST_NOEXCEPT = 0;
				/**
				 * Returns @c true if the current entry is directory.
				 * @throw NoSuchElementException the iteration has already ended
				 */
				virtual bool isDirectory() const = 0;
				/**
				 * Moves to the next entry. If the iterator has already reached at the end, does
				 * nothing.
				 * @throw IOException any I/O error occurred
				 */
				virtual void next() = 0;
			protected:
				DirectoryIteratorBase() BOOST_NOEXCEPT;
			};

			/// Traverses entries in the specified directory.
			class DirectoryIterator : public DirectoryIteratorBase {
			public:
				// constructors
				DirectoryIterator(const PathStringPiece& directoryName);
				~DirectoryIterator() BOOST_NOEXCEPT;
				// DirectoryIteratorBase
				const PathString& current() const;
				const PathString& directory() const BOOST_NOEXCEPT;
				bool hasNext() const BOOST_NOEXCEPT;
				bool isDirectory() const;
				void next();
			private:
				void update(const void* info);
			private:
#ifdef ASCENSION_OS_WINDOWS
				HANDLE handle_;
#else // ASCENSION_OS_POSIX
				DIR* handle_;
#endif
				PathString current_, directory_;
				bool currentIsDirectory_, done_;
			};

			/// Recursive version of @c DirectoryIterator.
			class RecursiveDirectoryIterator : public DirectoryIteratorBase {
			public:
				// constructors
				RecursiveDirectoryIterator(const PathStringPiece& directoryName);
				~RecursiveDirectoryIterator() BOOST_NOEXCEPT;
				// attributes
				void dontPush();
				std::size_t level() const BOOST_NOEXCEPT;
				void pop();
				// DirectoryIteratorBase
				const PathString& current() const;
				const PathString& directory() const BOOST_NOEXCEPT;
				bool hasNext() const BOOST_NOEXCEPT;
				bool isDirectory() const;
				void next();
			private:
				std::stack<DirectoryIterator*> stack_;
				PathString directory_;
				bool doesntPushNext_;
			};
#endif // !ASCENSION_NO_GREP

			/// @defgroup file_pathname Free Functions Related to File Path Name
			/// @{
			boost::filesystem::path canonicalizePathName(const boost::filesystem::path& pathName);
#ifdef ASCENSION_ABANDONED_AT_VERSION_08
			bool comparePathNames(
				const boost::basic_string_ref<boost::filesystem::path::value_type>& s1,
				const boost::basic_string_ref<boost::filesystem::path::value_type>& s2);
#endif // ASCENSION_ABANDONED_AT_VERSION_08
			/// @}

			/// @defgroup Free Functions Related to Document and File Path Name
			/// @{
			std::pair<std::string, bool> insertFileContents(Document& document,
				const Position& at, const boost::filesystem::path& fileName, const std::string& encoding,
				encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy, Position* endOfInsertedString = nullptr);
			void writeRegion(const Document& document, const Region& region,
				const boost::filesystem::path& fileName, const WritingFormat& format, bool append = false);
			/// @}

			/// Returns the file name.
			inline const boost::filesystem::path& TextFileStreamBuffer::fileName() const BOOST_NOEXCEPT {return fileName_;}

			/// Returns the open mode.
			inline std::ios_base::openmode TextFileStreamBuffer::mode() const BOOST_NOEXCEPT {return mode_;}

			/// Returns the document.
			inline const Document& TextFileDocumentInput::document() const BOOST_NOEXCEPT {return document_;}

			/// @see DocumentInput#encoding, #setEncoding
			inline std::string TextFileDocumentInput::encoding() const BOOST_NOEXCEPT {return encoding_;}

			/// Returns the file full name or an empty string if the document is not bound to any of the files.
			inline boost::filesystem::path TextFileDocumentInput::fileName() const BOOST_NOEXCEPT {return fileName_;}

			/// Returns true if the document is bound to any file.
			inline bool TextFileDocumentInput::isBoundToFile() const BOOST_NOEXCEPT {return !fileName_.empty();}

			/// @see DocumentInput#newline, #setNewline
			inline text::Newline TextFileDocumentInput::newline() const BOOST_NOEXCEPT {return newline_;}

			/// Returns true if the last opened input file contained Unicode byte order mark, or wrote BOM into
			/// the last output file.
			inline bool TextFileDocumentInput::unicodeByteOrderMark() const BOOST_NOEXCEPT {return unicodeByteOrderMark_;}
		}	// namespace fileio
	}	// namespace kernel
}	// namespace ascension

#endif // !ASCENSION_FILEIO_HPP
