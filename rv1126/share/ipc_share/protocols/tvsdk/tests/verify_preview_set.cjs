'use strict';
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const cp = require('node:child_process');
const root = path.resolve(__dirname, '../../..');
function extract(file, signature) {
    const source = fs.readFileSync(path.join(root, file), 'utf8');
    const start = source.indexOf(signature);
    if (start < 0) throw new Error(`Missing ${signature}`);
    const brace = source.indexOf('{', start);
    let depth = 1;
    let end = brace + 1;
    for (; depth && end < source.length; ++end) {
        if (source[end] === '{') ++depth;
        if (source[end] === '}') --depth;
    }
    if (depth) throw new Error('Unbalanced function');
    return source.slice(start, end);
}
const setter = extract('control/business/preview/preview_manage.cpp',
    'int CPreviewManage::set_preview_info(');
const callback = extract('protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp',
    'static NET_COMMON_ECODE_E cb_set_preview_info(');
const harness = `
#include <cassert>
#include <string>
namespace ISP { struct ImageParam_S {
    unsigned nBrightness = 0, nContrast = 0, nSaturation = 0, nSharpness = 0;
}; }
namespace Preview { struct PreviewInfo_S { ISP::ImageParam_S stImageParam; }; }
static int g_nIspResult = 0;
static ISP::ImageParam_S g_stApplied = {};
class CIspManage {
public:
    static CIspManage* instance() { static CIspManage stManager; return &stManager; }
    int set_image_config(const ISP::ImageParam_S& stParam) {
        g_stApplied = stParam; return g_nIspResult;
    }
};
class CPreviewManage { public: int set_preview_info(Preview::PreviewInfo_S stInfo); };
${setter}
using INT32 = int;
using LPVOID = void*;
using NET_COMMON_ECODE_E = int;
using NET_PreviewInfo_S = Preview::PreviewInfo_S;
constexpr int NET_E_SUCCEED = 0, NET_E_INVALID_PARAM = 102, NET_E_SET_CFG_FAILED = 103;
constexpr int ERR_WEB_PARAM = -100, AC_SET_PREVIEW_INFO = 225;
static int g_nTransportResult = 0;
static std::string g_strResult = "0";
namespace TvSdkConvert {
    static void ToPreviewInfo(const NET_PreviewInfo_S& stIn, Preview::PreviewInfo_S& stOut) { stOut = stIn; }
}
namespace Convert { static std::string to_string(const Preview::PreviewInfo_S&) { return "config"; } }
static std::string wrap_data_json(const std::string& strIn) { return "wrapped:" + strIn; }
static int execute_get_result(int nAction, const std::string& strIn, std::string& strOut) {
    assert(nAction == 225 && strIn == "wrapped:config");
    strOut = g_strResult; return g_nTransportResult;
}
namespace Json {
    static void get(const char* pJson, const char*, int& nOut) {
        if (std::string(pJson) != "missing") { nOut = std::stoi(pJson); }
    }
}
${callback}
int main() {
    CPreviewManage stManager;
    Preview::PreviewInfo_S stInput = {{40, 51, 62, 73}};
    assert(stManager.set_preview_info(stInput) == 0);
    assert(g_stApplied.nBrightness == 40 && g_stApplied.nContrast == 51);
    assert(g_stApplied.nSaturation == 62 && g_stApplied.nSharpness == 73);
    g_nIspResult = -7;
    assert(stManager.set_preview_info(stInput) == -7);
    assert(cb_set_preview_info(1, nullptr) == NET_E_INVALID_PARAM);
    assert(cb_set_preview_info(1, &stInput) == NET_E_SUCCEED);
    g_strResult = "-100";
    assert(cb_set_preview_info(1, &stInput) == NET_E_INVALID_PARAM);
    for (const auto& strResult : {"-7", "missing", ""}) {
        g_strResult = strResult;
        assert(cb_set_preview_info(1, &stInput) == NET_E_SET_CFG_FAILED);
    }
    g_strResult = "0"; g_nTransportResult = -1;
    assert(cb_set_preview_info(1, &stInput) == NET_E_SET_CFG_FAILED);
}
`;
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'preview-set-'));
try {
    const source = path.join(temp, 'test.cpp');
    const executable = path.join(temp, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(source, harness);
    const compiler = process.env.CXX || (process.platform === 'win32' ? 'C:/Strawberry/c/bin/g++.exe' : 'g++');
    cp.execFileSync(compiler, ['-std=c++17', '-Wall', '-Wextra', '-Werror', source, '-o', executable], {stdio: 'inherit'});
    cp.execFileSync(executable, [], {stdio: 'inherit'});
    console.log('PASS: preview ISP parameter forwarding and SDK business error mapping');
} finally {
    fs.rmSync(temp, {recursive: true, force: true});
}
