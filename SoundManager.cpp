#include "SoundManager.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <shlobj.h>

std::string WStringToUTF8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

SoundManager::SoundManager() : engineInitialized(false), contextInitialized(false), currentVolume(1.0f), currentLocalVolume(1.0f), savedMicVol(1.0f), savedLocalVol(1.0f), savedPassThrough(false) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    std::wstring exeDir = p.parent_path().wstring();

    if (std::filesystem::exists(exeDir + L"\\portable.txt")) {
        configPath = exeDir + L"\\config.ini";
        soundsDir = exeDir + L"\\sounds";
    } else {
        wchar_t appDataPath[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath);
        std::filesystem::path appData(appDataPath);
        std::filesystem::path sbAppData = appData / L"Soundboard";
        if (!std::filesystem::exists(sbAppData)) {
            std::filesystem::create_directory(sbAppData);
        }
        configPath = sbAppData.wstring() + L"\\config.ini";

        wchar_t musicPath[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_MYMUSIC, NULL, 0, musicPath);
        std::filesystem::path musicData(musicPath);
        std::filesystem::path sbMusic = musicData / L"Soundboard";
        if (!std::filesystem::exists(sbMusic)) {
            std::filesystem::create_directory(sbMusic);
        }
        soundsDir = sbMusic.wstring();
    }
}

SoundManager::~SoundManager() {
    Uninitialize();
}

bool SoundManager::Initialize(const ma_device_id* pDeviceID) {
    if (!contextInitialized) {
        if (ma_context_init(NULL, 0, NULL, &context) == MA_SUCCESS) {
            contextInitialized = true;
        }
    }

    ma_engine_config engineConfigMain = ma_engine_config_init();
    engineConfigMain.pContext = &context;
    engineConfigMain.pPlaybackDeviceID = (ma_device_id*)pDeviceID;
    
    if (ma_engine_init(&engineConfigMain, &engineMain) != MA_SUCCESS) {
        if (pDeviceID != nullptr) {
            engineConfigMain.pPlaybackDeviceID = NULL;
            if (ma_engine_init(&engineConfigMain, &engineMain) != MA_SUCCESS) {
                return false;
            }
        } else {
            return false;
        }
    }
    
    ma_engine_config engineConfigLocal = ma_engine_config_init();
    engineConfigLocal.pContext = &context;
    engineConfigLocal.pPlaybackDeviceID = NULL; // Always default
    if (ma_engine_init(&engineConfigLocal, &engineLocal) != MA_SUCCESS) {
        ma_engine_uninit(&engineMain);
        return false;
    }
    
    engineInitialized = true;
    ma_engine_set_volume(&engineMain, currentVolume);
    ma_engine_set_volume(&engineLocal, currentLocalVolume);
    return true;
}

void SoundManager::Uninitialize() {
    for (auto& s : sounds) {
        if (s->loaded) {
            ma_sound_uninit(&s->soundMain);
            ma_sound_uninit(&s->soundLocal);
            s->loaded = false;
        }
    }
    if (engineInitialized) {
        ma_engine_uninit(&engineMain);
        ma_engine_uninit(&engineLocal);
        engineInitialized = false;
    }
    if (contextInitialized) {
        ma_context_uninit(&context);
        contextInitialized = false;
    }
}

void SoundManager::ChangeDevice(const ma_device_id* pDeviceID) {
    std::vector<std::wstring> paths;
    for (const auto& s : sounds) {
        paths.push_back(s->path);
    }
    
    for (auto& s : sounds) {
        if (s->loaded) {
            ma_sound_uninit(&s->soundMain);
            ma_sound_uninit(&s->soundLocal);
            s->loaded = false;
        }
    }
    if (engineInitialized) {
        ma_engine_uninit(&engineMain);
        ma_engine_uninit(&engineLocal);
        engineInitialized = false;
    }

    Initialize(pDeviceID);

    for (size_t i = 0; i < sounds.size(); ++i) {
        std::string utf8Path = WStringToUTF8(sounds[i]->path);
        bool mainLoaded = (ma_sound_init_from_file(&engineMain, utf8Path.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &sounds[i]->soundMain) == MA_SUCCESS);
        bool localLoaded = (ma_sound_init_from_file(&engineLocal, utf8Path.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &sounds[i]->soundLocal) == MA_SUCCESS);
        if (mainLoaded && localLoaded) {
            sounds[i]->loaded = true;
        }
    }
}

void SoundManager::LoadFromDirectory(const std::wstring& directory) {
    if (!std::filesystem::exists(directory)) {
        std::filesystem::create_directory(directory);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            AddSound(entry.path().wstring());
        }
    }
    LoadConfig();
}

void SoundManager::AddSound(const std::wstring& path) {
    std::filesystem::path p(path);
    std::filesystem::path targetDir(soundsDir);
    std::wstring finalPath = path;
    
    std::filesystem::path destPath = targetDir / p.filename();
    if (std::filesystem::absolute(p) != std::filesystem::absolute(destPath)) {
        try {
            std::filesystem::copy_file(p, destPath, std::filesystem::copy_options::overwrite_existing);
            finalPath = destPath.wstring();
        } catch(...) {}
    }

    auto item = std::make_unique<SoundItem>();
    item->name = p.stem().wstring();
    item->path = finalPath;
    item->loaded = false;
    item->modifiers = 0;
    item->vk = 0;

    if (engineInitialized) {
        std::string utf8Path = WStringToUTF8(finalPath);
        bool mainLoaded = (ma_sound_init_from_file(&engineMain, utf8Path.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &item->soundMain) == MA_SUCCESS);
        bool localLoaded = (ma_sound_init_from_file(&engineLocal, utf8Path.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &item->soundLocal) == MA_SUCCESS);
        if (mainLoaded && localLoaded) {
            item->loaded = true;
        }
    }

    sounds.push_back(std::move(item));
}

void SoundManager::ToggleSoundItem(size_t index) {
    if (index >= sounds.size() || !sounds[index]->loaded) return;
    
    if (ma_sound_is_playing(&sounds[index]->soundMain) || ma_sound_is_playing(&sounds[index]->soundLocal)) {
        ma_sound_stop(&sounds[index]->soundMain);
        ma_sound_seek_to_pcm_frame(&sounds[index]->soundMain, 0);
        
        ma_sound_stop(&sounds[index]->soundLocal);
        ma_sound_seek_to_pcm_frame(&sounds[index]->soundLocal, 0);
    } else {
        ma_sound_seek_to_pcm_frame(&sounds[index]->soundMain, 0);
        ma_sound_start(&sounds[index]->soundMain);
        
        ma_sound_seek_to_pcm_frame(&sounds[index]->soundLocal, 0);
        ma_sound_start(&sounds[index]->soundLocal);
    }
}

void SoundManager::StopAll() {
    for (auto& s : sounds) {
        if (s->loaded) {
            if (ma_sound_is_playing(&s->soundMain)) {
                ma_sound_stop(&s->soundMain);
                ma_sound_seek_to_pcm_frame(&s->soundMain, 0);
            }
            if (ma_sound_is_playing(&s->soundLocal)) {
                ma_sound_stop(&s->soundLocal);
                ma_sound_seek_to_pcm_frame(&s->soundLocal, 0);
            }
        }
    }
}

void SoundManager::SetVolume(float volume) {
    currentVolume = volume;
    if (engineInitialized) {
        ma_engine_set_volume(&engineMain, currentVolume);
    }
}

void SoundManager::SetLocalVolume(float volume) {
    currentLocalVolume = volume;
    if (engineInitialized) {
        ma_engine_set_volume(&engineLocal, currentLocalVolume);
    }
}

const std::vector<std::unique_ptr<SoundItem>>& SoundManager::GetSounds() const {
    return sounds;
}

std::vector<AudioDeviceInfo> SoundManager::GetPlaybackDevices() {
    std::vector<AudioDeviceInfo> list;
    if (!contextInitialized) return list;

    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    if (ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, NULL, NULL) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < playbackDeviceCount; ++i) {
            AudioDeviceInfo info;
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, pPlaybackDeviceInfos[i].name, -1, NULL, 0);
            std::wstring wstr(size_needed, 0);
            MultiByteToWideChar(CP_UTF8, 0, pPlaybackDeviceInfos[i].name, -1, &wstr[0], size_needed);
            info.name = wstr;
            info.id = pPlaybackDeviceInfos[i].id;
            list.push_back(info);
        }
    }
    return list;
}

void SoundManager::SetHotkey(size_t index, UINT modifiers, UINT vk) {
    if (index < sounds.size()) {
        sounds[index]->modifiers = modifiers;
        sounds[index]->vk = vk;
        SaveConfig();
    }
}

void SoundManager::SetConfigState(float micVol, float localVol, bool passThrough, const std::wstring& deviceName) {
    savedMicVol = micVol;
    savedLocalVol = localVol;
    savedPassThrough = passThrough;
    savedDeviceName = deviceName;
    SaveConfig();
}

void SoundManager::SaveConfig() {
    std::wofstream file(configPath.c_str());
    if (file.is_open()) {
        file << L"MicVol=" << savedMicVol << L"\n";
        file << L"LocalVol=" << savedLocalVol << L"\n";
        file << L"PassThrough=" << (savedPassThrough ? 1 : 0) << L"\n";
        file << L"Device=" << savedDeviceName << L"\n";
        file << L"[Hotkeys]\n";
        for (const auto& s : sounds) {
            if (s->vk != 0) {
                file << s->name << L"=" << s->modifiers << L"," << s->vk << L"\n";
            }
        }
    }
}

void SoundManager::LoadConfig() {
    std::wifstream file(configPath.c_str());
    if (file.is_open()) {
        std::wstring line;
        bool inHotkeys = false;
        while (std::getline(file, line)) {
            if (line == L"[Hotkeys]") {
                inHotkeys = true;
                continue;
            }
            size_t eqPos = line.find(L'=');
            if (eqPos != std::wstring::npos) {
                std::wstring key = line.substr(0, eqPos);
                std::wstring val = line.substr(eqPos + 1);
                
                if (!inHotkeys) {
                    try {
                        if (key == L"MicVol") savedMicVol = std::stof(val);
                        else if (key == L"LocalVol") savedLocalVol = std::stof(val);
                        else if (key == L"PassThrough") savedPassThrough = (std::stoi(val) != 0);
                        else if (key == L"Device") savedDeviceName = val;
                    } catch(...) {}
                } else {
                    size_t commaPos = val.find(L',');
                    if (commaPos != std::wstring::npos) {
                        try {
                            UINT mod = std::stoi(val.substr(0, commaPos));
                            UINT vk = std::stoi(val.substr(commaPos + 1));
                            for (auto& s : sounds) {
                                if (s->name == key) {
                                    s->modifiers = mod;
                                    s->vk = vk;
                                    break;
                                }
                            }
                        } catch(...) {}
                    }
                }
            }
        }
    }
}
