// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <string>
#include <vector>


using namespace reshade::api;
using namespace reshade;


static effect_runtime* s_pRuntime = nullptr;

#include <map>
#include <iostream>

static std::map<std::string, bool> effects{};

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
        for (const auto& kv : effects) {
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

// Callback when Reshade begins effects
static void on_reshade_begin_effects(effect_runtime* runtime, command_list* cmd_list, resource_view rtv, resource_view rtv_srgb) {
    s_pRuntime = runtime;
    s_pRuntime->enumerate_techniques(nullptr, [](effect_runtime* runtime, effect_technique technique)
        {
            const int size = 255;
            char buffer[size];
            runtime->get_technique_name<255>(technique, buffer);
            // Items that will be disabled when going into menus
            bool state = runtime->get_technique_state(technique);
            if (state) {
                effects[buffer] = runtime->get_technique_state(technique);
            }
        }
    );
    reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(&on_reshade_begin_effects);
}

static void draw_settings_overlay(reshade::api::effect_runtime* runtime)
{
    ImGui::Text("Effects to be disabled in menus.");
    for (const auto& kv : effects) {
        // set effect state set_technique_state (effect_technique technique, bool enabled)
        // find_technique (const char *effect_name, const char *technique_name)=0
        effect_technique technique = s_pRuntime->find_technique(nullptr, kv.first.c_str());
        if (technique == 0) {
            std::cout << "Technique wasn't found! " << kv.first << std::endl;
            continue;
        }
        ImGui::Checkbox(kv.first.c_str(), &effects[kv.first]);
    }
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        if (!reshade::register_addon(hModule))
            return FALSE;

        reshade::register_event<reshade::addon_event::reshade_begin_effects>(&on_reshade_begin_effects);
        reshade::register_overlay(nullptr, &draw_settings_overlay);
        break;
    case DLL_PROCESS_DETACH:
        //reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(&on_reshade_begin_effects);
        reshade::unregister_addon(hModule);
        break;
    }

    return TRUE;
}

