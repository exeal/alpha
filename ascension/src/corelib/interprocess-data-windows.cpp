/**
 * @file interprocess-data-windows.cpp
 * Implements @c InterprocessData class on Win32 window system.
 * @author exeal
 * @date 2016-12-25 Created.
 * @date 2016-12-28 Separated from drag-and-drop-windows.cpp.
 */

#include <ascension/corelib/interprocess-data.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#include <ascension/win32/com/unknown-impl.hpp>
#include <ascension/win32/windows.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <list>

namespace ascension {
	namespace {
		/**
		 * @internal @c IDataObject implementation for OLE image drag-and-drop.
		 * <h3>Implementation References</h3>
		 * - "The Shell Drag/Drop Helper Object Part 1: IDropTargetHelper"
		 *   (http://msdn.microsoft.com/en-us/library/ms997500.aspx)
		 * - "The Shell Drag/Drop Helper Object Part 2: IDropSourceHelper"
		 *   (http://msdn.microsoft.com/en-us/library/ms997502.aspx)
		 * and their Japanese translations
		 * - "Shell Drag/Drop Helper オブジェクト 第 1 部 : IDropTargetHelper"
		 *   (http://www.microsoft.com/japan/msdn/windows/windows2000/ddhelp_pt1.aspx)
		 * - "Shell Drag/Drop Helper オブジェクト 第 2 部 : IDropSourceHelper"
		 *   (http://www.microsoft.com/japan/msdn/windows/windows2000/ddhelp_pt2.aspx)
		 * ...but these documents have many bugs. Well, there is no interface named "IDropSourceHelper".
		 * @note This does not support any device-specific renderings.
		 */
		class IDataObjectImpl : public win32::com::IUnknownImpl<ASCENSION_WIN32_COM_INTERFACE(IDataObject), win32::com::NoReferenceCounting> {
		public:
			/// @see IDataObject#GetData
			STDMETHODIMP GetData(FORMATETC* format, STGMEDIUM* medium) override {
				if(format == nullptr || medium == nullptr)
					return E_INVALIDARG;
				else if(format->lindex != -1)
					return DV_E_LINDEX;
				const auto entry(find(*format, std::begin(entries_)));
				if(entry == std::end(entries_))
					return DV_E_FORMATETC;
				else if(((*entry)->format.tymed & format->tymed) == 0)
					return DV_E_TYMED;
				const HRESULT hr = ::CopyStgMedium(&(*entry)->medium, medium);
				if(SUCCEEDED(hr))
					medium->pUnkForRelease = nullptr;
				return hr;
			}
			/// @see IDataObject#GetDataHere
			STDMETHODIMP GetDataHere(FORMATETC*, STGMEDIUM*) override {
				return E_NOTIMPL;
			}
			/// @see IDataObject#QueryGetData
			STDMETHODIMP QueryGetData(FORMATETC* format) override {
				if(format == nullptr)
					return E_INVALIDARG;
				else if(format->lindex != -1)
					return DV_E_LINDEX;
				const auto entry(find(*format, std::begin(entries_)));
				if(entry == std::end(entries_))
					return DV_E_FORMATETC;
				return (((*entry)->format.tymed & format->tymed) != 0) ? S_OK : DV_E_TYMED;
			}
			/// @see IDataObject#GetCanonicalFormatEtc
			STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* in, FORMATETC* out) override {
				if(in == nullptr || out == nullptr)
					return E_INVALIDARG;
				else if(in->lindex != -1)
					return DV_E_LINDEX;
				else if(in->ptd != nullptr)
					return DV_E_FORMATETC;
				*out = *in;
				return DATA_S_SAMEFORMATETC;
			}
			/// @see IDataObject#SetData
			STDMETHODIMP SetData(FORMATETC* format, STGMEDIUM* medium, BOOL release) override {
				if(format == nullptr || medium == nullptr)
					return E_INVALIDARG;
				STGMEDIUM clone;
				if(!release) {
					if(FAILED(::CopyStgMedium(medium, &clone)))
						return E_FAIL;
				}
				auto entry(std::begin(entries_));
				while(true) {
					entry = find(*format, entry);
					if(entry == std::end(entries_) || ((*entry)->format.tymed & format->tymed) != 0)
						break;
				}
				if(entry == std::end(entries_)) {	// a entry has the given format does not exist
					const std::shared_ptr<Entry> newEntry(static_cast<Entry*>(::CoTaskMemAlloc(sizeof(Entry))), &::CoTaskMemFree);
					if(newEntry.get() == nullptr)
						return E_OUTOFMEMORY;
					newEntry->format = *format;
					newEntry->medium = win32::makeZero<STGMEDIUM>();
					entries_.push_back(newEntry);
					entry = std::prev(std::end(entries_));
				} else if((*entry)->medium.tymed != TYMED_NULL) {
					::ReleaseStgMedium(&(*entry)->medium);
					(*entry)->medium = win32::makeZero<STGMEDIUM>();
				}

				assert((*entry)->medium.tymed == TYMED_NULL);
				(*entry)->medium = win32::boole(release) ? *medium : clone;
				return S_OK;
			}
			/// @see IDataObject#EnumFormatEtc
			STDMETHODIMP EnumFormatEtc(DWORD direction, IEnumFORMATETC** enumerator) override {
				if(direction == DATADIR_SET)
					return E_NOTIMPL;
				else if(direction != DATADIR_GET)
					return E_INVALIDARG;
				else if(enumerator == nullptr)
					return E_INVALIDARG;
				FORMATETC* const buffer = static_cast<FORMATETC*>(::CoTaskMemAlloc(sizeof(FORMATETC) * entries_.size()));
				if(buffer == nullptr)
					return E_OUTOFMEMORY;
				std::size_t j = 0;
				for(auto i(std::begin(entries_)), e(std::end(entries_)); i != e; ++i, ++j)
					buffer[j] = (*i)->format;
				const HRESULT hr = ::CreateFormatEnumerator(static_cast<UINT>(entries_.size()), buffer, enumerator);
				::CoTaskMemFree(buffer);
				return hr;
			}
			/// @see IDataObject#DAdvise
			STDMETHODIMP DAdvise(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD) override {
				return OLE_E_ADVISENOTSUPPORTED;
			}
			/// @see IDataObject#DUnadvise
			STDMETHODIMP DUnadvise(DWORD) override {
				return OLE_E_ADVISENOTSUPPORTED;
			}
			/// @see IDataObject#EnumDAdvise
			STDMETHODIMP EnumDAdvise(LPENUMSTATDATA*) override {
				return OLE_E_ADVISENOTSUPPORTED;
			}
		private:
			struct Entry : private boost::noncopyable {
				FORMATETC format;
				STGMEDIUM medium;
				Entry() BOOST_NOEXCEPT : format(win32::makeZero<FORMATETC>()), medium(win32::makeZero<STGMEDIUM>()) {
				}
				Entry(Entry&& other) BOOST_NOEXCEPT : format(std::move(other.format)), medium(std::move(other.medium)) {
					other.format = win32::makeZero<FORMATETC>();
					other.medium = win32::makeZero<STGMEDIUM>();
				}
				Entry& operator=(Entry&& other) BOOST_NOEXCEPT {
					format = std::move(other.format);
					other.format = win32::makeZero<FORMATETC>();
					medium = std::move(other.medium);
					other.medium = win32::makeZero<STGMEDIUM>();
					return *this;
				}
				~Entry() BOOST_NOEXCEPT {
					::CoTaskMemFree(format.ptd);
					::ReleaseStgMedium(&medium);
				}
			};

			/**
			 * Finds the entry which has the specified format.
			 * @param format The format to search
			 * @param initial The position where the search starts
			 * @return An iterator which addresses the found entry, or the end if not found
			 */
			std::list<std::shared_ptr<Entry>>::iterator find(const FORMATETC& format, std::list<std::shared_ptr<Entry>>::iterator initial) const BOOST_NOEXCEPT {
				const auto e(std::end(const_cast<IDataObjectImpl*>(this)->entries_));
				if(format.ptd == nullptr) {	// this does not support DVTARGETDEVICE
					for(auto i(initial); i != e; ++i) {
						const auto& other = (*i)->format;
						if(other.cfFormat == format.cfFormat && other.dwAspect == format.dwAspect && other.lindex == format.lindex)
							return i;
					}
				}
				return e;
			}

		private:
			std::list<std::shared_ptr<Entry>> entries_;
		};
	}

	InterprocessData::InterprocessData() {
	}

	/**
	 * Creates an @c InterprocessData object from @c IDataObject.
	 * @param impl The @c IDataObject
	 */
	InterprocessData::InterprocessData(win32::com::SmartPointer<IDataObject> impl) : impl_(impl) {
	}

	void InterprocessData::data(Format format, std::vector<std::uint8_t>& out) const {
		if(impl_.get() == nullptr)
			throw UnsupportedFormatException();

		FORMATETC formatEtc;
		formatEtc.cfFormat = format;
		formatEtc.ptd = nullptr;
		formatEtc.dwAspect = DVASPECT_CONTENT;
		formatEtc.lindex = -1;
		formatEtc.tymed = TYMED_HGLOBAL;
		auto medium(win32::makeZero<STGMEDIUM>());
		HRESULT hr = impl_->GetData(&formatEtc, &medium);
		if(FAILED(hr)) {
			if(hr == DV_E_FORMATETC)
				throw UnsupportedFormatException();
			throw makePlatformError(hr);
		}

		if(medium.tymed == TYMED_FILE || medium.tymed == TYMED_HGLOBAL) {
			const std::uint8_t* bytes = nullptr;
			std::size_t nbytes;
			if(medium.tymed == TYMED_FILE && medium.lpszFileName != nullptr) {
				bytes = reinterpret_cast<const std::uint8_t*>(medium.lpszFileName);
				nbytes = sizeof(OLECHAR) / sizeof(std::uint8_t) * std::wcslen(medium.lpszFileName);
			} else if(medium.tymed == TYMED_HGLOBAL && medium.hGlobal != nullptr) {
				bytes = static_cast<const std::uint8_t*>(::GlobalLock(medium.hGlobal));
				nbytes = ::GlobalSize(medium.hGlobal);
			}

			std::vector<uint8_t> temp;
			temp.reserve(nbytes);
			boost::for_each(boost::make_iterator_range_n(bytes, nbytes), [&temp](uint8_t byte) {
				temp.push_back(byte);
			});
			std::swap(out, temp);
			if(medium.tymed == TYMED_HGLOBAL)
				::GlobalUnlock(medium.hGlobal);
		} else {
			win32::com::SmartPointer<IStream> stream;
			if(medium.tymed == TYMED_ISTORAGE) {
				if(medium.pstg != nullptr)
					hr = medium.pstg->CreateStream(L"", STGM_READ | STGM_SHARE_DENY_NONE | STGM_CREATE, 0, 0, stream.initialize());
			} else if(medium.tymed = TYMED_ISTREAM)
				stream.reset(medium.pstm);

			if(stream.get() != nullptr) {
				std::vector<std::uint8_t> temp;
				static uint8_t buffer[1024];
				ULONG readBytes;
				while(true) {
					hr = stream->Read(buffer, 1024, &readBytes);
					if(FAILED(hr))
						break;
					boost::for_each(boost::make_iterator_range_n(buffer, readBytes), [&temp](std::uint8_t byte) {
						temp.push_back(byte);
					});
					if(hr == S_FALSE) {
						std::swap(out, temp);
						break;
					}
				}
			}
		}

		::ReleaseStgMedium(&medium);
		if(FAILED(hr))
			throw makePlatformError(hr);
	}

	void InterprocessData::formats(std::vector<Format>& out) const {
		std::vector<Format> formats;
		if(impl_.get() != nullptr) {
			win32::com::SmartPointer<IEnumFORMATETC> enumerator;
			HRESULT hr = impl_->EnumFormatEtc(DATADIR_GET, enumerator.initialize());
			if(SUCCEEDED(hr)) {
				FORMATETC format;
				ULONG nfetched;
				for(enumerator->Reset(); enumerator->Next(1, &format, &nfetched) == S_OK; ) {
					formats.push_back(format.cfFormat);
					if(format.ptd != nullptr)
						::CoTaskMemFree(format.ptd);
				}
			}
		}
		std::swap(out, formats);
	}

	bool InterprocessData::hasFormat(Format format) const BOOST_NOEXCEPT {
		std::vector<Format> temp;
		formats(temp);
		return boost::find(temp, format) != boost::const_end(temp);
	}

	bool InterprocessData::hasText() const BOOST_NOEXCEPT {
		std::vector<Format> temp;
		formats(temp);
		const Format textFormats[] = {CF_OEMTEXT, CF_TEXT, CF_UNICODETEXT};
		return boost::find_first_of(temp, textFormats) != boost::const_end(temp);
	}

	bool InterprocessData::hasURIs() const BOOST_NOEXCEPT {
		return false;	// TODO: Not implemented.
	}

	win32::com::SmartPointer<IDataObject> InterprocessData::native() {
		return impl_;
	}

	void InterprocessData::setData(Format format, const boost::iterator_range<const std::uint8_t*>& range) {
		if(impl_ == nullptr)
			impl_.reset(new IDataObjectImpl);

		FORMATETC formatEtc;
		formatEtc.cfFormat = format;
		formatEtc.ptd = nullptr;
		formatEtc.dwAspect = DVASPECT_CONTENT;
		formatEtc.lindex = -1;
		formatEtc.tymed = TYMED_HGLOBAL;
		auto medium(win32::makeZero<STGMEDIUM>());
		medium.tymed = TYMED_HGLOBAL;
		if(nullptr != (medium.hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, boost::size(range)))) {
			if(std::uint8_t* const buffer = static_cast<std::uint8_t*>(::GlobalLock(medium.hGlobal))) {
				boost::copy(range, buffer);
				::GlobalUnlock(medium.hGlobal);
				const HRESULT hr = impl_->SetData(&formatEtc, &medium, true);
				if(FAILED(hr)) {
					::ReleaseStgMedium(&medium);
					if(hr == DV_E_FORMATETC)
						throw UnsupportedFormatException();
					throw makePlatformError(hr);
				}
			}
		}
	}

	namespace {
		UINT localeCodePage(LCID locale) {
			const auto n = ::GetLocaleInfoW(locale, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, nullptr, 0);
			if(n != 0) {
				std::unique_ptr<WCHAR[]> p(new WCHAR[n]);
				if(0 != ::GetLocaleInfoW(locale, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, p.get(), n))
					return *reinterpret_cast<DWORD*>(p.get());
			}
			throw makePlatformError();
		}
	}

	void InterprocessData::setText(const StringPiece& text) {
		// UTF-16
		setData(CF_UNICODETEXT, boost::make_iterator_range(reinterpret_cast<const std::uint8_t*>(text.cbegin()), reinterpret_cast<const std::uint8_t*>(text.cend())));

		// ANSI
		const auto codePage = localeCodePage(LOCALE_USER_DEFAULT);
		static_assert(sizeof(char) == sizeof(std::uint8_t), "");
		static_assert(sizeof(WCHAR) == sizeof(Char), "");
		int nativeLength = ::WideCharToMultiByte(codePage, 0, reinterpret_cast<const WCHAR*>(text.cbegin()), static_cast<int>(text.length()), nullptr, 0, nullptr, nullptr);
		if(nativeLength != 0 || text.empty()) {
			std::unique_ptr<std::uint8_t[]> nativeBuffer(new std::uint8_t[nativeLength]);
			nativeLength = ::WideCharToMultiByte(codePage, 0, reinterpret_cast<const WCHAR*>(text.cbegin()), static_cast<int>(text.length()), reinterpret_cast<char*>(nativeBuffer.get()), nativeLength, nullptr, nullptr);
			if(nativeLength != 0 || text.empty()) {
				setData(CF_TEXT, boost::make_iterator_range_n(nativeBuffer.get(), nativeLength));
				setData(CF_OEMTEXT, boost::make_iterator_range_n(nativeBuffer.get(), nativeLength));

				std::uint8_t lcid[sizeof(LCID) / sizeof(std::uint8_t)];
				*reinterpret_cast<LCID*>(lcid) = LOCALE_USER_DEFAULT;
				setData(CF_LOCALE, boost::make_iterator_range(lcid));
			}
		}
	}

	String InterprocessData::text() const {
		std::vector<std::uint8_t> buffer;
		try {
			data(CF_UNICODETEXT, buffer);
			return String(reinterpret_cast<const Char*>(buffer.data()), buffer.size() / (sizeof(Char) / sizeof(std::uint8_t)));
		} catch(...) {
		}

		data(CF_LOCALE, buffer);
		const auto codePage = localeCodePage(*reinterpret_cast<const LCID*>(buffer.data()));
		try {
			data(CF_TEXT, buffer);
		} catch(...) {
			data(CF_OEMTEXT, buffer);
		}
		if(buffer.empty())
			return String();

		static_assert(sizeof(char) == sizeof(std::uint8_t), "");
		static_assert(sizeof(WCHAR) == sizeof(Char), "");
		const int ucsLength = ::MultiByteToWideChar(
			codePage, MB_PRECOMPOSED, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), nullptr, 0);
		std::unique_ptr<WCHAR[]> ucsBuffer(new WCHAR[ucsLength]);
		if(0 != ::MultiByteToWideChar(codePage, MB_PRECOMPOSED,
				reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), ucsBuffer.get(), static_cast<int>(ucsLength)))
			return String(reinterpret_cast<const Char*>(ucsBuffer.get()), ucsLength - 1);
		throw makePlatformError();
	}
}

#endif
