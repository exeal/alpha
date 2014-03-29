/**
 * @file buffer-bar.hpp
 * Defines @c BufferBar class.
 */

#include <boost/noncopyable.hpp>
#include <gtkmm/toolbar.h>

namespace alpha {
	class Buffer;
	class BufferList;
	class EditorPanes;

	namespace ui {
		///
		class BufferBar : public Gtk::Toolbar, private boost::noncopyable {
		public:
			explicit BufferBar(BufferList& bufferList);

		private:
			// BufferList signals
			void bufferAboutToBeRemoved(BufferList& buffers, Buffer& buffer);
			void bufferAdded(BufferList& buffers, Buffer& buffer);
			void bufferDisplayNameChanged(const Buffer& buffer);
			// EditorPanes signal
			void bufferSelectionChanged(EditorPanes& panes);
			// Gtk.ToolButton signal
			static void buttonClicked(const Buffer& buffer);

		private:
			BufferList& bufferList_;
		};
	}
}
