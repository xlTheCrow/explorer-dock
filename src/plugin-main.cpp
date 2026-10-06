#include "plugin-main.hpp"
#include "explorer-dock-manager.hpp"

#include <obs-frontend-api.h>
#include <windows.h>
#include <ole2.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-explorer-dock", "en-US")

namespace {
ExplorerDockManager g_manager;
bool g_oleInitialized = false;
}

MODULE_EXPORT const char *obs_module_description(void)
{
    return "Native Windows Explorer browser docks for OBS Studio";
}

bool obs_module_load(void)
{
    const HRESULT oleResult = OleInitialize(nullptr);
    if (SUCCEEDED(oleResult)) {
        g_oleInitialized = true;
    } else if (oleResult == RPC_E_CHANGED_MODE) {
        blog(LOG_WARNING, "[obs-explorer-dock] COM is already initialized in a different apartment model; ExplorerBrowser creation may fail.");
    } else {
        blog(LOG_WARNING, "[obs-explorer-dock] OleInitialize failed: 0x%08lX", static_cast<unsigned long>(oleResult));
    }

    obs_frontend_add_tools_menu_item(obs_module_text("Menu.ManageExplorerDocks"), [](void *) {
        g_manager.openManager();
    }, nullptr);

    g_manager.loadAndRestore();
    blog(LOG_INFO, "[obs-explorer-dock] Plugin loaded");
    return true;
}

void obs_module_unload(void)
{
    g_manager.shutdown();
    if (g_oleInitialized) {
        OleUninitialize();
        g_oleInitialized = false;
    }
    blog(LOG_INFO, "[obs-explorer-dock] Plugin unloaded");
}
