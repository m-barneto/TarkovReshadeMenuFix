// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

using namespace reshade::api;
using namespace reshade;

enum PresetState {
    NORMAL,
    MENU,
    NIGHT_VISION
};

static PresetState state = PresetState::MENU;
static effect_runtime* runtime = nullptr;
static CSimpleIniA* reshadeMenuFixConfig = nullptr;

static bool disableInMenus;
static bool disableWithNightVision;


static ImGui::FileBrowser normalPresetFileDialog;
static ImGui::FileBrowser menuPresetFileDialog;
static ImGui::FileBrowser nvPresetFileDialog;

static std::string normalPresetPath;
static std::string menuPresetPath;
static std::string nightVisionPresetPath;

static const std::string configDirectory = "./reshade-shaders";
static const std::string fullConfigPath = configDirectory + "\\" + "TarkovReshadeMenuFix.ini";


static bool updateActiveEffects() {
    if (runtime) {
        if (disableInMenus || disableWithNightVision) {
            if (!runtime->get_effects_state()) {
                runtime->set_effects_state(true);
            }
        }

        switch (state) {
        case PresetState::NORMAL:
            runtime->set_current_preset_path(normalPresetPath.c_str());
            break;
        case PresetState::MENU:
            if (disableInMenus) {
                runtime->set_effects_state(false);
            }
            else {
                runtime->set_current_preset_path(menuPresetPath.c_str());
            }
            break;
        case PresetState::NIGHT_VISION:
            if (disableWithNightVision) {
                runtime->set_effects_state(false);
            }
            else {
                runtime->set_current_preset_path(nightVisionPresetPath.c_str());
            }
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
    config.SetBoolValue("Behavior", "DisableInMenus", true);
    config.SetValue("Presets", "MenuPresetPath", buffer);
    config.SetBoolValue("Behavior", "DisableWithNightVision", true);
    config.SetValue("Presets", "NightVisionPresetPath", buffer);

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

    normalPresetPath = config->GetValue("Presets", "NormalPresetPath", "");
    menuPresetPath = config->GetValue("Presets", "MenuPresetPath", "");
    nightVisionPresetPath = config->GetValue("Presets", "NightVisionPresetPath", "");

    disableInMenus = config->GetBoolValue("Behavior", "DisableInMenus", true);
    disableWithNightVision = config->GetBoolValue("Behavior", "DisableWithNightVision", true);

    return config;
}

#pragma region Reshade
static void onInitEffectRuntime(effect_runtime* effectRuntime) {
    runtime = effectRuntime;
}

static void onBeginEffects(effect_runtime* eRuntime, command_list* cmd_list, resource_view rtv, resource_view rtv_srgb) {
    reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(&onBeginEffects);
    if (reshadeMenuFixConfig != nullptr) {
        free(reshadeMenuFixConfig);
    }
    reshadeMenuFixConfig = loadConfig();
    updateActiveEffects();
}

static void drawSettingsOverlay(reshade::api::effect_runtime* effectRuntime) {
    ImGui::Text("Normal Preset:");
    ImGui::Text(reshadeMenuFixConfig->GetValue("Presets", "NormalPresetPath", "Error!"));
    ImGui::SameLine();
    if (ImGui::Button("Select##Normal")) {
        normalPresetFileDialog.Open();
    }

    if (ImGui::Checkbox("Disable effects in menus", &disableInMenus)) {
        reshadeMenuFixConfig->SetBoolValue("Behavior", "DisableInMenu", disableInMenus);
        saveAndUpdate();
    }
    if (!disableInMenus) {
        ImGui::Text("Menu Preset:");
        ImGui::Text(reshadeMenuFixConfig->GetValue("Presets", "MenuPresetPath", "Error!"));
        ImGui::SameLine();
        if (ImGui::Button("Select##Menu")) {
            menuPresetFileDialog.Open();
        }
    }

    if (ImGui::Checkbox("Disable effects when using night vision", &disableWithNightVision)) {
        reshadeMenuFixConfig->SetBoolValue("Behavior", "DisableWithNightVision", disableWithNightVision);
        saveAndUpdate();
    }
    if (!disableWithNightVision) {
        ImGui::Text("Night Vision Preset:");
        ImGui::Text(reshadeMenuFixConfig->GetValue("Presets", "NightVisionPresetPath", "Error!"));
        ImGui::SameLine();
        if (ImGui::Button("Select##NightVision")) {
            nvPresetFileDialog.Open();
        }
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
        reshade::unregister_event<reshade::addon_event::init_effect_runtime>(&onInitEffectRuntime);
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

extern "C" __declspec(dllexport) void SetSelectedPreset(const char* presetPath) {
    runtime->set_current_preset_path(presetPath);
}
#pragma endregion
