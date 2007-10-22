// file.hpp
// (c) 2003-2007 exeal

#ifndef MANAH_FILE_HPP
#define MANAH_FILE_HPP
#include "windows.hpp"

namespace manah {
namespace win32 {
namespace io {

template<bool autoClose> class KernelHandle {
	MANAH_NONCOPYABLE_TAG(KernelHandle);
public:
	explicit KernelHandle(::HANDLE handle = 0) throw() : handle_(handle) {}
	virtual ~KernelHandle() throw();
	virtual bool close() throw() {if(toBoolean(::CloseHandle(handle_))) {handle_ = 0; return true;} return false;}
	::HANDLE get() const throw() {return handle_;}
protected:
	void setHandle(::HANDLE newHandle) throw();
private:
	::HANDLE handle_;
};

template<bool autoClose> inline KernelHandle<autoClose>::~KernelHandle() throw() {close();}
template<> inline KernelHandle<false>::~KernelHandle() throw() {}
template<bool autoClose> inline void KernelHandle<autoClose>::setHandle(::HANDLE newHandle) throw() {close(); handle_ = newHandle;}
template<> inline void KernelHandle<false>::setHandle(::HANDLE newHandle) throw() {handle_ = newHandle;}

class FileException : public std::runtime_error {
public:
	explicit FileException(const std::string& what_arg) : std::runtime_error(what_arg) {}
};

struct FileConstants {
	enum PointerMovementMode {FROM_BEGIN = FILE_BEGIN, FROM_CURRENT = FILE_CURRENT, FROM_END = FILE_END};
};

template<bool noThrow> class File : public KernelHandle<false>, public FileConstants {
public:
	// constructors
	explicit File(::HANDLE handle = 0) throw();
	File(const ::WCHAR* fileName, ::DWORD desiredAccess, ::DWORD shareMode, ::DWORD creationDisposition,
		::DWORD flagsAndAttributes, ::SECURITY_ATTRIBUTES* securityAttributes = 0, ::HANDLE templateFile = 0);
	virtual ~File();
	// operators (T must be primitive)
	template<typename T> File&	operator<<(const T& buffer) {write<T>(buffer); return *this;}
	template<typename T> File&	operator>>(T& buffer) {read<T>(buffer); return *this;}
	// constructions
	virtual void				abort();
	virtual std::auto_ptr<File>	duplicate() const;
	virtual bool				open(const ::WCHAR* fileName, ::DWORD desiredAccess, ::DWORD shareMode, ::DWORD creationDisposition,
									::DWORD flagsAndAttributes, ::SECURITY_ATTRIBUTES* securityAttributes = 0, ::HANDLE templateFile = 0);
	virtual bool				close();
	// I/O (T must be primitive)
	virtual bool	read(void* buffer, DWORD bytes, DWORD* readBytes = 0);
	template<typename T>
	bool			read(T& buffer) {
		char buf[sizeof(T)];
		if(read(buf, sizeof(T))) {
			std::uninitialized_copy(buf, buf + 1, buffer);
			return true;
		}
		return false;
	}
	virtual bool	write(const void* lpBuffer, ::DWORD cb, ::DWORD* pcbWritten = 0);
	template<typename T>
	bool			write(const T& buffer) {return write(&buffer, sizeof(T));}
	virtual bool	flush();
	// cursor
	virtual ::ULONGLONG	getLength() const;
	virtual ::ULONGLONG	seek(::LONGLONG offset, PointerMovementMode mode);
	void				seekToBegin();
	::ULONGLONG			seekToEnd();
	virtual void		setLength(::ULONGLONG newLength);
	// locking
	virtual bool	lockRange(::DWORD pos, ::DWORD count);
	virtual bool	unlockRange(::DWORD pos, ::DWORD count);
	// state
	virtual ::DWORD	getPosition() const;
	virtual bool	getFileTime(::LPFILETIME creationTime,
						::LPFILETIME lastAccessTime, ::LPFILETIME lastWriteTime) const;
	virtual ::DWORD	getCompressedFileSize(::DWORD* fileSizeHigh) const;
	virtual bool	isOpened() const;

protected:
#ifdef _DEBUG
	virtual void	assertValidAsFile() const {assert(isOpened());}
#else
	virtual void	assertValidAsFile() const {}
#endif /* _DEBUG */
private:
	static void	throwCurrentError();

private:
	::WCHAR* fileName_;	// full name of the file
	bool managed_;		// whether close file when deleted object
};

template<class DataType, bool noThrow> class MemoryMappedFile : public KernelHandle<true> {
public:
	class View {
		MANAH_NONCOPYABLE_TAG(View);
	public:
		~View() {if(parent_.get() != 0) ::UnmapViewOfFile(pointer_);}
		DataType* getData() const throw() {return pointer_;}
	private:
		View(MemoryMappedFile& parent, const DataType* pointer) throw() : parent_(parent), pointer_(pointer) {}
		MemoryMappedFile<DataType, noThrow>& parent_;
		DataType* const pointer_;
		friend class MemoryMappedFile<DataType, noThrow>;
	};
	// constructors
	MemoryMappedFile(const File<noThrow>& file, ::DWORD protection,
		const ::SECURITY_ATTRIBUTES* securityAttributes = 0, const ::ULARGE_INTEGER* maximumSize = 0, const ::WCHAR* name = 0);
	MemoryMappedFile(::DWORD desiredAccess, bool inheritHandle, const ::WCHAR* name);
	// methods
	static bool			flushViewOfFile(const void* baseAddress, ::DWORD flushBytes = 0) throw();
	std::auto_ptr<View>	mapView(::DWORD desiredAccess,
							const ::ULARGE_INTEGER* fileOffset = 0, ::DWORD mappingBytes = 0, const void* baseAddress = 0);
};


// File /////////////////////////////////////////////////////////////////////

template<bool noThrow> inline File<noThrow>::File(::HANDLE handle /* = 0*/) throw() : KernelHandle<false>(handle), fileName_(0), managed_(false) {}

template<bool noThrow> inline File<noThrow>::File(const ::WCHAR* fileName,
		::DWORD desiredAccess, ::DWORD shareMode, ::DWORD creationDisposition, ::DWORD flagsAndAttributes,
		::SECURITY_ATTRIBUTES* securityAttributes /* = 0 */, ::HANDLE templateFile /* = 0 */) : KernelHandle<false>(0), fileName_(0) {
	assert(fileName != 0); open(fileName, desiredAccess, shareMode, creationDisposition, flagsAndAttributes, securityAttributes, templateFile);}

template<bool noThrow> inline File<noThrow>::~File() {if(get() != 0 && managed_) close();}

template<bool noThrow> inline void File<noThrow>::abort() {
	if(get() != 0)
		KernelHandle<false>::close();
	delete[] fileName_;
	fileName_ = 0;
}

template<bool noThrow> inline bool File<noThrow>::close() {
	if(get() != 0) {
		if(!KernelHandle<false>::close()) {
			throwCurrentError();
			return false;
		}
	}
	managed_ = false;
	delete[] fileName_;
	fileName_ = 0;
	return true;
}

template<bool noThrow> inline std::auto_ptr<File<noThrow> > File<noThrow>::duplicate() const {
	if(get() == 0)
		return std::auto_ptr<File<noThrow> >(0);

	::HANDLE handle;
	if(!toBoolean(::DuplicateHandle(::GetCurrentProcess(), get(), ::GetCurrentProcess(), &handle, 0, false, DUPLICATE_SAME_ACCESS))) {
		throwCurrentError();
		return std::auto_ptr<File<noThrow> >(0);
	}
	std::auto_ptr<File> newFile(new File(handle));
	assert(newFile->get() != 0);
	newFile->managed_ = managed_;

	return newFile;
}

template<bool noThrow> inline bool File<noThrow>::flush() {
	assertValidAsFile();
	if(!toBoolean(::FlushFileBuffers(get()))) {
		throwCurrentError();
		return false;
	}
	return true;
}

template<bool noThrow> inline ::ULONGLONG File<noThrow>::getLength() const {
	assertValidAsFile();

	::ULARGE_INTEGER len;
	len.LowPart = ::GetFileSize(get(), &len.HighPart);
	if(len.LowPart == -1 && ::GetLastError() != NO_ERROR)
		throwCurrentError();
	return len.QuadPart;
}

template<bool noThrow> inline ::DWORD File<noThrow>::getCompressedFileSize(::DWORD* fileSizeHigh) const {
	assertValidAsFile();
	const ::DWORD	size = ::GetCompressedFileSizeW(fileName_, fileSizeHigh);
	if(size == static_cast<DWORD>(-1) && ::GetLastError() != NO_ERROR)
		throwCurrentError();
	return size;
}

template<bool noThrow> inline bool File<noThrow>::getFileTime(
		::LPFILETIME creationTime, ::LPFILETIME lastAccessTime, ::LPFILETIME lastWriteTime) const {
	assertValidAsFile();
	const bool succeeded = toBoolean(::GetFileTime(get(), creationTime, lastAccessTime, lastWriteTime));
	if(!succeeded && ::GetLastError() != NO_ERROR)
		throwCurrentError();
	return succeeded;
}

template<bool noThrow> inline ::DWORD File<noThrow>::getPosition() const {
	assertValidAsFile();
	const ::DWORD pos = ::SetFilePointer(get(), 0, 0, FILE_CURRENT);
	if(pos == static_cast<::DWORD>(-1))
		throwCurrentError();
	return pos;
}

template<bool noThrow> inline bool File<noThrow>::isOpened() const {return get() != 0;}

template<bool noThrow> inline bool File<noThrow>::lockRange(::DWORD pos, ::DWORD count) {
	assertValidAsFile();
	if(!toBoolean(::LockFile(get(), pos, 0, count, 0))) {
		throwCurrentError();
		return false;
	}
	return true;
}

template<bool noThrow> inline bool File<noThrow>::open(const ::WCHAR* fileName,
		::DWORD desiredAccess, ::DWORD shareMode, ::DWORD creationDisposition, ::DWORD flagsAndAttributes,
		::SECURITY_ATTRIBUTES* securityAttributes /* = 0 */, ::HANDLE templateFile /* = 0 */) {
	assert(fileName != 0);

	if(isOpened()) {
		if(noThrow)
			return false;
		else
			throw FileException("File is already opened.");
	}

	::HANDLE handle = ::CreateFileW(fileName, desiredAccess, shareMode,
		securityAttributes, creationDisposition, flagsAndAttributes, templateFile);
	if(handle == INVALID_HANDLE_VALUE) {
		throwCurrentError();
		return false;
	}

	setHandle(handle);
	fileName_ = new ::WCHAR[std::wcslen(fileName) + 1];
	std::wcscpy(fileName_, fileName);
	managed_ = true;

	return true;
}

template<bool noThrow> inline bool File<noThrow>::read(void* buffer, ::DWORD bytes, ::DWORD* readBytes /* = 0 */) {
	assertValidAsFile();
	assert(buffer != 0);

	if(bytes == 0) {
		if(readBytes != 0)
			*readBytes = 0;
		return true;
	}

	::DWORD readBytesBuffer;
	if(!toBoolean(::ReadFile(get(), buffer, bytes, &readBytesBuffer, 0))) {
		throwCurrentError();
		return false;
	}
	if(readBytes != 0)
		*readBytes = readBytesBuffer;
	return true;
}

template<bool noThrow> inline ::ULONGLONG File<noThrow>::seek(::LONGLONG offset, PointerMovementMode mode) {
	assertValidAsFile();

	::LARGE_INTEGER	offsets;
	offsets.QuadPart = offset;
	offsets.LowPart = ::SetFilePointer(get(), offsets.LowPart, &offsets.HighPart, mode);
	if(offsets.LowPart == -1 && ::GetLastError() != NO_ERROR) {
		throwCurrentError();
		return false;
	}
	return true;
}

template<bool noThrow> inline void File<noThrow>::setLength(::ULONGLONG newLength) {
	assertValidAsFile();
	seek(newLength, FROM_BEGIN);
	if(!toBoolean(::SetEndOfFile(get())))
		throwCurrentError();
}

template<bool noThrow> inline void File<noThrow>::seekToBegin() {seek(0, File<noThrow>::FROM_BEGIN);}

template<bool noThrow> inline ::ULONGLONG File<noThrow>::seekToEnd() {return seek(0, File<noThrow>::FROM_END);}

template<bool noThrow> inline void File<noThrow>::throwCurrentError() {}

template<> inline void File<false>::throwCurrentError() {
	std::string	message;
	void* buffer = 0;
	const ::DWORD n = ::GetLastError();

	::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
		| FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		0, n, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<char*>(&buffer), 0, 0);
	message = static_cast<char*>(buffer);
	::LocalFree(buffer);
	throw FileException(message);
}

template<bool noThrow> inline bool File<noThrow>::unlockRange(::DWORD pos, ::DWORD count) {
	assertValidAsFile();
	if(!toBoolean(::UnlockFile(get(), pos, 0, count, 0))) {
		throwCurrentError();
		return false;
	}
	return true;
}

template<bool noThrow> inline bool File<noThrow>::write(const void* buffer, ::DWORD bytes, ::DWORD* writtenBytes /* = 0 */) {
	assertValidAsFile();
	assert(buffer != 0);

	if(bytes == 0)
		return true;

	::DWORD writtenBytesBuffer;
	if(!toBoolean(::WriteFile(get(), buffer, bytes, (writtenBytes != 0) ? writtenBytes : &writtenBytesBuffer, 0))) {
		throwCurrentError();
		return false;
	}
	return true;
}


// MemoryMappedFile /////////////////////////////////////////////////////////

template<class DataType, bool noThrow>
inline MemoryMappedFile<DataType, noThrow>::MemoryMappedFile(const File<noThrow>& file, ::DWORD protection,
		const ::SECURITY_ATTRIBUTES* attributes /* = 0 */, const ::ULARGE_INTEGER* maximumSize /* = 0 */, const ::WCHAR* name /* = 0 */) {
	try {
		setHandle(::CreateFileMappingW(file.get(), const_cast<::SECURITY_ATTRIBUTES*>(attributes),
			protection, (maximumSize != 0) ? maximumSize->HighPart : 0, (maximumSize != 0) ? maximumSize->LowPart : 0, name));
	} catch(...) {if(!noThrow) throw;}
}

template<class DataType, bool noThrow>
inline MemoryMappedFile<DataType, noThrow>::MemoryMappedFile(::DWORD desiredAccess, bool inheritHandle, const ::WCHAR* name) {
	try {setHandle(::OpenFileMapping(desiredAccess, inheritHandle, name));}
	catch(...) {if(!noThrow) throw;}
}

template<class DataType, bool noThrow>
inline bool MemoryMappedFile<DataType, noThrow>::flushViewOfFile(const void* baseAddress, ::DWORD flushBytes /* = 0 */) throw() {
	return toBoolean(::FlushViewOfFile(baseAddress, flushBytes));}

template<class DataType, bool noThrow>
inline std::auto_ptr<typename MemoryMappedFile<DataType, noThrow>::View> MemoryMappedFile<DataType, noThrow>::mapView(::DWORD desiredAccess,
		const ::ULARGE_INTEGER* fileOffset /* = 0 */, ::DWORD mappingBytes /* = 0 */, const void* baseAddress /* = 0 */) {
	try {
		return std::auto_ptr<View>(new View(*this,
			static_cast<DataType*>(::MapViewOfFileEx(get(), desiredAccess,
				(fileOffset != 0) ? fileOffset->HighPart : 0, (fileOffset != 0) ? fileOffset->LowPart : 0,
				mappingBytes, const_cast<void*>(baseAddress)))));
	} catch(...) {
		if(!noThrow)
			throw;
		return std::auto_ptr<View>(0);
	}
}

}}} // namespace manah.win32.io

#endif /* !MANAH_FILE_HPP */
