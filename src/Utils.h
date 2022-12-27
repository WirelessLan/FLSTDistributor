#pragma once

namespace Utils {
	void Trim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
		}));
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	const RE::TESFile* LookupModByName(RE::TESDataHandler* dataHandler, const std::string& pluginName) {
		for (RE::TESFile* file : dataHandler->files) {
			if (file->filename == pluginName)
				return file;
		}
		return nullptr;
	}

	enum RecordFlag	{
		kNone = 0,
		kESM = 1 << 0,
		kActive = 1 << 3,
		kLocalized = 1 << 7,
		kESL = 1 << 9
	};

	bool IsLight(const RE::TESFile* mod) { 
		return (mod->flags & RecordFlag::kESL) == RecordFlag::kESL;
	}

	uint32_t GetPartialIndex(const RE::TESFile* mod) {
		return !IsLight(mod) ? mod->compileIndex : (0xFE000 | mod->smallFileCompileIndex);
	}

	uint32_t ToFormId(const std::string& formIdStr) {
		return std::stoul(formIdStr, nullptr, 16) & 0xFFFFFF;
	}

	RE::TESForm* GetFormFromIdentifier(const std::string& pluginName, const uint32_t formId) {
		RE::TESDataHandler* g_dataHandler = RE::TESDataHandler::GetSingleton();
		if (!g_dataHandler)
			return nullptr;

		const RE::TESFile* mod = LookupModByName(g_dataHandler, pluginName);
		if (!mod || mod->compileIndex == -1)
			return nullptr;

		uint32_t actualFormId = formId;
		uint32_t pluginIndex = GetPartialIndex(mod);
		if (!IsLight(mod))
			actualFormId |= pluginIndex << 24;
		else
			actualFormId |= pluginIndex << 12;

		return RE::TESForm::GetFormByID(actualFormId);
	}

	RE::TESForm* GetFormFromIdentifier(const std::string& pluginName, const std::string& formIdStr) {
		uint32_t formID = ToFormId(formIdStr);
		return GetFormFromIdentifier(pluginName, formID);
	}
}
