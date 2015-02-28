/**
 * @file caret-gtk.cpp
 * @author exeal
 * @date 2014-04-20 Created.
 */

#include <ascension/viewer/caret.hpp>
#if ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)

namespace ascension {
	namespace viewer {
		void copySelection(Caret& caret, bool useKillRing) {
			// TODO: Not implemented.
		}

		void cutSelection(Caret& caret, bool useKillRing) {
			// TODO: Not implemented.
		}

		void Caret::abortInput() {
			// TODO: Not implemented.
		}

		bool Caret::canPastePlatformData() const {
			// TODO: Not implemented.
			return false;
		}

		void Caret::paste(bool useKillRing) {
			// TODO: Not implemented.
		}
	}
}

#endif	// ASCENSION_SELECTS_WINDOW_SYSTEM(GTK)
