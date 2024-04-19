// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

using namespace reshade::api;
using namespace reshade;

enum PresetState {
    NORMAL,
    MENU,
    NIGHT_VISION
};

static PresetState state = PresetState::NORMAL;
static effect_runtime* runtime = nullptr;
static CSimpleIniA* reshadeMenuFixConfig = nullptr;




static ImGui::FileBrowser normalPresetFileDialog;
static ImGui::FileBrowser menuPresetFileDialog;
static ImGui::FileBrowser nvPresetFileDialog;

static std::string normalPresetPath;
static std::string menuPresetPath;
static std::string nightVisionPresetPath;

static const std::string configDirectory = "./reshade-shaders";
static const std::string fullConfigPath = configDirectory + "\\" + "TarkovReshadeMenuFix.ini";


static std::vector<std::string> effects{};
static std::map<std::string, bool> menuEffects{};
static std::map<std::string, bool> nvEffects{};

static bool updateActiveEffects() {
    if (runtime) {
        switch (state) {
        case PresetState::NORMAL:
            runtime->set_current_preset_path(normalPresetPath.c_str());
            break;
        case PresetState::MENU:
            runtime->set_current_preset_path(menuPresetPath.c_str());
            break;
        case PresetState::NIGHT_VISION:
            runtime->set_current_preset_path(nightVisionPresetPath.c_str());
            break;
        }
        return true;
    }
    return false;
}

static void saveAndUpdate() {
    reshadeMenuFixConfig->SaveFile(fullConfigPath.c_str());
    updateActiveEffects();
}

static void saveDefaultConfig() {
    std::filesystem::create_directory(configDirectory);

    CSimpleIniA config;
    config.SetUnicode();

    const int size = 255;
    char buffer[size];
    runtime->get_current_preset_path(buffer);
    buffer[size - 1] = '\0';

    config.SetValue("Presets", "NormalPresetPath", buffer);
    config.SetValue("Presets", "NightVisionPresetPath", buffer);
    config.SetValue("Presets", "MenuPresetPath", buffer);

    config.SaveFile(fullConfigPath.c_str());
}

static CSimpleIniA* loadConfig() {
    // Check path, if it exists, read it, otherwise, load defaults
    std::filesystem::create_directory(configDirectory);

    if (!std::filesystem::exists(fullConfigPath)) {
        // create default
        saveDefaultConfig();
    }
    // load config

    CSimpleIniA* config = new CSimpleIniA();
    config->LoadFile(fullConfigPath.c_str());

    if (config->GetValue("Presets", "NormalPresetPath") == nullptr) {
        std::cout << "Normal Preset Path not found in loaded config!" << std::endl;
    }
    else {
        normalPresetPath = config->GetValue("Presets", "NormalPresetPath");
    }
    if (config->GetValue("Presets", "MenuPresetPath") == nullptr) {
        std::cout << "Menu Preset Path not found in loaded config!" << std::endl;
    }
    else {
        menuPresetPath = config->GetValue("Presets", "MenuPresetPath");
    }
    if (config->GetValue("Presets", "NightVisionPresetPath") == nullptr) {
        std::cout << "Night Vision Preset Path not found in loaded config!" << std::endl;
    }
    else {
        nightVisionPresetPath = config->GetValue("Presets", "NightVisionPresetPath");
    }

    return config;
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
    if (reshadeMenuFixConfig != nullptr) {
        free(reshadeMenuFixConfig);
    }
    reshadeMenuFixConfig = loadConfig();
    reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(&onBeginEffects);
}

static void drawSettingsOverlay(reshade::api::effect_runtime* effectRuntime) {
    ImGui::Text("Normal Preset:");
    ImGui::Text(reshadeMenuFixConfig->GetValue("Presets", "NormalPresetPath", "Error!"));
    ImGui::SameLine();
    if (ImGui::Button("Select##Normal")) {
        normalPresetFileDialog.Open();
    }

    ImGui::Text("Menu Preset:");
    ImGui::Text(reshadeMenuFixConfig->GetValue("Presets", "MenuPresetPath", "Error!"));
    ImGui::SameLine();
    if (ImGui::Button("Select##Menu")) {
        menuPresetFileDialog.Open();
    }

    ImGui::Text("Night Vision Preset:");
    ImGui::Text(reshadeMenuFixConfig->GetValue("Presets", "NightVisionPresetPath", "Error!"));
    ImGui::SameLine();
    if (ImGui::Button("Select##NightVision")) {
        nvPresetFileDialog.Open();
    }


    normalPresetFileDialog.Display();
    menuPresetFileDialog.Display();
    nvPresetFileDialog.Display();

    if (normalPresetFileDialog.HasSelected()) {
        std::cout << "Selected filename" << normalPresetFileDialog.GetSelected().string() << std::endl;
        normalPresetPath = normalPresetFileDialog.GetSelected().string();
        normalPresetFileDialog.ClearSelected();

        reshadeMenuFixConfig->SetValue("Presets", "NormalPresetPath", normalPresetPath.c_str());
        saveAndUpdate();
    }

    if (menuPresetFileDialog.HasSelected()) {
        std::cout << "Selected filename" << menuPresetFileDialog.GetSelected().string() << std::endl;
        menuPresetPath = menuPresetFileDialog.GetSelected().string();
        menuPresetFileDialog.ClearSelected();

        reshadeMenuFixConfig->SetValue("Presets", "MenuPresetPath", menuPresetPath.c_str());
        saveAndUpdate();
    }


    if (nvPresetFileDialog.HasSelected()) {
        std::cout << "Selected filename" << nvPresetFileDialog.GetSelected().string() << std::endl;
        nightVisionPresetPath = nvPresetFileDialog.GetSelected().string();
        nvPresetFileDialog.ClearSelected();

        reshadeMenuFixConfig->SetValue("Presets", "NightVisionPresetPath", nightVisionPresetPath.c_str());
        saveAndUpdate();
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
        menuPresetFileDialog.SetTitle("Menu Preset");
        menuPresetFileDialog.SetTypeFilters({ ".ini" });
        nvPresetFileDialog.SetTitle("Night Vision Preset");
        nvPresetFileDialog.SetTypeFilters({ ".ini" });
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

extern "C" __declspec(dllexport) bool SetMenuState(bool isInMenu) {
    PresetState newState = isInMenu ? PresetState::MENU : PresetState::NORMAL;
    if (newState != state) {
        state = newState;
        return updateActiveEffects();
    }
    return true;
}

extern "C" __declspec(dllexport) bool SetNightVisionState(bool isUsingNightVision) {
    PresetState newState = isUsingNightVision ? PresetState::NIGHT_VISION : PresetState::NORMAL;
    if (newState != state) {
        state = newState;
        return updateActiveEffects();
    }
    return true;
}
#pragma endregion
