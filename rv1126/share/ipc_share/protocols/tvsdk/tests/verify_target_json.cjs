'use strict';
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const cp = require('node:child_process');
const root = path.resolve(__dirname, '../../../../../..');
const source = fs.readFileSync(path.join(root, 'SDK/af_sdk/sdk_share/tools/convert/BG6_ZHSJ/BU_SJCL/AlarmInfoConvert.cpp'), 'utf8');
const start = source.indexOf('static void convert_detection_targets(');
const end = source.indexOf('\nvoid SDKConvert::deal', start);
if (start < 0 || end < 0) throw new Error('Missing target conversion');
const harness = `
#include <algorithm>
#include <cassert>
#include <vector>
#include <string>
using INT32 = int;
enum { NET_TARGET_ALL, NET_TARGET_HUMAN, NET_TARGET_VEHICLE, NET_TARGET_OTHER };
namespace Json {
struct Object { int count = 0; std::vector<Object> items; };
Object* get(Object* p, const char*) { return p; }
namespace Array {
int size(Object* p) { return int(p->items.size()); }
Object* get(Object* p, int i) { return &p->items.at(i); }
}
namespace Value { void get(Object* p, int& value) { value = p->count; } }
}
namespace SDKConvert { class CSDKConvert {
bool input;
public:
explicit CSDKConvert(bool value): input(value) {}
void field(Json::Object* p, const char*, int& value) {
    if (input) value = p->count; else p->count = value;
}
void field_array(Json::Object* p, const char*, int* values, unsigned count, unsigned capacity) {
    if (input) {
        for (unsigned i = 0; i < p->items.size() && i < capacity; ++i) values[i] = p->items[i].count;
    } else {
        p->items.clear();
        for (unsigned i = 0; i < count && i < capacity; ++i) p->items.push_back({values[i], {}});
    }
}
}; }
${source.slice(start, end)}
int main() {
    for (int requestedCount : {1, 3}) {
        Json::Object json{requestedCount, {{0, {}}}};
        int count = 0, targets[8] = {9,9,9,9,9,9,9,9};
        convert_detection_targets(&json, count, targets, true);
        assert(count == 3 && targets[0] == 1 && targets[1] == 2 && targets[2] == 3);
        convert_detection_targets(&json, count, targets, false);
        assert(json.count == 3 && json.items.size() == 1 && json.items[0].count == 0);
    }
    for (int count : {0, 1, 2}) {
        int targets[8] = {1, 3};
        Json::Object json;
        convert_detection_targets(&json, count, targets, false);
        assert(json.count == count && int(json.items.size()) == count);
        if (count == 2) assert(json.items[1].count == 3);
    }
    int count = 1, targets[8] = {};
    Json::Object json;
    convert_detection_targets(&json, count, targets, false);
    assert(json.count == 3 && json.items.size() == 1 && json.items[0].count == 0);
}
`;
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'target-json-'));
try {
    const cpp = path.join(temp, 'test.cpp');
    const exe = path.join(temp, process.platform === 'win32' ? 'test.exe' : 'test');
    fs.writeFileSync(cpp, harness);
    const compiler = process.env.CXX || (process.platform === 'win32' ? 'C:/Strawberry/c/bin/g++.exe' : 'g++');
    cp.execFileSync(compiler, ['-std=c++17', '-Wall', '-Wextra', '-Werror', cpp, '-o', exe], {stdio: 'inherit'});
    cp.execFileSync(exe, [], {stdio: 'inherit'});
    console.log('PASS: all-target JSON count, compact array, input expansion, partial and empty selections');
} finally {
    fs.rmSync(temp, {recursive: true, force: true});
}
