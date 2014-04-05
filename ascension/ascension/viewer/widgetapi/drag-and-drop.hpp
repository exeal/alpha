/**
 * @file drag-and-drop.hpp
 * @author exeal
 * @date 2011-10-12 created
 * @date 2014
 */

#ifndef ASCENSION_DRAG_AND_DROP_HPP
#define ASCENSION_DRAG_AND_DROP_HPP
#include <ascension/viewer/widgetapi/user-input.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/dragcontext.h>
#	include <gtkmm/selectiondata.h>
#	include <gtkmm/targetlist.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QMimeData>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <NSPasteboard.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
#	include <ascension/win32/com/smart-pointer.hpp>
#	include <ObjIdl.h>
#	include <ShlObj.h>	// IDragSourceHelper
#endif
#include <boost/optional.hpp>

namespace ascension {

	namespace graphics {
		class Image;
	}

	namespace viewers {
		namespace widgetapi {

			typedef std::uint16_t DropAction;
			const DropAction DROP_ACTION_IGNORE = 0;
			const DropAction DROP_ACTION_COPY = 1 << 0;
			const DropAction DROP_ACTION_MOVE = 1 << 1;
			const DropAction DROP_ACTION_LINK = 1 << 2;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			const DropAction DROP_ACTION_WIN32_SCROLL = 1 << 3;
#endif
			DropAction resolveDefaultDropAction(DropAction possibleActions, UserInput::Modifiers modifiers);

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			typedef Glib::RefPtr<Gtk::TargetList> NativeMimeData;
			typedef Glib::RefPtr<const Gtk::TargetList> ConstNativeMimeData;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			typedef std::shared_ptr<QMimeData> NativeMimeData;
			typedef std::shared_ptr<const QMimeData> ConstNativeMimeData;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			typedef std::shared_ptr<NSPasteboard> NativeMimeData;
			typedef std::shared_ptr<const NSPasteboard> ConstNativeMimeData;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			typedef win32::com::SmartPointer<IDataObject> NativeMimeData;
			typedef win32::com::SmartPointer<IDataObject> ConstNativeMimeData;
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			class MimeDataFormats {
			public:
				typedef
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					std::string
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
					QString
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
					CLIPFORMAT
#else
					ASCENSION_CANT_DETECT_PLATFORM();
#endif
					Format;
				/// Returns a list of formats supported by the object.
				virtual std::list<Format>&& formats() const;
				/// Returns @c true if this object can return data for the MIME type specified by @a format.
				virtual bool hasFormat(Format format) const BOOST_NOEXCEPT;
//				/// Returns @c true if this object can return an image.
//				virtual bool hasImage() const BOOST_NOEXCEPT;
				/// Returns @c true if this object can return plain text.
				virtual bool hasText() const BOOST_NOEXCEPT;
				/// Returns @c true if this object can return a list of URI.
				virtual bool hasURIs() const BOOST_NOEXCEPT;

#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			public:
				explicit MimeDataFormats(std::vector<std::string>&& targets);
			private:
				const std::vector<std::string> targets_;
#endif
			protected:
				MimeDataFormats() BOOST_NOEXCEPT {}
			};

			class MimeData : public MimeDataFormats {
			public:
				/// Default constructor creates an empty MIME data.
				MimeData();

				std::vector<std::uint8_t>&& data(Format format) const;
//				graphics::Image image() const;
				String text() const;
				template<typename OutputIterator> void uris(OutputIterator out) const;

				void setData(Format format, const boost::iterator_range<const std::uint8_t*>& range);
//				void setImage(const graphics::Image& image);
				void setText(const StringPiece& text);
				template<typename SinglePassReadableRange> void setURIs(const SinglePassReadableRange& uris);

				// MimeDataFormats
				std::list<Format>&& formats() const;
				virtual bool hasFormat(Format format) const BOOST_NOEXCEPT;
//				bool hasImage() const BOOST_NOEXCEPT;
				bool hasText() const BOOST_NOEXCEPT;
				bool hasURIs() const BOOST_NOEXCEPT;

			public:
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				explicit MimeData(Gtk::SelectionData& impl);
			private:
				std::shared_ptr<Gtk::SelectionData> impl_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				explicit MimeData(QMimeData& impl);
			private:
				std::shared_ptr<QMimeData> impl_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				explicit MimeData(win32::com::SmartPointer<IDataObject> impl);
			private:
				win32::com::SmartPointer<IDataObject> impl_;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			};


			class DragContext {
			public:
				explicit DragContext(Widget::reference source) BOOST_NOEXCEPT : source_(source) {}
				DropAction defaultAction() const BOOST_NOEXCEPT;
				DropAction execute(DropAction supportedActions
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
					, int mouseButton, GdkEvent* event
#endif
				);
				void setImage(const graphics::Image& image, const graphics::geometry::BasicPoint<std::uint32_t>& hotspot);
				void setMimeData(std::shared_ptr<const MimeData> data);
				DropAction supportedActions() const BOOST_NOEXCEPT;
			private:
				Widget::reference source_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				Glib::RefPtr<Gdk::DragContext> context_;
				std::shared_ptr<const MimeData> mimeData_;
				Glib::RefPtr<Gdk::Pixbuf> icon_;
				int iconHotspotX_, iconHotspotY_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				QDrag impl_;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				win32::com::SmartPointer<IDataObject> mimeData_;
				win32::com::SmartPointer<IDragSourceHelper> imageProvider_;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			};

			class DragLeaveInput : public Event {};

			class DragInputBase : public MouseButtonInput {
			public:
				DragInputBase(const MouseButtonInput& mouse, DropAction possibleActions) :
					MouseButtonInput(mouse), possibleActions_(possibleActions),
					defaultAction_(resolveDefaultDropAction(possibleActions, mouse.modifiers())) {}
				void acceptProposedAction();
				DropAction dropAction() const BOOST_NOEXCEPT {return action_;}
				DropAction possibleActions() const BOOST_NOEXCEPT {return possibleActions_;}
				DropAction proposedAction() const BOOST_NOEXCEPT {return defaultAction_;}
				void setDropAction(DropAction action);
			private:
				const DropAction possibleActions_, defaultAction_;
				DropAction action_;
			};

			class DropInput : public DragInputBase {
			public:
				DropInput(const MouseButtonInput& mouse, DropAction possibleActions, std::shared_ptr<const MimeData> data) :
					DragInputBase(mouse, possibleActions), mimeData_(data) {}
				std::shared_ptr<const MimeData> mimeData() const BOOST_NOEXCEPT {return mimeData_;}
			private:
				std::shared_ptr<const MimeData> mimeData_;
			};

			class DragMoveInput : public DragInputBase {
			public:
				DragMoveInput(const MouseButtonInput& mouse, DropAction possibleActions, const MimeDataFormats& formats) : DragInputBase(mouse, possibleActions), formats_(formats) {}
				void accept(boost::optional<const graphics::Rectangle> rectangle);
				void ignore(boost::optional<const graphics::Rectangle> rectangle);
				const MimeDataFormats& mimeDataFormats() const BOOST_NOEXCEPT {return formats_;}
			private:
				const MimeDataFormats& formats_;
			};

			class DragEnterInput : public DragMoveInput {
			public:
				DragEnterInput(const MouseButtonInput& mouse, DropAction possibleActions, const MimeDataFormats& formats) : DragMoveInput(mouse, possibleActions, formats) {}
			};

			/**
			 */
			class DropTarget {
			public:
				/// Destructor.
				virtual ~DropTarget() BOOST_NOEXCEPT {}
				virtual void dragEntered(DragEnterInput& input) = 0;
				virtual void dragLeft(DragLeaveInput& input) = 0;
				virtual void dragMoved(DragMoveInput& input) = 0;
				virtual void dropped(DropInput& input) = 0;
			};

		}

		namespace detail {
			class DragEventAdapter {
			public:
				explicit DragEventAdapter(viewers::widgetapi::DropTarget& target) : target_(target) {}
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
				void adaptDragLeaveEvent(const Glib::RefPtr<Gdk::DragContext>& context, guint time);
				bool adaptDragMoveEvent(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
				bool adaptDropEvent(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, guint time);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
				void adaptDragEnterEvent(QDragEnterEvent* event);
				void adaptDragLeaveEvent(QDragLeaveEvent* event);
				void adaptDragMoveEvent(QDragMoveEvent* event);
				void adaptDropEvent(QDropEvent* event);
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
				HRESULT adaptDragEnterEvent(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
				HRESULT adaptDragLeaveEvent();
				HRESULT adaptDragMoveEvent(DWORD keyState, POINTL location, DWORD* effect);
				HRESULT adaptDropEvent(IDataObject* data, DWORD keyState, POINTL location, DWORD* effect);
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			private:
				viewers::widgetapi::DropTarget& target_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#endif
			};
		}
	}
}

#endif // !ASCENSION_DRAG_AND_DROP_HPP
