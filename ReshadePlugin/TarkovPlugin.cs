using BepInEx;
using EFT.UI.Screens;
using EFT.UI;
using System.Reflection;
using System.Runtime.InteropServices;
using Aki.Reflection.Patching;

namespace TarkovPlugin {
    [BepInPlugin("Mattdokn.ReshadePlugin", "ReshadePlugin", "1.0.0")]
    public class Plugin : BaseUnityPlugin {

        [DllImport("ReshadeAPI.addon")]
        private static extern bool SetMenuState(bool inMenu);

        void Awake() {
            new OnScreenChangePatch().Enable();
        }

        internal class OnScreenChangePatch : ModulePatch {
            protected override MethodBase GetTargetMethod() =>
                typeof(MenuTaskBar).GetMethod("OnScreenChanged");

            [PatchPrefix]
            public static void Prefix(EEftScreenType eftScreenType) {
                // NotificationManagerClass.DisplayMessageNotification(eftScreenType.ToString());
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
    }
}
