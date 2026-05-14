#include "ServerModManager.hpp"

#include "Utils/ZipUtils.hpp"

ServerModManager::ServerModManager() {
    std::filesystem::create_directory("/mods");
}

void ServerModManager::initLoad() {
#ifdef BUILD_TYPE_DEDICATED
    std::string moddir = "mods";
#else
    std::string moddir = "/mods";
#endif

    for (const auto& entry : std::filesystem::directory_iterator(moddir)) {
        if (entry.path().extension().string() == ".zip") {
            std::string modName = entry.path().stem().string();
            ZipUtils::extract_all(entry.path().string(), moddir + "/" + modName);
            loadMod(modName);
        }
    }
}

void ServerModManager::loadMod(std::string& modName) {
    ServerMod* mod = new ServerMod(modName);
    mod->loadAssets();
    mod->loadMainLua();
    mods[modName] = mod;
    std::cout << "Mod " << modName << " loaded" << std::endl;
}

