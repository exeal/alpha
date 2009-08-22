/**
 * @file fileio.hpp
 * Defines @c ascension#kernel#fileio namespace.
 * @author exeal
 * @date 2009 separated from document.hpp
 */

#ifndef ASCENSION_FILEIO_HPP
#define ASCENSION_FILEIO_HPP
#include <ascension/document.hpp>
#ifdef ASCENSION_POSIX
#include <sys/types.h>
#endif // ASCENSION_POSIX

namespace ascension {
	namespace kernel {

		/// Provides features about file-bound document.
		namespace fileio {

			/**
			 * Character type for file names. This is equivalent to
			 * @c ASCENSION_FILE_NAME_CHARACTER_TYPE configuration symbol.
			 */
			typedef ASCENSION_FILE_NAME_CHARACTER_TYPE Char;
			/// String type for file names.
			typedef std::basic_string<Char> String;
			/// Represents platform-dependent IO error.
			typedef PlatformDependentError<std::ios_base::failure> PlatformDependentIOError;

			/// Used by functions and methods write to files. 
			struct WritingFormat {
				/// The the encoding name.
				std::string encoding;
				/// The newline.
				Newline newline;
				/// The substituion policy of encoding.
				encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy;
				/// Set @c true to write a UTF byte order signature. This member is ignored if the
				/// encoding was not Unicode.
				bool unicodeByteOrderMark;
			};

			/// The specified file is not found.
			class FileNotFoundException : public std::ios_base::failure {
			public:
				explicit FileNotFoundException(const String& fileName);
				const String& fileName() const /*throw()*/;
			private:
				const String fileName_;
			};

			/// The access to the target entity was rejected.
			class AccessDeniedException : public std::ios_base::failure {
			public:
				AccessDeniedException();
			};

			/// The encoding failed for unmappable character.
			/// @see encoding#Encoder#UNMAPPABLE_CHARACTER
			class UnmappableCharacterException : public std::ios_base::failure {
			public:
				UnmappableCharacterException();
			};

			/// The encoding failed for malformed input.
			/// @see encoding#Encoder#MALFORMED_INPUT
			class MalformedInputException : public std::ios_base::failure {
			public:
				MalformedInputException();
			};

			/// File I/O exception.
			class IOException : public std::runtime_error {
			public:
				/// Error types.
				enum Type {
#if 0
					/// Failed for out of memory.
					OUT_OF_MEMORY,
#endif
					/// The file to be opend is too huge.
					HUGE_FILE,
#if 0
					/// Tried to write the read only document. If the disk file is read only, POSIX
					/// @c errno is set to @c BBADF, or Win32 @c GetLastError returns
					/// @c ERROR_FILE_READ_ONLY.
					READ_ONLY_MODE,
#endif
					/// The file is read only and not writable.
					UNWRITABLE_FILE,
					/// Failed to create the temporary file for writing.
					CANNOT_CREATE_TEMPORARY_FILE,
					/// Failed to write to the file and the file was <strong>lost</strong>.
					LOST_DISK_FILE
				};
			public:
				/// Constructor.
				explicit IOException(Type type) /*throw()*/ : std::runtime_error(""), type_(type) {}
				/// Returns the error type.
				Type type() const /*throw()*/ {return type_;}
			private:
				Type type_;
			};

			class TextFileDocumentInput;

			/**
			 * Interface for objects which are interested in getting informed about changes of @c FileBinder.
			 * @see FileBinder#addListener, FileBinder#removeListener
			 */
			class IFilePropertyListener {
			private:
				/// The encoding or newline of the bound file was changed.
				virtual void fileEncodingChanged(const TextFileDocumentInput& textFile) = 0;
				/// The the name of the bound file was changed.
				virtual void fileNameChanged(const TextFileDocumentInput& textFile) = 0;
				friend class TextFileDocumentInput;
			};

			/// Interface for objects which should handle the unexpected time stamp of the file.
			class IUnexpectedFileTimeStampDirector {
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
				virtual bool queryAboutUnexpectedDocumentFileTimeStamp(Document& document, Context context) /*throw()*/ = 0;
				friend class TextFileDocumentInput;
			};
#if 0
			/// Interface for objects which are interested in getting informed about progression of file IO.
			class IFileIOProgressMonitor {
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
				virtual length_t queryIntervalLineCount() const = 0;
				/// Releases the object.
				virtual void release() = 0;
				friend class Document;
			};
#endif
			/**
			 * @c std#basic_streambuf implementation of the text file with encoding conversion.
			 * @note This class is not intended to be subclassed.
			 */
			class TextFileStreamBuffer : public std::basic_streambuf<ascension::Char> {
				MANAH_NONCOPYABLE_TAG(TextFileStreamBuffer);
			public:
				TextFileStreamBuffer(const String& fileName, std::ios_base::openmode mode,
					const std::string& encoding, encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					bool writeUnicodeByteOrderMark);
				~TextFileStreamBuffer();
				TextFileStreamBuffer* close();
				TextFileStreamBuffer* closeAndDiscard();
				std::string encoding() const /*throw()*/;
				const String& fileName() const /*throw()*/;
				bool isOpen() const /*throw()*/;
				std::ios_base::openmode mode() const /*throw()*/;
				bool unicodeByteOrderMark() const /*throw()*/;
			private:
				TextFileStreamBuffer* closeFile() /*throw()*/;
				void openForReading(const std::string& encoding);
				void openForWriting(bool writeUnicodeByteOrderMark);
				// std.basic_streambuf
				int_type overflow(int_type c /* = traits_type::eof() */);
				int_type pbackfail(int_type c /* = traits_type::eof() */);
				int sync();
				int_type underflow();
			private:
				typedef std::basic_streambuf<ascension::Char> Base;
#ifdef ASCENSION_WINDOWS
				HANDLE fileHandle_, fileMapping_;
#else // ASCENSION_POSIX
				int fileDescriptor_;
#endif
				const String fileName_;
				std::ios_base::openmode mode_;
				struct {
					const byte* first;
					const byte* last;
					const byte* current;
				} inputMapping_;
#ifdef ASCENSION_WINDOWS
				LARGE_INTEGER originalFileEnd_;
#else // ASCENSION_POSIX
				off_t originalFileEnd_;
#endif
				std::auto_ptr<encoding::Encoder> encoder_;
				ascension::Char ucsBuffer_[8192];
			};

			class TextFileDocumentInput : public IDocumentInput, public IDocumentStateListener {
				MANAH_NONCOPYABLE_TAG(TextFileDocumentInput);
			public:
				/// The structure used to represent a file time.
#ifdef ASCENSION_WINDOWS
				typedef FILETIME Time;
#else // ASCENSION_POSIX
				typedef ::time_t Time;
#endif
				/// Lock types for opened file.
				enum LockType {
					DONT_LOCK,		///< Does not lock.
					SHARED_LOCK,	///< Uses shared lock.
					EXCLUSIVE_LOCK	///< Uses exclusive lock.
				};
				/// Lock mode for opened file.
				struct LockMode {
					LockType type;		///< The type of the lock.
					bool onlyAsEditing;	///< @c true if the lock will not be performed unless modification occurs.
				};
				/// Option flags for @c TextFileDocumentInput#write method.
				enum WritingOption {
					BY_COPYING		= 0x01,	///< Not implemented.
					CREATE_BACKUP	= 0x02	///< Creates backup files.
				};
			public:
				explicit TextFileDocumentInput(Document& document);
				~TextFileDocumentInput() /*throw()*/;
				bool checkTimeStamp();
				const Document& document() const /*throw()*/;
				LockType lockType() const /*throw()*/;
				// listener
				void addListener(IFilePropertyListener& listener);
				void removeListener(IFilePropertyListener& listener);
				// bound file name
				String extensionName() const /*throw()*/;
				bool isOpen() const /*throw()*/;
				String name() const /*throw()*/;
				String pathName() const /*throw()*/;
				bool rebind(const String& fileName);
				// encodings
				void setEncoding(const std::string& encoding);
				void setNewline(Newline newline);
				bool unicodeByteOrderMark() const /*throw()*/;
				// I/O
				void close();
				bool open(const String& fileName, LockMode lockMode,
					const std::string& encoding, encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					IUnexpectedFileTimeStampDirector* unexpectedTimeStampDirector = 0);
				bool write(const String& fileName, const WritingFormat& format, const manah::Flags<WritingOption>& options);
				// IDocumentInput
				std::string encoding() const /*throw()*/;
				bool isChangeable() const /*throw()*/;
				ascension::String location() const /*throw()*/;
				Newline newline() const /*throw()*/;
			private:
				bool lock() /*throw()*/;
				bool unlock() /*throw()*/;
				bool verifyTimeStamp(bool internal, Time& newTimeStamp) /*throw()*/;
				// IDocumentStateListener
				void documentAccessibleRegionChanged(const Document& document);
				void documentModificationSignChanged(const Document& document);
				void documentPropertyChanged(const Document& document, const DocumentPropertyKey& key);
				void documentReadOnlySignChanged(const Document& document);
			private:
				class FileLocker;
				FileLocker* fileLocker_;
				Document& document_;
				String fileName_;
				std::string encoding_;
				bool unicodeByteOrderMark_;
				Newline newline_;
				std::size_t savedDocumentRevision_;
				Time userLastWriteTime_, internalLastWriteTime_;
				ascension::internal::Listeners<IFilePropertyListener> listeners_;
				IUnexpectedFileTimeStampDirector* timeStampDirector_;
			};

#ifndef ASCENSION_NO_GREP
			class DirectoryIteratorBase {
				MANAH_NONCOPYABLE_TAG(DirectoryIteratorBase);
			public:
				virtual ~DirectoryIteratorBase() /*throw()*/;
				/**
				 * Returns the current entry name.
				 * @throw NoSuchElementException the iteration has already ended
				 */
				virtual const String& current() const = 0;
				/**
				 * Returns the directory name this iterator traverses. This value does not end
				 * with path-separator.
				 */
				virtual const String& directory() const /*throw()*/ = 0;
				/**
				 * Returns true if the current entry is directory.
				 * @throw NoSuchElementException the iteration has already ended
				 */
				virtual bool isDirectory() const = 0;
				/**
				 * Returns true if the iterator is end.
				 * @throw NoSuchElementException the iteration has already ended
				 */
				virtual bool isDone() const /*throw()*/ = 0;
				/**
				 * Moves to the next entry. If the iterator has already reached at the end, does
				 * nothing.
				 * @throw IOException any I/O error occurred
				 */
				virtual void next() = 0;
			protected:
				DirectoryIteratorBase() /*throw()*/;
			};

			/// Traverses entries in the specified directory.
			class DirectoryIterator : public DirectoryIteratorBase {
			public:
				// constructors
				DirectoryIterator(const Char* directoryName);
				~DirectoryIterator() /*throw()*/;
				// DirectoryIteratorBase
				const String& current() const;
				const String& directory() const /*throw()*/;
				bool isDirectory() const;
				bool isDone() const /*throw()*/;
				void next();
			private:
				void update(const void* info);
			private:
#ifdef ASCENSION_WINDOWS
				HANDLE handle_;
#else // ASCENSION_POSIX
				DIR* handle_;
#endif
				String current_, directory_;
				bool currentIsDirectory_, done_;
			};

			/// Recursive version of @c DirectoryIterator.
			class RecursiveDirectoryIterator : public DirectoryIteratorBase {
			public:
				// constructors
				RecursiveDirectoryIterator(const Char* directoryName);
				~RecursiveDirectoryIterator() /*throw()*/;
				// attributes
				void dontPush();
				std::size_t level() const /*throw()*/;
				void pop();
				// DirectoryIteratorBase
				const String& current() const;
				const String& directory() const /*throw()*/;
				bool isDirectory() const;
				bool isDone() const /*throw()*/;
				void next();
			private:
				std::stack<DirectoryIterator*> stack_;
				String directory_;
				bool doesntPushNext_;
			};
#endif // !ASCENSION_NO_GREP

			// free functions related to file path name
			String canonicalizePathName(const Char* pathName);
			bool comparePathNames(const Char* s1, const Char* s2);

			// free function
			std::pair<std::string, bool> insertFileContents(Document& document,
				const Position& at, const String& fileName, const std::string& encoding,
				encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy, Position* endOfInsertedString = 0);
			void writeRegion(const Document& document, const Region& region,
				const String& fileName, const WritingFormat& format, bool append = false);

			/// Returns the file name.
			inline const String& TextFileStreamBuffer::fileName() const /*throw()*/ {return fileName_;}

			/// Returns the open mode.
			inline std::ios_base::openmode TextFileStreamBuffer::mode() const /*throw()*/ {return mode_;}

			/// Returns the document.
			inline const Document& TextFileDocumentInput::document() const /*throw()*/ {return document_;}

			/// @see IDocumentInput#encoding, #setEncoding
			inline std::string TextFileDocumentInput::encoding() const /*throw()*/ {return encoding_;}

			/// Returns true if the document is bound to any file.
			inline bool TextFileDocumentInput::isOpen() const /*throw()*/ {return !fileName_.empty();}

			/// @see IDocumentInput#newline, #setNewline
			inline Newline TextFileDocumentInput::newline() const /*throw()*/ {return newline_;}

			/// Returns the file full name or an empty string if the document is not bound to any of the files.
			inline String TextFileDocumentInput::pathName() const /*throw()*/ {return fileName_;}

			/// Returns true if the last opened input file contained Unicode byte order mark, or wrote BOM into
			/// the last output file.
			inline bool TextFileDocumentInput::unicodeByteOrderMark() const /*throw()*/ {return unicodeByteOrderMark_;}

		}	// namespace fileio
	}	// namespace kernel
}	// namespace ascension

#endif // !ASCENSION_FILEIO_HPP
