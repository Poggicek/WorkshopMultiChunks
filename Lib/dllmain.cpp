#include "pch.h"
#include "utils/module.h"
#include <funchook.h>
#include <filesystem>

#define ROOTBIN "/bin/win64/"
#define GAMEBIN "/csgo/bin/win64/"

typedef bool (*PrepareWorkshopUpload_t)(void* a1, void* str, bool a3);
PrepareWorkshopUpload_t g_prepareWorkshopUpload = nullptr;
funchook_t* g_pHook = nullptr;

bool endsWith(const std::string& s, const std::string& suffix)
{
    return s.size() >= suffix.size() && s.rfind(suffix) == (s.size() - suffix.size());
}

std::string replace(std::string str, std::string substr1, std::string substr2)
{
    for (size_t index = str.find(substr1, 0); index != std::string::npos && substr1.length(); index = str.find(substr1, index + substr2.length()))
        str.replace(index, substr1.length(), substr2);
    return str;
}

void Detour_PrepareWorkshopUpload(void* a1, void* str, bool a3)
{
    const char* path = *(const char**)((uintptr_t)a1 + 0x288);
    uint64_t workshopId = *(uint64_t*)((uintptr_t)a1 + 0x1F0);

    std::filesystem::path p(path);

    auto mapName = p.stem().string();
    auto addonPath = p.parent_path();
    Info("Map name: %s\n", mapName.c_str());
    Info("Addon path: %s\n", addonPath.c_str());

    if (!std::filesystem::exists(addonPath / (mapName + "_dir.vpk")))
    {
        Error("Addon path does not exist: %s\n", addonPath.c_str());
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(addonPath))
    {
        auto& path = entry.path();
        auto filename = path.filename().string();
        auto ext = path.extension().string();

        if (ext == ".vpk" && filename.find(mapName) == 0)
        {
            if (endsWith(path.stem().string(), "_dir"))
                std::filesystem::rename(path, addonPath / (mapName + ".vpk"));
            else
            {
                auto replacedString = replace(filename, mapName, std::to_string(workshopId));
                std::filesystem::rename(path, addonPath / "vpks" / std::to_string(workshopId) / replacedString);
            }
        }
    }

    g_prepareWorkshopUpload(a1, str, a3);
}

void SetupHook()
{
    CModule mod(ROOTBIN "tools/", "cs2_workshop_manager");

    const uint8_t sig[] = "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x55\x57\x41\x54\x41\x56\x41\x57\x48\x8D\xAC\x24\x80\xFD\xFF\xFF";

    int err = 0;
    g_prepareWorkshopUpload = (PrepareWorkshopUpload_t)mod.FindSignature(sig, sizeof(sig) - 1, err);

    if (err)
        Error("Failed to find signature, error %d\n", err);

    auto g_pHook = funchook_create();
    funchook_prepare(g_pHook, (void**)&g_prepareWorkshopUpload, (void*)Detour_PrepareWorkshopUpload);
    funchook_install(g_pHook, 0);
}

void MainInit()
{
    SetupHook();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        MainInit();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

