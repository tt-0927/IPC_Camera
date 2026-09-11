/* Compile the production save helper with fake configuration/resource dependencies. */
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const cp = require('node:child_process');
const root = path.resolve(__dirname, '../../..');
const source = fs.readFileSync(path.join(root, 'control/task/sub_task/event_task.cpp'), 'utf8');
const start = source.indexOf('template <typename TConfig>\nstatic int save_scene_event_config(');
if (start < 0) throw new Error('Save helper not found');
const helper = source.slice(start, source.indexOf('\n#endif', start));
const harness = `
#include <cassert>
#include <vector>
namespace Event { enum class Type_E { SAMPLE }; }
struct Config { bool bEnable = false; std::vector<int> aAlarmTime{1}; };
namespace Alarm { struct EventSchedule_S {
    Event::Type_E enEventType = Event::Type_E::SAMPLE;
    bool bStatus = false; std::vector<int> defenseTime{1};
}; }
static int failStage = 0, refreshes = 0, writes = 0;
static bool overview = false, unrelated = true;
class CEventConfigure {
public:
    Config data; Alarm::EventSchedule_S plan;
    static CEventConfigure* instance() { static CEventConfigure value; return &value; }
    int get_configure(Config& value) { value = data; return failStage == 2 ? -2 : 0; }
    int get_configure(Alarm::EventSchedule_S& value) { value = plan; return 0; }
    int set_configure(const Config& value) {
        ++writes;
        if (failStage == 3) { failStage = 0; return -3; }
        data = value; return 0;
    }
    int set_configure(const Alarm::EventSchedule_S& value) {
        ++writes;
        if (failStage == 4) { failStage = 0; return -4; }
        plan = value; return 0;
    }
};
class CEventManage {
public:
    static CEventManage* instance() { static CEventManage value; return &value; }
    void update_event_schedule() { ++refreshes; }
};
static int check_analytics_resource(Event::Type_E, bool enabled, bool checkOnly, bool syncOnly = false) {
    if (checkOnly) return failStage == 1 ? -1 : 0;
    assert(syncOnly);
    if (failStage == 5) return -5;
    overview = enabled; return 0;
}
#define dlog_error(...) ((void)0)
${helper}
int main() {
    auto* storage = CEventConfigure::instance();
    Config desired; desired.bEnable = true; desired.aAlarmTime = {4, 14};
    for (int stage = 0; stage <= 5; ++stage) {
        storage->data = Config{}; storage->plan = Alarm::EventSchedule_S{};
        overview = false; writes = 0; refreshes = 0; failStage = stage;
        const int result = save_scene_event_config(Event::Type_E::SAMPLE, desired);
        assert(unrelated);
        if (stage == 0) {
            assert(result == 0 && overview && storage->data.bEnable && storage->plan.bStatus);
            assert(storage->plan.defenseTime == desired.aAlarmTime);
            desired.bEnable = false;
            assert(save_scene_event_config(Event::Type_E::SAMPLE, desired) == 0);
            assert(!overview && !storage->data.bEnable && !storage->plan.bStatus);
            desired.bEnable = true;
        } else {
            assert(result == -stage && !overview && !storage->data.bEnable && !storage->plan.bStatus);
            assert(storage->data.aAlarmTime == std::vector<int>{1});
            if (stage < 3) assert(writes == 0 && refreshes == 0);
        }
    }
}
`;
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'ipc-scene-save-'));
const input = path.join(directory, 'test.cpp');
const output = path.join(directory, process.platform === 'win32' ? 'test.exe' : 'test');
fs.writeFileSync(input, harness);
cp.execFileSync(process.env.CXX || 'g++', ['-std=c++17', input, '-o', output], { stdio: 'inherit' });
cp.execFileSync(output, [], { stdio: 'inherit' });
console.log('PASS: production save helper enable/disable, resource/read/write/sync failures and rollback (fake dependencies)');
console.log('Build artifacts: ' + directory);
