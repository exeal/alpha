/**
 * @file fileio.hpp
 * Defines @c ascension#kernel#fileio namespace.
 * @author exeal
 * @date 2009 separated from document.hpp
 * @date 2009-2011
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
#ifndef ASCENSION_NO_GREP
#	include <stack>
#endif // !ASCENSION_NO_GREP

namespace ascension {
	namespace kernel {

		/// Provides features about file-bound document.
		namespace fileio {

			/**
			 * Character type for file names. This is equivalent to
			 * @c ASCENSION_FILE_NAME_CHARACTER_TYPE configuration symbol.
			 */
			typedef ASCENSION_FILE_NAME_CHARACTER_TYPE PathCharacter;
			/// String type for file names.
			typedef std::basic_string<PathCharacter> PathString;

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

			class IOException : public PlatformDependentError<std::ios_base::failure> {
			public:
				explicit IOException(const PathString& fileName);
				IOException(const PathString& fileName, value_type code);
				~IOException() throw();
				value_type code() const /*throw()*/;
				const PathString& fileName() const /*throw()*/;
			public:
				static bool isFileNotFound(const IOException& e);
				static bool isPermissionDenied(const IOException& e);
			private:
				const PathString fileName_;
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
				virtual bool queryAboutUnexpectedDocumentFileTimeStamp(Document& document, Context context) /*throw()*/ = 0;
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
			class TextFileStreamBuffer : public std::basic_streambuf<Char> {
				ASCENSION_NONCOPYABLE_TAG(TextFileStreamBuffer);
			public:
				TextFileStreamBuffer(const PathString& fileName,
					std::ios_base::openmode mode, const std::string& encoding,
					encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					bool writeUnicodeByteOrderMark);
				~TextFileStreamBuffer();
				TextFileStreamBuffer* close();
				TextFileStreamBuffer* closeAndDiscard();
				std::string encoding() const /*throw()*/;
				const PathString& fileName() const /*throw()*/;
				bool isOpen() const /*throw()*/;
				std::ios_base::openmode mode() const /*throw()*/;
				bool unicodeByteOrderMark() const /*throw()*/;
			private:
				void buildEncoder(const std::string& encoding, bool detectEncoding);
				void buildInputMapping();
				TextFileStreamBuffer* closeFile() /*throw()*/;
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
				const PathString fileName_;
				std::ios_base::openmode mode_;
				struct {
					const Byte* first;
					const Byte* last;
					const Byte* current;
				} inputMapping_;
#ifdef ASCENSION_OS_WINDOWS
				LARGE_INTEGER originalFileEnd_;
#else // ASCENSION_OS_POSIX
				off_t originalFileEnd_;
#endif
				std::auto_ptr<encoding::Encoder> encoder_;
				Char ucsBuffer_[8192];
			};

			class TextFileDocumentInput : public DocumentInput, public DocumentStateListener {
				ASCENSION_NONCOPYABLE_TAG(TextFileDocumentInput);
			public:
				/// The structure used to represent a file time.
#ifdef ASCENSION_OS_WINDOWS
				typedef FILETIME Time;
#else // ASCENSION_OS_POSIX
				typedef ::time_t Time;
#endif
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
				~TextFileDocumentInput() /*throw()*/;
				bool checkTimeStamp();
				const Document& document() const /*throw()*/;
				// listener
				void addListener(FilePropertyListener& listener);
				void removeListener(FilePropertyListener& listener);
				// bound file
				void bind(const PathString& fileName);
				PathString fileName() const /*throw()*/;
				bool isBoundToFile() const /*throw()*/;
				void lockFile(const LockMode& mode);
				LockType lockType() const /*throw()*/;
				void revert(const std::string& encoding,
					encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy,
					UnexpectedFileTimeStampDirector* unexpectedTimeStampDirector = 0);
				void unbind() /*throw()*/;
				void unlockFile();
				void write(const WritingFormat& format, const WritingOption* options = 0);
				// encodings
				TextFileDocumentInput& setEncoding(const std::string& encoding);
				TextFileDocumentInput& setNewline(text::Newline newline);
				bool unicodeByteOrderMark() const /*throw()*/;
				// DocumentInput
				std::string encoding() const /*throw()*/;
				String location() const /*throw()*/;
				text::Newline newline() const /*throw()*/;
			private:
				bool verifyTimeStamp(bool internal, Time& newTimeStamp) /*throw()*/;
				// DocumentInput
				bool isChangeable(const Document& document) const /*throw()*/;
				void postFirstDocumentChange(const Document& document) /*throw()*/;
				// DocumentStateListener
				void documentAccessibleRegionChanged(const Document& document);
				void documentModificationSignChanged(const Document& document);
				void documentPropertyChanged(const Document& document, const DocumentPropertyKey& key);
				void documentReadOnlySignChanged(const Document& document);
			private:
				std::shared_ptr<TextFileDocumentInput> weakSelf_;	// for Document.setInput call
				class FileLocker;
				FileLocker* fileLocker_;
				Document& document_;
				PathString fileName_;
				std::string encoding_;
				bool unicodeByteOrderMark_;
				text::Newline newline_;
				std::size_t savedDocumentRevision_;
				Time userLastWriteTime_, internalLastWriteTime_;
				LockMode desiredLockMode_;
				detail::Listeners<FilePropertyListener> listeners_;
				UnexpectedFileTimeStampDirector* timeStampDirector_;
			};

#ifndef ASCENSION_NO_GREP
			class DirectoryIteratorBase {
				ASCENSION_NONCOPYABLE_TAG(DirectoryIteratorBase);
			public:
				virtual ~DirectoryIteratorBase() /*throw()*/;
				/**
				 * Returns the current entry name.
				 * @throw NoSuchElementException the iteration has already ended
				 */
				virtual const PathString& current() const = 0;
				/**
				 * Returns the directory name this iterator traverses. This value does not end
				 * with path-separator.
				 */
				virtual const PathString& directory() const /*throw()*/ = 0;
				/**
				 * Returns @c false if the iterator is end.
				 */
				virtual bool hasNext() const /*throw()*/ = 0;
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
				DirectoryIteratorBase() /*throw()*/;
			};

			/// Traverses entries in the specified directory.
			class DirectoryIterator : public DirectoryIteratorBase {
			public:
				// constructors
				DirectoryIterator(const PathCharacter* directoryName);
				~DirectoryIterator() /*throw()*/;
				// DirectoryIteratorBase
				const PathString& current() const;
				const PathString& directory() const /*throw()*/;
				bool hasNext() const /*throw()*/;
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
				RecursiveDirectoryIterator(const PathCharacter* directoryName);
				~RecursiveDirectoryIterator() /*throw()*/;
				// attributes
				void dontPush();
				std::size_t level() const /*throw()*/;
				void pop();
				// DirectoryIteratorBase
				const PathString& current() const;
				const PathString& directory() const /*throw()*/;
				bool hasNext() const /*throw()*/;
				bool isDirectory() const;
				void next();
			private:
				std::stack<DirectoryIterator*> stack_;
				PathString directory_;
				bool doesntPushNext_;
			};
#endif // !ASCENSION_NO_GREP

			// free functions related to file path name
			PathString canonicalizePathName(const PathCharacter* pathName);
			bool comparePathNames(const PathCharacter* s1, const PathCharacter* s2);

			// free function
			std::pair<std::string, bool> insertFileContents(Document& document,
				const Position& at, const PathString& fileName, const std::string& encoding,
				encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy, Position* endOfInsertedString = 0);
			void writeRegion(const Document& document, const Region& region,
				const PathString& fileName, const WritingFormat& format, bool append = false);

			/// Returns the file name.
			inline const PathString& TextFileStreamBuffer::fileName() const /*throw()*/ {return fileName_;}

			/// Returns the open mode.
			inline std::ios_base::openmode TextFileStreamBuffer::mode() const /*throw()*/ {return mode_;}

			/// Returns the document.
			inline const Document& TextFileDocumentInput::document() const /*throw()*/ {return document_;}

			/// @see DocumentInput#encoding, #setEncoding
			inline std::string TextFileDocumentInput::encoding() const /*throw()*/ {return encoding_;}

			/// Returns the file full name or an empty string if the document is not bound to any of the files.
			inline PathString TextFileDocumentInput::fileName() const /*throw()*/ {return fileName_;}

			/// Returns true if the document is bound to any file.
			inline bool TextFileDocumentInput::isBoundToFile() const /*throw()*/ {return !fileName_.empty();}

			/// @see DocumentInput#newline, #setNewline
			inline text::Newline TextFileDocumentInput::newline() const /*throw()*/ {return newline_;}

			/// Returns true if the last opened input file contained Unicode byte order mark, or wrote BOM into
			/// the last output file.
			inline bool TextFileDocumentInput::unicodeByteOrderMark() const /*throw()*/ {return unicodeByteOrderMark_;}

		}	// namespace fileio
	}	// namespace kernel
}	// namespace ascension

#endif // !ASCENSION_FILEIO_HPP
