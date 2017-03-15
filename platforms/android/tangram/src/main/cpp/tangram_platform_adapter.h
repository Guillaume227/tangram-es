#pragma once

#include "platform.h"
#include <memory>
#include <android/asset_manager_jni.h>


std::string resolveScenePath(const char* path);

class PlatformTangramImpl;

class TangramPlatformAdapter : public Tangram::Platform {

	std::shared_ptr<PlatformTangramImpl> const platformImpl_;
    AAssetManager* assetManager = nullptr; // todo: NAO fork does not set the assetManager

public:

    TangramPlatformAdapter(std::shared_ptr<PlatformTangramImpl> const& tangramPlatform);
    
    void requestRender() const override;
    std::vector<char> bytesFromFile(const char* _path) const override;
    void setContinuousRendering(bool _isContinuous) override;
    std::vector<char> systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const override;
    std::vector<Tangram::FontSourceHandle> systemFontFallbacksHandle() const override;
    bool startUrlRequest(const std::string& _url, Tangram::UrlCallback _callback) override;
    void cancelUrlRequest(const std::string& _url) override;

private:

    bool bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) const;
    std::string fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) const;
    std::string fontFallbackPath(int _importance, int _weightHint) const;
};

