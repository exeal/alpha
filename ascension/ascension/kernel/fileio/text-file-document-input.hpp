/**
 * @file text-file-document-input.hpp
 * Defines @c TextFileDocumentInput class.
 * @author exeal
 * @date 2009 separated from document.hpp
 * @date 2016-09-21 Separated from fileio.hpp.
 */

#ifndef ASCENSION_TEXT_FILE_DOCUMENT_INPUT_HPP
#define ASCENSION_TEXT_FILE_DOCUMENT_INPUT_HPP
#include <ascension/config.hpp>				// ASCENSION_NO_GREP
#include <ascension/platforms.hpp>
#include <ascension/kernel/document.hpp>
#include <ascension/kernel/document-input.hpp>
#include <ascension/corelib/encoder.hpp>	// encoding.Encoder.*
#include <boost/filesystem/path.hpp>

namespace ascension {
	namespace kernel {
		/// Provides features about file-bound document.
		namespace fileio {
			/// Used by functions and methods write to files. 
			struct WritingFormat {
				/// The the encoding name.
				std::string encoding;
				/// The newline.
				text::Newline newline;
				/// The substituion policy of encoding.
				encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy;
				/// Set @c true to write a UTF byte order signature. This member is ignored if the encoding was not
				/// Unicode.
				bool unicodeByteOrderMark;
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
				std::string encoding() const BOOST_NOEXCEPT override;
				static_assert(std::is_same<DocumentInput::LocationType, boost::filesystem::path::string_type>::value, "");
				DocumentInput::LocationType location() const override BOOST_NOEXCEPT;
				text::Newline newline() const override BOOST_NOEXCEPT;
			private:
				void documentModificationSignChanged(const Document& document);
				bool verifyTimeStamp(bool internal, std::time_t& newTimeStamp) BOOST_NOEXCEPT;
				// DocumentInput
				bool isChangeable(const Document& document) const override BOOST_NOEXCEPT;
				void postFirstDocumentChange(const Document& document) override BOOST_NOEXCEPT;
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

			/// @defgroup file_pathname Free Functions Related to File Path Name
			/// @{
			boost::filesystem::path canonicalizePathName(const boost::filesystem::path& pathName);
			/// @}

			/// @defgroup Free Functions Related to Document and File Path Name
			/// @{
			std::tuple<std::string, bool, Position> insertFileContents(
				Document& document, const Position& at, const boost::filesystem::path& fileName,
				const std::string& encoding, encoding::Encoder::SubstitutionPolicy encodingSubstitutionPolicy);
			void writeRegion(const Document& document, const Region& region,
				const boost::filesystem::path& fileName, const WritingFormat& format, bool append = false);
			/// @}

			/// Returns the document.
			inline const Document& TextFileDocumentInput::document() const BOOST_NOEXCEPT {
				return document_;
			}

			/// @see DocumentInput#encoding, #setEncoding
			inline std::string TextFileDocumentInput::encoding() const BOOST_NOEXCEPT {
				return encoding_;
			}

			/// Returns the file full name or an empty string if the document is not bound to any of the files.
			inline boost::filesystem::path TextFileDocumentInput::fileName() const BOOST_NOEXCEPT {
				return fileName_;
			}

			/// Returns true if the document is bound to any file.
			inline bool TextFileDocumentInput::isBoundToFile() const BOOST_NOEXCEPT {
				return !fileName_.empty();
			}

			/// @see DocumentInput#newline, #setNewline
			inline text::Newline TextFileDocumentInput::newline() const BOOST_NOEXCEPT {
				return newline_;
			}

			/// Returns true if the last opened input file contained Unicode byte order mark, or wrote BOM into
			/// the last output file.
			inline bool TextFileDocumentInput::unicodeByteOrderMark() const BOOST_NOEXCEPT {
				return unicodeByteOrderMark_;
			}
		}
	}
}	// namespace ascension.kernel.fileio

#endif // !ASCENSION_TEXT_FILE_DOCUMENT_INPUT_HPP
