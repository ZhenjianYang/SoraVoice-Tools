#pragma once

namespace Encode {
	enum Encode {
		GBK,
		SJIS,
		UTF8
	};

	static inline int GetChCount_Utf8(const char* p) {
		unsigned t = (unsigned char)*p;
		if (t < 0x80) return 1;
		else if(t >= 0xC0 && t < 0xE0) return  2;
		else if (t >= 0xE0) return 3;
		else return 1;
	}

	static inline int GetChCount_GBK(const char* p) {
		return (unsigned char)*p < 0x80 ? 1 : 2;
	}

	static inline int GetChCount_SJis(const char* p) {
		return (unsigned char)*p < 0x80 || ((unsigned char)*p >= 0xA1 && (unsigned char)*p <= 0xDF) ? 1 : 2;
	}

	static auto GetChCoutFun(Encode encode) {
		switch (encode)
		{
		case Encode::GBK:
			return GetChCount_GBK;
		case Encode::SJIS:
			return GetChCount_SJis;
		case Encode::UTF8:
			return GetChCount_Utf8;
		default:
			return (decltype(&GetChCount_GBK))nullptr;
		}
	}
}
