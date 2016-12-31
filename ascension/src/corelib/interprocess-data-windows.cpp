/**
 * @file interprocess-data-windows.cpp
 * Implements @c InterprocessData class on Win32 window system.
 * @author exeal
 * @date 2016-12-25 Created.
 * @date 2016-12-28 Separated from drag-and-drop-windows.cpp.
 */

#include <ascension/corelib/interprocess-data.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/range/algorithm/find_first_of.hpp>
#include <boost/range/algorithm/for_each.hpp>

namespace ascension {
	InterprocessData::InterprocessData() {
	}

	void InterprocessData::data(Format format, std::vector<std::uint8_t>& out) const {
		if(impl_.get() == nullptr)
			throw IllegalStateException("An empty MimeData.");

		FORMATETC formatEtc;
		formatEtc.cfFormat = format;
		formatEtc.ptd = nullptr;
		formatEtc.dwAspect = DVASPECT_CONTENT;
		formatEtc.lindex = -1;
		formatEtc.tymed = TYMED_HGLOBAL;
		win32::AutoZero<STGMEDIUM> medium;
		HRESULT hr = impl_->GetData(&formatEtc, &medium);
		if(FAILED(hr)) {
			if(hr == DV_E_FORMATETC)
				throw UnsupportedFormatException();
			throw hr;
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
			throw hr;
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

	void InterprocessData::setData(Format format, const boost::iterator_range<const std::uint8_t*>& range) {
		if(impl_ == nullptr)
			throw IllegalStateException("An empty MimeData.");

		FORMATETC formatEtc;
		formatEtc.cfFormat = format;
		formatEtc.ptd = nullptr;
		formatEtc.dwAspect = DVASPECT_CONTENT;
		formatEtc.lindex = -1;
		formatEtc.tymed = TYMED_HGLOBAL;
		win32::AutoZero<STGMEDIUM> medium;
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
					throw hr;
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
