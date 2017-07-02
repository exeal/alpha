/**
 * @file interprocess-data.hpp
 * Defines @c InterprocessData class.
 * @author exeal
 * @date 2011-10-12 Created.
 * @date 2016-12-28 Separated from drag-and-drop.hpp.
 */

#ifndef ASCENSION_INTERPROCESS_DATA_HPP
#define ASCENSION_INTERPROCESS_DATA_HPP
#include <ascension/corelib/basic-exceptions.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gtkmm/selectiondata.h>
#	include <gtkmm/targetlist.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QMimeData>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <NSPasteboard.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/com/smart-pointer.hpp>
#	include <ObjIdl.h>
#endif
#include <boost/range/iterator_range.hpp>
#include <vector>

namespace ascension {
	namespace graphics {
		class Image;
	}

	/// Base class of @c InterprocessData.
	class InterprocessDataFormats {
	public:
		/// The format of interprocess data.
		typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			std::string
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			QString
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			???
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			CLIPFORMAT
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			Format;
		/// Returns a list of formats supported by the object.
		virtual void formats(std::vector<Format>& out) const = 0;
		/// Returns @c true if this object can return data for the MIME type specified by @a format.
		virtual bool hasFormat(Format format) const BOOST_NOEXCEPT = 0;
//		/// Returns @c true if this object can return an image.
//		virtual bool hasImage() const BOOST_NOEXCEPT = 0;
		/// Returns @c true if this object can return plain text.
		virtual bool hasText() const BOOST_NOEXCEPT = 0;
		/// Returns @c true if this object can return a list of URI.
		virtual bool hasURIs() const BOOST_NOEXCEPT = 0;
	};

	/**
	 * This class just wraps the following platform-native classes.
	 * <table>
	 *   <tr><th>Window System</th><th>Platform mechanism</th></tr>
	 *   <tr><td>GTK+3</td><td>@c Gtk#TargetList</td></tr>
	 *   <tr><td>Nokia Qt</td><td>@c QMimeData</td></tr>
	 *   <tr><td>Quartz</td><td>@c NSPasteboard</td></tr>
	 *   <tr><td>Win32</td><td>@c IDataObject</td></tr>
	 * </table>
	 */
	class InterprocessData : public InterprocessDataFormats {
	public:
		class UnsupportedFormatException : public UnknownValueException {
		public:
			UnsupportedFormatException() : UnknownValueException("This format is not supported by this MimeData.") {}
		};
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		typedef Glib::RefPtr<Gtk::TargetList> Native;
		typedef Glib::RefPtr<const Gtk::TargetList> ConstNative;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		typedef std::shared_ptr<QMimeData> Native;
		typedef std::shared_ptr<const QMimeData> ConstNative;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		typedef std::shared_ptr<NSPasteboard> Native;
		typedef std::shared_ptr<const NSPasteboard> ConstNative;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		typedef win32::com::SmartPointer<IDataObject> Native;
		typedef win32::com::SmartPointer<IDataObject> Const;
#else
	ASCENSION_CANT_DETECT_PLATFORM();
#endif
	public:
		/// Default constructor creates an empty data.
		InterprocessData();

		/**
		 * Returns the data stored in the object in the specified format.
		 * @param format The format
		 * @param[out] out The result data
		 * @throw UnsupportedFormatException @a format is not supported
		 * @throw ... Any platform specific exception
		 */
		void data(Format format, std::vector<std::uint8_t>& out) const;
//		graphics::Image image() const;
		/**
		 * Returns the text data.
		 * @throw UnsupportedFormatException textual format is not supported
		 */
		String text() const;
		/**
		 * Returns the URI data.
		 * @tparam OutputIterator The type of @a out
		 * @param[out] out The result data
		 * @throw UnsupportedFormatException URI format is not supported
		*/
		template<typename OutputIterator> void uris(OutputIterator out) const;

		/**
		 * Sets the data associated with the specified format.
		 * @param format The format
		 * @param bytes The byte sequence
		 * @throw UnsupportedFormatException @a format is not supported
		 */
		void setData(Format format, const boost::iterator_range<const std::uint8_t*>& bytes);
//		void setImage(const graphics::Image& image);
		/**
		 * Sets the textual data.
		 * @param text The text data
		 * @throw NullPointerException @a text is @c null
		 * @throw UnsupportedFormatException textual format is not supported
		 */
		void setText(const StringPiece& text);
		/**
		 * Sets the URI data.
		 * @tparam SinglePassReadableRange The type of @a uris
		 * @param uris The URI data
		 * @throw UnsupportedFormatException URI format is not supported
		 */
		template<typename SinglePassReadableRange> void setURIs(const SinglePassReadableRange& uris);

		// InterprocessDataFormats
		void formats(std::vector<Format>& out) const override;
		virtual bool hasFormat(Format format) const BOOST_NOEXCEPT override;
//		bool hasImage() const BOOST_NOEXCEPT override;
		bool hasText() const BOOST_NOEXCEPT override;
		bool hasURIs() const BOOST_NOEXCEPT override;

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		std::shared_ptr<Gtk::SelectionData> native();
		std::shared_ptr<const Gtk::SelectionData> native() const;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		std::shared_ptr<QMimeData> native();
		std::shared_ptr<const QMimeData> native() const;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
		??? native();
		const native() const;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		win32::com::SmartPointer<IDataObject> native();
#endif

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
		explicit InterprocessData(Gtk::SelectionData& impl);
	private:
		std::shared_ptr<Gtk::SelectionData> impl_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
		explicit InterprocessData(QMimeData& impl);
	private:
		std::shared_ptr<QMimeData> impl_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
		explicit InterprocessData(win32::com::SmartPointer<IDataObject> impl);
	private:
		win32::com::SmartPointer<IDataObject> impl_;
#else
		ASCENSION_CANT_DETECT_PLATFORM();
#endif
	};
}

#endif // !ASCENSION_INTERPROCESS_DATA_HPP
