#include <util/url.h>
#include "tangram.h"

#include "tangram_platform_adapter.h"
#include "PlatformTangramImpl.hpp"
#include "IUrlCallback.hpp"

#include <GLES2/gl2platform.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h> // dlopen, dlsym

#include <android/log.h>
#include <cstdarg>

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>

PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOESEXT = 0;

// Android assets are distinguished from file paths by the "asset" scheme.
static const char* aaPrefix = "asset:///";
static const size_t aaPrefixLen = 9;

namespace Tangram {

    void Tangram::logMsg(const char *fmt, ...) {

        va_list args;
        va_start(args, fmt);
        __android_log_vprint(ANDROID_LOG_DEBUG, "Tangram", fmt, args);
        va_end(args);
    }

    std::string resolveScenePath(const char *path) {
        // If the path is an absolute URL (like a file:// or http:// URL)
        // then resolving it will return the same URL. Otherwise, we resolve
        // it against the "asset" scheme to know later that this path is in
        // the asset bundle.
        return Tangram::Url(path).resolved("asset:///").string();
    }

    void setCurrentThreadPriority(int priority) {
        int tid = gettid();
        setpriority(PRIO_PROCESS, tid, priority);
    }


    void initGLExtensions() {

        static bool glExtensionsLoaded = false;

        if (glExtensionsLoaded) {
            return;
        }

        void *libhandle = dlopen("libGLESv2.so", RTLD_LAZY);

        glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYOESPROC) dlsym(libhandle,
                                                                      "glBindVertexArrayOES");
        glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSOESPROC) dlsym(libhandle,
                                                                            "glDeleteVertexArraysOES");
        glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSOESPROC) dlsym(libhandle,
                                                                      "glGenVertexArraysOES");

        glExtensionsLoaded = true;
    }

    std::shared_ptr<Platform>
    Platform::wrapPlatform(const std::shared_ptr<PlatformTangramImpl> &tangramPlatform) {
        return std::make_shared<TangramPlatformAdapter>(tangramPlatform);
    }

}
TangramPlatformAdapter::TangramPlatformAdapter(std::shared_ptr<PlatformTangramImpl> const& tangramPlatform)
: platformImpl_(tangramPlatform){}

void TangramPlatformAdapter::requestRender() const
{
	platformImpl_->requestRender();
}

std::vector<char> TangramPlatformAdapter::bytesFromFile(const char* _path) const {
    std::vector<char> data;

    auto allocator = [&](size_t size) {
        data.resize(size);
        return data.data();
    };

    if (strncmp(_path, aaPrefix, aaPrefixLen) == 0) {
        bytesFromAssetManager(_path + aaPrefixLen, allocator);
    } else {
        Platform::bytesFromFileSystem(_path, allocator);
    }

    return data;
}

void TangramPlatformAdapter::setContinuousRendering(bool _isContinuous) {
	
	platformImpl_->setContinuousRendering(_isContinuous);	
}


std::vector<char> TangramPlatformAdapter::systemFont(const std::string& _name, const std::string& _weight, const std::string& _face) const 
{
	 std::string path = fontPath(_name, _weight, _face);

    if (path.empty()) { return {}; }

    auto data = bytesFromFile(path.c_str());

    return data;
}
using namespace Tangram;
std::vector<FontSourceHandle> TangramPlatformAdapter::systemFontFallbacksHandle() const
{
	
	std::vector<FontSourceHandle> handles;

    int importance = 0;
    int weightHint = 400;

    std::string fallbackPath = fontFallbackPath(importance, weightHint);

    while (!fallbackPath.empty()) {
        handles.emplace_back(fallbackPath);

        fallbackPath = fontFallbackPath(importance++, weightHint);
    }

    return handles;
}

struct CUrlCallback : public IUrlCallback {
    UrlCallback callback_;

    CUrlCallback(UrlCallback const &callback) : callback_(callback) { }

    void onUrlSuccess(std::vector<int8_t> const &response) override {
        callback_.operator()(std::vector<char>(response.begin(), response.end()));
    }
	
	void onUrlFailure() override {
		callback_.operator()(std::vector<char>());
	}
};

bool TangramPlatformAdapter::startUrlRequest(const std::string& _url, UrlCallback _callback)
{
	return platformImpl_->startUrlRequest(_url,  std::make_shared<CUrlCallback>(_callback));
}

void TangramPlatformAdapter::cancelUrlRequest(const std::string& _url) 
{
	platformImpl_->cancelUrlRequest(_url);
}

std::string TangramPlatformAdapter::fontPath(const std::string& _family, const std::string& _weight, const std::string& _style) const
{
	std::string key = _family + "_" + _weight + "_" + _style;

	return platformImpl_->getFontFilePath(key);
}

std::string TangramPlatformAdapter::fontFallbackPath(int _importance, int _weightHint) const
{
	return platformImpl_->getFontFallbackFilePath(_importance, _weightHint);	
}


bool TangramPlatformAdapter::bytesFromAssetManager(const char* _path, std::function<char*(size_t)> _allocator) const
{
    AAsset* asset = AAssetManager_open(assetManager, _path, AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
        logMsg("Failed to open asset at path: %s\n", _path);
        return false;
    }

    size_t size = AAsset_getLength(asset);
    unsigned char* data = reinterpret_cast<unsigned char*>(_allocator(size));

    int read = AAsset_read(asset, data, size);
    if (read <= 0) {
        logMsg("Failed to read asset at path: %s\n", _path);
    }
    AAsset_close(asset);

    return read > 0;
}