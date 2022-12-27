#pragma once

#include <regex>
#include <fstream>
#include <set>

#include "Utils.h"

namespace Distributor {
	struct FormData {
		std::string pluginName;
		uint32_t formId;

		bool operator<(const FormData& o)  const {
			if (pluginName == o.pluginName)
				return formId < o.formId;
			return pluginName.compare(o.pluginName);
		}
	};

	std::map<FormData, std::set<FormData>> g_distMap;

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

	void LoadConfig(const std::string& configPath) {
		std::ifstream configFile(configPath);

		logger::info(FMT_STRING("Reading the config file: {}"), configPath);
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

			FormData listForm = { listFormPlugin,  Utils::ToFormId(listFormId) };
			FormData targetForm = { targetPlugin, Utils::ToFormId(targetFormId) };
			auto it = g_distMap.find(listForm);
			if (it == g_distMap.end()) {
				auto result = g_distMap.insert(std::make_pair(listForm, std::set<FormData>()));
				if (!result.second) {
					logger::info(FMT_STRING("Failed to insert the form: {}"), line);
					continue;
				}

				it = result.first;
			}

			it->second.insert(targetForm);
		}
	}

	void LoadConfigs() {
		const std::filesystem::path configDir{ "Data\\F4SE\\Plugins\\" + std::string(Version::PROJECT) };

		const std::regex filter(".*.txt");
		const std::filesystem::directory_iterator dir_iter(configDir);
		for (auto iter : dir_iter) {
			if (!std::filesystem::is_regular_file(iter.status()))
				continue;

			if (!std::regex_match(iter.path().filename().string(), filter))
				continue;

			LoadConfig(iter.path().string());
		}
	}

	void Distribute() {
		logger::info("Start Distribution"sv);
		for (std::pair<FormData, std::set<FormData>> e : g_distMap) {
			RE::TESForm* form = Utils::GetFormFromIdentifier(e.first.pluginName, e.first.formId);
			if (!form) {
				logger::info(FMT_STRING("Cannot find the FormList: {} | 0x{:08X}"), e.first.pluginName, e.first.formId);
				continue;
			}

			RE::BGSListForm* listForm =	form->As<RE::BGSListForm>();
			if (!listForm) {
				logger::info(FMT_STRING("Invalid FormList: {} | 0x{:08X}"), e.first.pluginName, e.first.formId);
				continue;
			}

			logger::info(FMT_STRING("Distribute to FormList 0x{:08X}..."), listForm->formID);
			for (FormData formData : e.second) {
				form = Utils::GetFormFromIdentifier(formData.pluginName, formData.formId);
				if (!form) {
					logger::info(FMT_STRING("Cannot find the Form: {} | 0x{:08X}"), formData.pluginName, formData.formId);
					continue;
				}

				auto it = std::find(listForm->arrayOfForms.begin(), listForm->arrayOfForms.end(), form);
				if (it == listForm->arrayOfForms.end()) {
					listForm->arrayOfForms.push_back(form);
					logger::info(FMT_STRING("Form Added : 0x{:08X}"), form->formID);
				}
			}
		}
		logger::info("Distribution End"sv);
	}
}
