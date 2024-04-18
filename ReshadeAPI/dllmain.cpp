// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

using namespace reshade::api;
using namespace reshade;

static effect_runtime* s_pRuntime = nullptr;

static std::vector<std::string> effects{};
static std::map<std::string, bool> menuEffects{};
static std::map<std::string, bool> nvEffects{};

#pragma region Exports
extern "C" __declspec(dllexport) bool SetEffectsState(bool enabled) {
    if (s_pRuntime) {
        s_pRuntime->set_effects_state(enabled);
        return true;
    }
    return false;
}

extern "C" __declspec(dllexport) bool SetMenuState(bool inMenu) {
    if (s_pRuntime) {
        bool state = !inMenu;
        for (const auto& kv : menuEffects) {
            if (kv.second) {
                // set effect state set_technique_state (effect_technique technique, bool enabled)
                // find_technique (const char *effect_name, const char *technique_name)=0
                effect_technique technique = s_pRuntime->find_technique(nullptr, kv.first.c_str());
                if (technique == 0) {
                    std::cout << "Technique wasn't found! " << kv.first << std::endl;
                    continue;
                }
                s_pRuntime->set_technique_state(technique, state);
            }
        }
        return true;
    }
    return false;
}
#pragma endregion

static void saveConfig() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error rc = ini.LoadFile("example.ini");
    if (rc < 0) { /* handle error */ };
}

static void refreshEffects() {
    const std::filesystem::path shadersDirectory = L"reshade-shaders\\Shaders";

    for (const auto& entry : std::filesystem::recursive_directory_iterator(shadersDirectory)) {
        if (entry.is_regular_file() && entry.path().filename().extension() == ".fx") {
            effects.push_back(entry.path().filename().string());
        }
    }
    //sort files
    std::sort(effects.begin(), effects.end());
}

#pragma region Reshade
static void onBeginEffects(effect_runtime* runtime, command_list* cmd_list, resource_view rtv, resource_view rtv_srgb) {
    s_pRuntime = runtime;
    s_pRuntime->enumerate_techniques(nullptr, [](effect_runtime* runtime, effect_technique technique)
        {
            const int size = 255;
            char buffer[size];
            runtime->get_technique_name<255>(technique, buffer);
            buffer[size - 1] = '\0';
            std::string techniqueName = std::string(buffer);
            // Items that will be disabled when going into menus
            bool state = runtime->get_technique_state(technique);
            if (state) {
                menuEffects[techniqueName] = runtime->get_technique_state(technique);
                nvEffects[techniqueName] = runtime->get_technique_state(technique);
            }
        }
    );
    reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(&onBeginEffects);
}

static void drawSettingsOverlay(reshade::api::effect_runtime* runtime) {
    bool hasChanged = false;
    ImGui::Text("Effects to be disabled in menus.");
    for (const auto& kv : menuEffects) {
        // set effect state set_technique_state (effect_technique technique, bool enabled)
        // find_technique (const char *effect_name, const char *technique_name)=0
        effect_technique technique = s_pRuntime->find_technique(nullptr, kv.first.c_str());
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
        effect_technique technique = s_pRuntime->find_technique(nullptr, kv.first.c_str());
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

