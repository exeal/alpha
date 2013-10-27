/**
 * @file drag-and-drop.hpp
 * @author exeal
 * @date 2011-10-12 created
 */

#ifndef ASCENSION_DRAG_AND_DROP_HPP
#define ASCENSION_DRAG_AND_DROP_HPP
#include <ascension/viewer/widgetapi/user-input.hpp>
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
#	include <gdkmm/dragcontext.h>
#	include <gtkmm/selectiondata.h>
#	include <gtkmm/targetlist.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
#	include <QMimeData>
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#	include <NSPasteboard.h>
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
#	include <ascension/win32/com/smart-pointer.hpp>
#	include <ObjIdl.h>
#	include <ShlObj.h>	// IDragSourceHelper
#endif

namespace ascension {
	namespace viewers {
		namespace widgetapi {

			typedef std::uint16_t DropAction;
			const DropAction DROP_ACTION_IGNORE = 0;
			const DropAction DROP_ACTION_COPY = 1 << 0;
			const DropAction DROP_ACTION_MOVE = 1 << 1;
			const DropAction DROP_ACTION_LINK = 1 << 2;
#ifdef ASCENSION_WINDOW_SYSTEM_WIN32
			const DropAction DROP_ACTION_WIN32_SCROLL = 1 << 3;
#endif
			DropAction resolveDefaultDropAction(DropAction possibleActions, UserInput::ModifierKey modifierKeys);

#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
			typedef Glib::RefPtr<Gtk::TargetList> NativeMimeData;
			typedef Glib::RefPtr<const Gtk::TargetList> ConstNativeMimeData;
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
			typedef std::shared_ptr<QMimeData> NativeMimeData;
			typedef std::shared_ptr<const QMimeData> ConstNativeMimeData;
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
			typedef std::shared_ptr<NSPasteboard> NativeMimeData;
			typedef std::shared_ptr<const NSPasteboard> ConstNativeMimeData;
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
			typedef win32::com::SmartPointer<IDataObject> NativeMimeData;
			typedef win32::com::SmartPointer<IDataObject> ConstNativeMimeData;
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif

			class MimeData {
			public:
				/// Default constructor creates an empty MIME data.
				MimeData();

				/// Returns @c true if this object can return an image.
				bool hasImage() const BOOST_NOEXCEPT;
				/// Returns @c true if this object can return plain text.
				bool hasText() const BOOST_NOEXCEPT;
				/// Returns @c true if this object can return a list of URI.
				bool hasURIs() const BOOST_NOEXCEPT;

				graphics::Image image() const;
				String text() const;
				template<typename OutputIterator> void uris(OutputIterator out) const;

				void setImage(const graphics::Image& image);
				void setText(const StringPiece& text);
				template<typename SinglePassReadableRange> void setURIs(const SinglePassReadableRange& uris);
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				typedef Gtk::TargetEntry Format;
				MimeData(const Gtk::SelectionData& borrowed);
			private:
				std::unique_ptr<Gtk::SelectionData> impl_;
				std::shared_ptr<const Gtk::SelectionData> borrowed_;
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				MimeData(const QMimeData& borrowed);
			private:
				std::shared_ptr<QMimeData> impl_;
				std::shared_ptr<const QMimeData> borrowed_;
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
			private:
				???NSPasteboard??? impl_;
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				typedef CLIPFORMAT Format;
				MimeData(IDataObject& borrowed);
			private:
				win32::com::SmartPointer<IDataObject> impl_;
#else
			ASCENSION_CANT_DETECT_PLATFORM();
#endif
			};

			class DragContext {
			public:
				DropAction defaultAction() const BOOST_NOEXCEPT;
				DropAction execute(DropAction supportedActions);
#if 0
				const MimeData mimeData() const BOOST_NOEXCEPT;
#endif
				void setImage(const graphics::Image& image, const graphics::geometry::BasicPoint<std::uint16_t>& hotspot);
				void setMimeData(ConstNativeMimeData data);
				DropAction supportedActions() const BOOST_NOEXCEPT;
			private:
#if defined(ASCENSION_WINDOW_SYSTEM_GTK)
				Glib::RefPtr<Gdk::DragContext> impl_;
#elif defined(ASCENSION_WINDOW_SYSTEM_QT)
				QDrag impl_;
#elif defined(ASCENSION_WINDOW_SYSTEM_QUARTZ)
#elif defined(ASCENSION_WINDOW_SYSTEM_WIN32)
				win32::com::SmartPointer<IDataObject> mimeData_;
				win32::com::SmartPointer<IDragSourceHelper> imageProvider_;
#else
				ASCENSION_CANT_DETECT_PLATFORM();
#endif
			};

			class DragLeaveInput : public Event {};

			class DropInput : public MouseButtonInput {
			public:
				DropInput(const MouseButtonInput& mouse, DropAction possibleActions, ConstNativeMimeData data) :
					MouseButtonInput(mouse), possibleActions_(possibleActions),
					defaultAction_(resolveDefaultDropAction(possibleActions, mouse.modifiers())), mimeData_(data) {}
				void acceptProposedAction();
				DropAction dropAction() const BOOST_NOEXCEPT {return action_;}
				ConstNativeMimeData mimeData() const BOOST_NOEXCEPT {return mimeData_;}
				DropAction possibleActions() const BOOST_NOEXCEPT {return possibleActions_;}
				DropAction proposedAction() const BOOST_NOEXCEPT {return defaultAction_;}
				void setDropAction(DropAction action);
			private:
				const DropAction possibleActions_;
				const DropAction defaultAction_;
				DropAction action_;
				ConstNativeMimeData mimeData_;
			};

			class DragMoveInput : public DropInput {
			public:
				DragMoveInput(const MouseButtonInput& mouse, DropAction possibleActions,
					ConstNativeMimeData data) : DropInput(mouse, possibleActions, data) {}
			};

			class DragEnterInput : public DragMoveInput {
			public:
				DragEnterInput(const MouseButtonInput& mouse, DropAction possibleActions,
					ConstNativeMimeData data) : DragMoveInput(mouse, possibleActions, data) {}
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
	}
}

#endif // !ASCENSION_DRAG_AND_DROP_HPP
