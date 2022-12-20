#pragma once

#include <regex>
#include <fstream>

#include "Utils.h"

namespace Distributor {
	uint8_t GetNextChar(const std::string& line, uint32_t& index) {
		if (index < line.length())
			return line[index++];

		return 0xFF;
	}

	std::string GetNextData(const std::string& line, uint32_t& index, char delimeter) {
		uint8_t ch;
		std::string retVal = "";

		while ((ch = GetNextChar(line, index)) != 0xFF) {
			if (ch == '#') {
				if (index > 0) index--;
				break;
			}

			if (delimeter != 0 && ch == delimeter)
				break;

			retVal += static_cast<char>(ch);
		}

		Utils::Trim(retVal);
		return retVal;
	}

	void LoadConfig(std::unordered_map<RE::BGSListForm*, std::vector<RE::TESForm*>>& map, const std::string& configPath) {
		std::ifstream configFile(configPath);

		logger::info(FMT_STRING("Read the config file: {}"), configPath);
		if (!configFile.is_open()) {
			logger::info(FMT_STRING("Cannot open the config file: {}"), configPath);
			return;
		}

		std::string line;
		std::string listFormPlugin, listFormId, targetPlugin, targetFormId;
		while (std::getline(configFile, line)) {
			Utils::Trim(line);
			if (line.empty() || line[0] == '#')
				continue;

			uint32_t index = 0;

			listFormPlugin = GetNextData(line, index, '|');
			if (listFormPlugin.empty()) {
				logger::info(FMT_STRING("Cannot read the FormList's plugin name: {}"), line);
				continue;
			}

			listFormId = GetNextData(line, index, '|');
			if (listFormId.empty()) {
				logger::info(FMT_STRING("Cannot read the FormList's formID: {}"), line);
				continue;
			}

			targetPlugin = GetNextData(line, index, '|');
			if (targetPlugin.empty()) {
				logger::info(FMT_STRING("Cannot read the target form's plugin name: {}"), line);
				continue;
			}

			targetFormId = GetNextData(line, index, 0);
			if (targetFormId.empty()) {
				logger::info(FMT_STRING("Cannot read the target form's formID: {}"), line);
				continue;
			}

			RE::TESForm* form = Utils::GetFormFromIdentifier(listFormPlugin, listFormId);
			if (!form) {
				logger::info(FMT_STRING("Cannot find the FormList: {} | {}"), listFormPlugin, listFormId);
				continue;
			}

			RE::BGSListForm* listForm = RE::fallout_cast<RE::BGSListForm*, RE::TESForm>(form);
			if (!listForm) {
				logger::info(FMT_STRING("Invalid FormList: {} | {}"), listFormPlugin, listFormId);
				continue;
			}

			form = Utils::GetFormFromIdentifier(targetPlugin, targetFormId);
			if (!form) {
				logger::info(FMT_STRING("Cannot find the Form: {} | {}"), targetPlugin, targetFormId);
				continue;
			}

			auto it = map.find(listForm);
			if (it == map.end()) {
				std::vector<RE::TESForm*> nVec{ form };
				map.insert(std::make_pair(listForm, nVec));
			}
			else {
				it->second.push_back(form);
			}
		}
	}

	std::unordered_map<RE::BGSListForm*, std::vector<RE::TESForm*>> LoadConfigs() {
		std::unordered_map<RE::BGSListForm*, std::vector<RE::TESForm*>> retMap;
		const std::filesystem::path configDir{ "Data\\F4SE\\Plugins\\" + std::string(Version::PROJECT) };

		const std::regex filter(".*.txt");
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			LoadConfig(retMap, iter.path().string());
		}

		return retMap;
	}

	void Distribute() {
		std::unordered_map<RE::BGSListForm*, std::vector<RE::TESForm*>> configs = LoadConfigs();

		for (std::pair<RE::BGSListForm*, std::vector<RE::TESForm*>> e : configs) {
			logger::info(FMT_STRING("Distribute to FormList {:08X}..."), e.first->formID);
			for (RE::TESForm* form : e.second) {
				auto it = std::find(e.first->arrayOfForms.begin(), e.first->arrayOfForms.end(), form);
				if (it == e.first->arrayOfForms.end()) {
					e.first->arrayOfForms.push_back(form);
					logger::info(FMT_STRING("Form Added : {:08X}"), form->formID);
				}
			}
		}
	}
}
