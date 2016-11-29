#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <memory>
#include <algorithm>
#include <vector>

#include "platform_tizen.h"
#include "platform_gl.h"
#include "urlWorker.h"

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#include <dlog.h>
#include <fontconfig/fontconfig.h>

#define NUM_WORKERS 4

static bool s_isContinuousRendering = false;
static std::function<void()> s_renderCallbackFunction = nullptr;

static std::vector<std::string> s_fallbackFonts {
        "/usr/share/fonts/BreezeSans-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansEthiopic-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansArabic-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansBangla-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansDevanagari-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansGujarati-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansHebrew-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansArmenian-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansJapanese-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansGeorgian-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansKhmer-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansKannada-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansKorean-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansLao-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansMalayalam-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansMyanmar-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansOdia-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansGurmukhi-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansSinhala-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansTamil-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansTelugu-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansThai-Regular.ttf",
        "/usr/share/fallback_fonts/BreezeSansChinese-Regular.ttf"};

static FcConfig* s_fcConfig = nullptr;

static UrlWorker s_workers;

bool startUrlRequest(const std::string& _url, UrlCallback _callback) {
    s_workers.enqueue(std::make_unique<UrlTask>(_url, _callback));
    return true;
}

void cancelUrlRequest(const std::string& _url) {
    s_workers.cancel(_url);
}

void initUrlRequests(const char* proxyAddress) {
    s_workers.start(4, proxyAddress);
}

void stopUrlRequests() {
    s_workers.stop();
}

static bool s_update = false;
#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "Tangram"

void logMsg(const char* fmt, ...) {

    va_list vl;
    va_start(vl, fmt);
    dlog_vprint(DLOG_WARN, LOG_TAG, fmt, vl);
    va_end(vl);
}

void setRenderCallbackFunction(std::function<void()> callback) {
    s_renderCallbackFunction = callback;
}

void setEvasGlAPI(Evas_GL_API *glApi) {
    __evas_gl_glapi = glApi;
}

void requestRender() {
    s_update = true;
    if (s_renderCallbackFunction) {
        s_renderCallbackFunction();
    }
}

bool shouldRender() {
    bool update = s_update;
    s_update = false;
    return update;
}


void setContinuousRendering(bool _isContinuous) {

    s_isContinuousRendering = _isContinuous;

}

bool isContinuousRendering() {

    return s_isContinuousRendering;

}

void initPlatformFontSetup() {

    return;

    static bool s_platformFontsInit = false;
    if (s_platformFontsInit) { return; }

    s_fcConfig = FcInitLoadConfigAndFonts();

    std::string style = "Regular";

    FcStrSet* fcLangs = FcGetLangs();
    FcStrList* fcLangList = FcStrListCreate(fcLangs);
    FcChar8* fcLang;
    while ((fcLang = FcStrListNext(fcLangList))) {
        FcValue fcStyleValue, fcLangValue;

        fcStyleValue.type = fcLangValue.type = FcType::FcTypeString;
        fcStyleValue.u.s = reinterpret_cast<const FcChar8*>(style.c_str());
        fcLangValue.u.s = fcLang;

        // create a pattern with style and family font properties
        FcPattern* pat = FcPatternCreate();

        FcPatternAdd(pat, FC_STYLE, fcStyleValue, true);
        FcPatternAdd(pat, FC_LANG, fcLangValue, true);
        //FcPatternPrint(pat);

        FcConfigSubstitute(s_fcConfig, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);

        FcResult res;
        FcPattern* font = FcFontMatch(s_fcConfig, pat, &res);
        if (font) {
            FcChar8* file = nullptr;
            if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
                // Make sure this font file is not previously added.
                if (std::find(s_fallbackFonts.begin(), s_fallbackFonts.end(),
                              reinterpret_cast<char*>(file)) == s_fallbackFonts.end()) {
                    s_fallbackFonts.emplace_back(reinterpret_cast<char*>(file));
                }
            }
            FcPatternDestroy(font);
        }
        FcPatternDestroy(pat);
    }
    FcStrListDone(fcLangList);
    s_platformFontsInit = true;
}

std::string systemFontFallbackPath(int _importance, int _weightHint) {

    if ((size_t)_importance >= s_fallbackFonts.size()) {
        return "";
    }

    return s_fallbackFonts[_importance];
}

std::string systemFontPath(const std::string& _name, const std::string& _weight,
                           const std::string& _face) {

    initPlatformFontSetup();

    if (!s_fcConfig) {
        return "";
    }

    std::string fontFile = "";
    FcValue fcFamily, fcFace, fcWeight;

    fcFamily.type = fcFace.type = fcWeight.type = FcType::FcTypeString;
    fcFamily.u.s = reinterpret_cast<const FcChar8*>(_name.c_str());
    fcWeight.u.s = reinterpret_cast<const FcChar8*>(_weight.c_str());
    fcFace.u.s = reinterpret_cast<const FcChar8*>(_face.c_str());

    // Create a pattern with family, style and weight font properties
    FcPattern* pattern = FcPatternCreate();

    FcPatternAdd(pattern, FC_FAMILY, fcFamily, true);
    FcPatternAdd(pattern, FC_STYLE, fcFace, true);
    FcPatternAdd(pattern, FC_WEIGHT, fcWeight, true);
    //FcPatternPrint(pattern);

    FcConfigSubstitute(s_fcConfig, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult res;
    FcPattern* font = FcFontMatch(s_fcConfig, pattern, &res);
    if (font) {
        FcChar8* file = nullptr;
        FcChar8* fontFamily = nullptr;
        if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch &&
            FcPatternGetString(font, FC_FAMILY, 0, &fontFamily) == FcResultMatch) {
            // We do not want the "best" match, but an "exact" or at least the same "family" match
            // We have fallbacks to cover rest here.
            if (strcmp(reinterpret_cast<const char*>(fontFamily), _name.c_str()) == 0) {
                fontFile = reinterpret_cast<const char*>(file);
            }
        }
        FcPatternDestroy(font);
    }

    FcPatternDestroy(pattern);

    return fontFile;
}

unsigned char* bytesFromFile(const char* _path, size_t& _size) {

    if (!_path || strlen(_path) == 0) { return nullptr; }

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        _size = 0;
        return nullptr;
    }

    _size = resource.tellg();

    resource.seekg(std::ifstream::beg);

    char* cdata = (char*) malloc(sizeof(char) * (_size));

    resource.read(cdata, _size);
    resource.close();

    return reinterpret_cast<unsigned char *>(cdata);
}

std::string stringFromFile(const char* _path) {

    std::string out;
    if (!_path || strlen(_path) == 0) { return out; }

    std::ifstream resource(_path, std::ifstream::ate | std::ifstream::binary);

    if(!resource.is_open()) {
        logMsg("Failed to read file at path: %s\n", _path);
        return out;
    }

    resource.seekg(0, std::ios::end);
    out.resize(resource.tellg());
    resource.seekg(0, std::ios::beg);
    resource.read(&out[0], out.size());
    resource.close();

    return out;
}

void setCurrentThreadPriority(int priority){
    int tid = syscall(SYS_gettid);
    //int  p1 = getpriority(PRIO_PROCESS, tid);

    setpriority(PRIO_PROCESS, tid, priority);

    //int  p2 = getpriority(PRIO_PROCESS, tid);
    //logMsg("set niceness: %d -> %d\n", p1, p2);
}

void initGLExtensions() {
     // glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYPROC)glfwGetProcAddress("glBindVertexArray");
     // glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSPROC)glfwGetProcAddress("glDeleteVertexArrays");
     // glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSPROC)glfwGetProcAddress("glGenVertexArrays");
}
