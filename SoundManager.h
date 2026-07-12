#pragma once
#include <string>
#include <vector>
#include <memory>
#include <windows.h>
#include "miniaudio.h"

struct AudioDeviceInfo {
    std::wstring name;
    ma_device_id id;
};

struct SoundItem {
    std::wstring name;
    std::wstring path;
    ma_sound soundMain;
    ma_sound soundLocal;
    bool loaded;
    UINT modifiers;
    UINT vk;
};

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    bool Initialize(const ma_device_id* pDeviceID = nullptr);
    void Uninitialize();
    void ChangeDevice(const ma_device_id* pDeviceID);

    void LoadFromDirectory(const std::wstring& directory);
    void AddSound(const std::wstring& path);
    void ToggleSoundItem(size_t index);
    void StopAll();
    void SetVolume(float volume);
    void SetLocalVolume(float volume);

    const std::vector<std::unique_ptr<SoundItem>>& GetSounds() const;
    std::vector<AudioDeviceInfo> GetPlaybackDevices();

    void SetHotkey(size_t index, UINT modifiers, UINT vk);
    void SetConfigState(float micVol, float localVol, bool passThrough, const std::wstring& deviceName);
    
    float GetSavedMicVol() const { return savedMicVol; }
    float GetSavedLocalVol() const { return savedLocalVol; }
    bool GetSavedPassThrough() const { return savedPassThrough; }
    std::wstring GetSavedDeviceName() const { return savedDeviceName; }
    std::wstring GetSoundsDir() const { return soundsDir; }

    void LoadConfig();
    void SaveConfig();

private:
    std::vector<std::unique_ptr<SoundItem>> sounds;
    ma_engine engineMain;
    ma_engine engineLocal;
    ma_context context;
    bool engineInitialized;
    bool contextInitialized;
    float currentVolume;
    float currentLocalVolume;
    
    float savedMicVol = 1.0f;
    float savedLocalVol = 1.0f;
    bool savedPassThrough = false;
    std::wstring savedDeviceName;
    
    std::wstring configPath;
    std::wstring soundsDir;
};
