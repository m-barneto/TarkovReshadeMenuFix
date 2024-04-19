// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

using namespace reshade::api;
using namespace reshade;

static effect_runtime* runtime = nullptr;
static CSimpleIniA* config = nullptr;
static bool isUsingNightVision = false;
static bool isInMenu = false;

static std::vector<std::string> effects{};
static std::map<std::string, bool> menuEffects{};
static std::map<std::string, bool> nvEffects{};

static bool updateActiveEffects() {
	// in normal vision raid
	bool state = !isInMenu && !isUsingNightVision;
	std::cout << "State: " << state << std::endl;
	if (runtime) {
		for (const auto& kv : menuEffects) {
			if (kv.second) {
				effect_technique technique = runtime->find_technique(nullptr, kv.first.c_str());
				if (technique == 0) {
					std::cout << "Technique wasn't found! " << kv.first << std::endl;
					continue;
				}
				runtime->set_technique_state(technique, false);
			}
		}
		for (const auto& kv : nvEffects) {
			if (kv.second) {
				effect_technique technique = runtime->find_technique(nullptr, kv.first.c_str());
				if (technique == 0) {
					std::cout << "Technique wasn't found! " << kv.first << std::endl;
					continue;
				}
				runtime->set_technique_state(technique, false);
			}
		}
		return true;
	}
	return false;
}

static std::vector<std::string> getActiveEffects() {
	std::vector<std::string> activeEffects{};
	for (int i = 0; i < effects.size(); i++) {
		effect_technique technique = runtime->find_technique(nullptr, effects[i].c_str());
		if (technique == 0) {
			std::cout << "Technique wasn't found! " << effects[i].c_str() << std::endl;
			continue;
		}
		if (runtime->get_technique_state(technique)) {
			activeEffects.push_back(effects[i]);
		}
	}
	return activeEffects;
}

static void refreshEffects() {
	effects.clear();

	const std::filesystem::path shadersDirectory = "./reshade-shaders/Shaders";

	for (const auto& entry : std::filesystem::recursive_directory_iterator(shadersDirectory))
	{
		if (entry.is_regular_file() && entry.path().filename().extension() == ".fx")
		{
			std::string entryPath = entry.path().filename().string();

			std::cout << entry.path().filename().string().substr(0, entryPath.size() - 3) << std::endl;
			effects.push_back(entry.path().filename().string().substr(0, entryPath.size() - 3));
		}
	}
	//sort files
	std::sort(effects.begin(), effects.end());
}

static void saveDefaultConfig() {
	std::string configDirectory = "./reshade-shaders";
	std::filesystem::create_directory(configDirectory);

	std::string fullPath = configDirectory + "\\" + "TarkovReshadeMenuFix.ini";

	CSimpleIniA config;
	config.SetUnicode();

	for (int i = 0; i < effects.size(); i++) {
		config.SetBoolValue("DisabledMenuEffects", effects[i].c_str(), true);
	}
	for (int i = 0; i < effects.size(); i++) {
		config.SetBoolValue("DisabledNightVisionEffects", effects[i].c_str(), true);
	}

	config.SaveFile(fullPath.c_str());
}

static CSimpleIniA* loadConfig() {
	// Check path, if it exists, read it, otherwise, load defaults
	std::string configDirectory = "./reshade-shaders";
	std::filesystem::create_directory(configDirectory);

	std::string fullPath = configDirectory + "\\" + "TarkovReshadeMenuFix.ini";

	if (!std::filesystem::exists(fullPath)) {
		// create default
		saveDefaultConfig();
	}
	// load config

	CSimpleIniA config;
	config.LoadFile(fullPath.c_str());

	for (int i = 0; i < effects.size(); i++) {
		// if its in config
		
	}

	return &config;
}

#pragma region Reshade
static void onInitEffectRuntime(effect_runtime* effectRuntime) {
	runtime = effectRuntime;
}

static void onBeginEffects(effect_runtime* eRuntime, command_list* cmd_list, resource_view rtv, resource_view rtv_srgb) {
	eRuntime->enumerate_techniques(nullptr, [](effect_runtime* runtime, effect_technique technique)
		{
			const int size = 255;
			char buffer[size];
			runtime->get_technique_name<255>(technique, buffer);
			buffer[size - 1] = '\0';
			std::string techniqueName = std::string(buffer);

			effects.push_back(techniqueName);
		}
	);
	config = loadConfig();
	reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(&onBeginEffects);
}

static void drawSettingsOverlay(reshade::api::effect_runtime* effectRuntime) {
	bool hasChanged = false;
	ImGui::Text("Effects to be disabled in menus.");
	for (const auto& kv : menuEffects) {
		// set effect state set_technique_state (effect_technique technique, bool enabled)
		// find_technique (const char *effect_name, const char *technique_name)=0
		effect_technique technique = effectRuntime->find_technique(nullptr, kv.first.c_str());
		if (technique == 0) {
			std::cout << "Technique wasn't found! " << kv.first << std::endl;
			continue;
		}
		if (ImGui::Checkbox(kv.first.c_str(), &menuEffects[kv.first])) {
			hasChanged = true;
		}
	}

	ImGui::Text("Effects to be disabled while using night vision.");
	for (const auto& kv : nvEffects) {
		// set effect state set_technique_state (effect_technique technique, bool enabled)
		// find_technique (const char *effect_name, const char *technique_name)=0
		effect_technique technique = effectRuntime->find_technique(nullptr, kv.first.c_str());
		if (technique == 0) {
			std::cout << "Technique wasn't found! " << kv.first << std::endl;
			continue;
		}
		if (ImGui::Checkbox(kv.first.c_str(), &nvEffects[kv.first])) {
			hasChanged = true;
		}
	}

	if (hasChanged) {
		// save settings
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		if (!reshade::register_addon(hModule))
			return FALSE;
		reshade::register_event<reshade::addon_event::init_effect_runtime>(&onInitEffectRuntime);
		reshade::register_event<reshade::addon_event::reshade_begin_effects>(&onBeginEffects);
		reshade::register_overlay(nullptr, &drawSettingsOverlay);
		break;
	case DLL_PROCESS_DETACH:
		//reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(&on_reshade_begin_effects);
		reshade::unregister_addon(hModule);
		break;
	}

	return TRUE;
}
#pragma endregion

#pragma region Exports
extern "C" __declspec(dllexport) bool SetEffectsState(bool enabled) {
	if (runtime) {
		runtime->set_effects_state(enabled);
		return true;
	}
	return false;
}

extern "C" __declspec(dllexport) bool SetMenuState(bool bIsInMenu) {
	isInMenu = bIsInMenu;
	return updateActiveEffects();
}

extern "C" __declspec(dllexport) bool SetNightVisionState(bool bIsUsingNightVision) {
	isUsingNightVision = bIsUsingNightVision;
	return updateActiveEffects();
}
#pragma endregion
