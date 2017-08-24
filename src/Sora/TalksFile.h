#pragma once

#include "Talk.h"

#include <string>
#include <vector>
#include <ostream>

namespace Sora {

	class TalksFile {
	public:
		using TalksT = std::vector<Talk>;

		TalksT& Talks() { return talks; }
		const TalksT& Talks() const { return talks; }
		PtrDialogList& PtrDialogs() { return pDialogs; }
		const PtrDialogList& PtrDialogs() const { return pDialogs; }

		virtual ~TalksFile() { }
		const std::string& ErrMsg() const { return err; }

		TalksFile() = default;

		virtual bool WriteTo(std::ostream& os) const { return false; }
		virtual bool WriteTo(const std::string& filename) const { return false; }

	public:
		TalksFile(const TalksFile& _Other) :
			talks(_Other.talks), err(_Other.err) {
			setPDialogs();
		}
		TalksFile& operator=(const TalksFile& _Other) {
			talks = _Other.talks;
			err = _Other.err;
			setPDialogs();
			return *this;
		}
		TalksFile(TalksFile&& _Right) :
			talks(std::move(_Right.talks)), pDialogs(std::move(_Right.pDialogs)), err(std::move(_Right.err)) {
			setPDialogs();
		}
		TalksFile& operator=(TalksFile&& _Right) {
			talks = std::move(_Right.talks);
			pDialogs = std::move(_Right.pDialogs);
			err = std::move(_Right.err);
			setPDialogs();
			return *this;
		}

	protected:
		TalksT talks;
		PtrDialogList pDialogs;
		std::string err;

		void setPDialogs() {
			pDialogs.clear();
			for (auto& talk : this->talks) {
				for (auto& dlg : talk.Dialogs()) {
					pDialogs.push_back(&dlg);
				}
			}
		}
	};
}
