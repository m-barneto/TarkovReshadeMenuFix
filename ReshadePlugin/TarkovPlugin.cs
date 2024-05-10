using BepInEx;
using EFT.UI.Screens;
using EFT.UI;
using System.Reflection;
using System.Runtime.InteropServices;
using Aki.Reflection.Patching;
using BSG.CameraEffects;

namespace TarkovPlugin {
    [BepInPlugin("Mattdokn.ReshadePlugin", "ReshadePlugin", "1.0.0")]
    public class Plugin : BaseUnityPlugin {
        [DllImport("ReshadeAPI.addon")]
        private static extern bool SetMenuState(bool bIsInMenu);
        [DllImport("ReshadeAPI.addon")]
        private static extern bool SetNightVisionState(bool bIsUsingNightVision);
        [DllImport("ReshadeAPI.addon")]
        private static extern bool SetSelectedPreset(string presetPath);

        void Awake() {
            new OnScreenChangePatch().Enable();
            new NightVisionPatch().Enable();
        }

        internal class OnScreenChangePatch : ModulePatch {
            protected override MethodBase GetTargetMethod() =>
                typeof(MenuTaskBar).GetMethod("OnScreenChanged");

            [PatchPrefix]
            public static void Prefix(EEftScreenType eftScreenType) {
                switch (eftScreenType) {
                    case EEftScreenType.None:
                    case EEftScreenType.BattleUI:
                        SetMenuState(false);
                        break;
                    default:
                        SetMenuState(true);
                        break;
                }
            }
        }

        internal class NightVisionPatch : ModulePatch {
            protected override MethodBase GetTargetMethod() => typeof(NightVision).GetMethod("method_1");

            [PatchPrefix]
            public static void OnNvToggle(ref bool on) {
                SetNightVisionState(on);
            }
        }
    }
}
