#pragma once

#include "Talk.h"

#include <string>
#include <vector>
#include <ostream>

namespace Sora {

	class TalksFile {
	public:
		using TalksT = std::vector<Talk>;
		static constexpr int MaxTalksNum = 5000;

		TalksT& Talks() { return talks; }
		const TalksT& Talks() const { return talks; }
		PtrDialogList& PtrDialogs() { return pDialogs; }
		const PtrDialogList& PtrDialogs() const { return pDialogs; }

		virtual ~TalksFile() { }
		const std::string& ErrMsg() const { return err; }

		TalksFile() = default;

		virtual bool WriteTo(std::ostream& os) const { return false; }
		virtual bool WriteTo(const std::string& filename) const { return false; }

		void Resort() {
			TalksT tmp_talks;
			tmp_talks.resize(talks.size(), Talk(-1, Talk::InvalidTalk));
			for (auto & talk : talks) {
				if (talk.No() < 0 || talk.GetType() == Talk::InvalidTalk) continue;
				if (talk.No() >= (int)tmp_talks.size()) {
					tmp_talks.resize(talk.No() + 1, Talk(-1, Talk::InvalidTalk));
				}
				tmp_talks[talk.No()] = std::move(talk);
			}
			while (!tmp_talks.empty() && tmp_talks.back().GetType() == Talk::InvalidTalk) tmp_talks.pop_back();
			talks = std::move(tmp_talks);
			ResetPDialogs();
		}

	public:
		TalksFile(const TalksFile& _Other) :
			talks(_Other.talks), err(_Other.err) {
			ResetPDialogs();
		}
		TalksFile& operator=(const TalksFile& _Other) {
			talks = _Other.talks;
			err = _Other.err;
			ResetPDialogs();
			return *this;
		}
		TalksFile(TalksFile&& _Right) :
			talks(std::move(_Right.talks)), pDialogs(std::move(_Right.pDialogs)), err(std::move(_Right.err)) {
			ResetPDialogs();
		}
		TalksFile& operator=(TalksFile&& _Right) {
			talks = std::move(_Right.talks);
			pDialogs = std::move(_Right.pDialogs);
			err = std::move(_Right.err);
			ResetPDialogs();
			return *this;
		}

	protected:
		TalksT talks;
		PtrDialogList pDialogs;
		std::string err;

	public:
		void ResetPDialogs() {
			pDialogs.clear();
			for (auto& talk : this->talks) {
				for (auto& dlg : talk.Dialogs()) {
					pDialogs.push_back(&dlg);
				}
			}
		}
	};
}
