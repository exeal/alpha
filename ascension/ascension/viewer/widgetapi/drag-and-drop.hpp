/**
 * @file drag-and-drop.hpp
 * @author exeal
 * @date 2011-10-12 created
 * @date 2014
 */

#ifndef ASCENSION_DRAG_AND_DROP_HPP
#define ASCENSION_DRAG_AND_DROP_HPP
#include <ascension/viewer/widgetapi/event/located-user-input.hpp>
#include <ascension/viewer/widgetapi/widget.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#	include <gdkmm/dragcontext.h>
#	include <gtkmm/selectiondata.h>
#	include <gtkmm/targetlist.h>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
#	include <QMimeData>
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
#	include <NSDragging.h>	// NSDragOperation
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

	namespace viewer {
		namespace widgetapi {
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
			typedef Gdk::DragAction DropAction;
			const DropAction DROP_ACTION_IGNORE = Gdk::ACTION_DEFAULT;
			const DropAction DROP_ACTION_COPY = Gdk::ACTION_COPY;
			const DropAction DROP_ACTION_MOVE = Gdk::ACTION_MOVE;
			const DropAction DROP_ACTION_LINK = Gdk::ACTION_LINK;
			const DropAction DROP_ACTION_GTK_PRIVATE = Gdk::ACTION_PRIVATE;
			const DropAction DROP_ACTION_GTK_ASK = Gdk::ACTION_ASK;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QT)
			typedef Gdk::DragAction DropAction;
			const DropAction DROP_ACTION_IGNORE = Qt::IgnoreAction;
			const DropAction DROP_ACTION_COPY = Qt::CopyAction;
			const DropAction DROP_ACTION_MOVE = Qt::MoveAction;
			const DropAction DROP_ACTION_LINK = Qt::LinkAction;
			const DropAction DROP_ACTION_QT_MASK = Qt::ActionMask;
			const DropAction DROP_ACTION_QT_TARGET_MOVE = Qt::TargetMoveAction;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(QUARTZ)
			typedef Gdk::DragAction DropAction;
			const DropAction DROP_ACTION_IGNORE = NSDragOperationNone;
			const DropAction DROP_ACTION_COPY = NSDragOperationCopy;
			const DropAction DROP_ACTION_MOVE = NSDragOperationMove;
			const DropAction DROP_ACTION_LINK = NSDragOperationLink;
			const DropAction DROP_ACTION_OSX_GENERIC = NSDragOperationGeneric;
			const DropAction DROP_ACTION_OSX_PRIVATE = NSDragOperationPrivate;
			const DropAction DROP_ACTION_OSX_DELETE = NSDragOperationDelete;
			const DropAction DROP_ACTION_OSX_EVERY = NSDragOperationEvery;
#elif ASCENSION_SELECTS_WINDOW_SYSTEM(WIN32)
			typedef Gdk::DragAction DropAction;
			const DropAction DROP_ACTION_IGNORE = DROPEFFECT_NONE;
			const DropAction DROP_ACTION_COPY = DROPEFFECT_COPY;
			const DropAction DROP_ACTION_MOVE = DROPEFFECT_MOVE;
			const DropAction DROP_ACTION_LINK = DROPEFFECT_LINK;
			const DropAction DROP_ACTION_WIN32_SCROLL = DROPEFFECT_SCROLL;
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			DropAction resolveDefaultDropAction(DropAction possibleActions,
				event::LocatedUserInput::MouseButton buttons, const event::KeyboardModifiers& modifiers);

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
				virtual void formats(std::vector<Format>& out) const;
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

				void data(Format format, std::vector<std::uint8_t>& out) const;
//				graphics::Image image() const;
				String text() const;
				template<typename OutputIterator> void uris(OutputIterator out) const;

				void setData(Format format, const boost::iterator_range<const std::uint8_t*>& range);
//				void setImage(const graphics::Image& image);
				void setText(const StringPiece& text);
				template<typename SinglePassReadableRange> void setURIs(const SinglePassReadableRange& uris);

				// MimeDataFormats
				void formats(std::vector<Format>& out) const override;
				virtual bool hasFormat(Format format) const BOOST_NOEXCEPT override;
//				bool hasImage() const BOOST_NOEXCEPT override;
				bool hasText() const BOOST_NOEXCEPT override;
				bool hasURIs() const BOOST_NOEXCEPT override;

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
				void setImage(const graphics::Image& image, const boost::geometry::model::d2::point_xy<std::uint32_t>& hotspot);
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

			class DragLeaveInput : public event::Event {};

			class DragInputBase : public event::LocatedUserInput {
			public:
				DragInputBase(const LocatedUserInput& userInput, DropAction possibleActions) :
					event::LocatedUserInput(userInput), possibleActions_(possibleActions),
					defaultAction_(resolveDefaultDropAction(possibleActions, userInput.buttons(), userInput.modifiers())) {}
				void acceptProposedAction() {setDropAction(proposedAction());}
				DropAction dropAction() const BOOST_NOEXCEPT {return action_;}
				DropAction possibleActions() const BOOST_NOEXCEPT {return possibleActions_;}
				DropAction proposedAction() const BOOST_NOEXCEPT {return defaultAction_;}
				void setDropAction(DropAction action) {action_ = action;}
			private:
				const DropAction possibleActions_, defaultAction_;
				DropAction action_;
			};

			class DropInput : public DragInputBase {
			public:
				DropInput(const LocatedUserInput& userInput, DropAction possibleActions, std::shared_ptr<const MimeData> data) :
					DragInputBase(userInput, possibleActions), mimeData_(data) {}
				std::shared_ptr<const MimeData> mimeData() const BOOST_NOEXCEPT {return mimeData_;}
			private:
				std::shared_ptr<const MimeData> mimeData_;
			};

			class DragMoveInput : public DragInputBase {
			public:
				DragMoveInput(const LocatedUserInput& userInput, DropAction possibleActions, const MimeDataFormats& formats) : DragInputBase(userInput, possibleActions), formats_(formats) {}
				void accept(boost::optional<const graphics::Rectangle> rectangle) {}	// TODO: Not implemented.
				void ignore(boost::optional<const graphics::Rectangle> rectangle) {}	// TODO: Not implemented.
				const MimeDataFormats& mimeDataFormats() const BOOST_NOEXCEPT {return formats_;}
			private:
				const MimeDataFormats& formats_;
			};

			class DragEnterInput : public DragMoveInput {
			public:
				DragEnterInput(const LocatedUserInput& userInput, DropAction possibleActions, const MimeDataFormats& formats) : DragMoveInput(userInput, possibleActions, formats) {}
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
				explicit DragEventAdapter(viewer::widgetapi::DropTarget& target) : target_(target) {}
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
				viewer::widgetapi::DropTarget& target_;
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
#endif
			};
		}
	}
}

#endif // !ASCENSION_DRAG_AND_DROP_HPP
