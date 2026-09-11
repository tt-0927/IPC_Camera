/* Static source contracts only; does not execute firmware or emulate storage failures. */
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '../../..');
const source = fs.readFileSync(path.join(root, 'control/task/sub_task/event_task.cpp'), 'utf8');
const helper = source.slice(source.indexOf('static int check_analytics_resource('),
    source.indexOf('void Task::Event::GetOrdinaryEventEnableStatus'));
const setter = source.slice(source.indexOf('void Task::Event::SetLeavePostInfo::handle()'),
    source.indexOf('void Task::Event::GetIllegalLaneChangeInfo::handle()'));
assert.match(helper, /const bool bCheckOnly = false/);
assert.match(helper, /#ifdef SCENE_INTELLIGENCE\s+case Event::Type::LEAVE_POST: already_in_target_state = \(oldStatus.bLeavePost == bEnable\); break;/);
assert.match(helper, /#ifdef SCENE_INTELLIGENCE\s+case Event::Type::LEAVE_POST: newStatus.bLeavePost = bEnable; break;/);
assert.match(helper, /newStatus = oldStatus/);
assert(helper.indexOf('if (bCheckOnly)') < helper.indexOf('set_configure(newStatus)'));
assert(helper.indexOf('ERR_EVENT_RESOURCE_CONFLICT') < helper.indexOf('if (bCheckOnly)'));
assert(setter.indexOf('rule.stRegion.IsValid()') < setter.indexOf('check_analytics_resource('));
assert(setter.indexOf('stInfo.bEnable, true)') < setter.indexOf('set_configure(stInfo)'));
assert(setter.indexOf('set_configure(stEventSchedule)') < setter.indexOf('stInfo.bEnable);'));
assert.match(setter, /nRet = CEventConfigure::instance\(\)->set_configure\(stInfo\);\s+if \(nRet != 0\)/);
assert.match(setter, /if \(nRet != 0\)\s*\{\s*const int nConfigRestore/);
assert.match(setter, /set_configure\(stPreviousSchedule\)/);
assert.match(setter, /result\(nRet\)/);
console.log('PASS: leave-post validation ordering, dry-run, status mapping, rollback and conditional guards');

const resources = fs.readFileSync(path.join(root, 'control/business/event/event_resource.cpp'), 'utf8');
const mapped = new Set([...resources.matchAll(/\{Event::Type_E::(\w+), &Event::SmartEventEnableStatus_S::/g)].map(m => m[1]));
const missing = [];
for (const match of source.matchAll(/void Task::Event::(Set\w+)::handle\(\)([\s\S]*?)(?=\nvoid |$)/g)) {
    const event = match[2].match(/stEventSchedule.enEventType = ::Event::Type_E::(\w+)/);
    if (event && mapped.has(event[1]) && !match[2].includes('check_analytics_resource(') &&
        !match[2].includes('SmartEventEnableStatus_S')) {
        missing.push(`${event[1]}: ${match[1]}`);
    }
}
console.log(`AUDIT: ${missing.length} mapped event setters lack overview-status synchronization`);
console.log(missing.join('\n'));
assert.equal(missing.length, 0);

const mappings = new Map([...resources.matchAll(/\{Event::Type_E::(\w+), &Event::SmartEventEnableStatus_S::(\w+)\}/g)]
    .map(match => [match[1], match[2]]));
const shared = source.slice(source.indexOf('static int save_scene_event_config('),
    source.indexOf('/* 获取普通事件启用状态 */'));
assert(shared.indexOf('stInfo.bEnable, true)') < shared.indexOf('set_configure(stInfo)'));
assert(shared.indexOf('set_configure(stEventSchedule)') < shared.indexOf('stInfo.bEnable, false, true)'));
assert.match(shared, /set_configure\(stPrevious\)/);
assert.match(shared, /set_configure\(stPreviousSchedule\)/);
assert.match(helper, /if \(!bEnable && !bSyncOnly\)/);
const calls = [...source.matchAll(/result\(save_scene_event_config\(::Event::Type_E::(\w+), stInfo\)\)/g)];
assert.equal(calls.length, 26);
assert.equal(new Set(calls.map(match => match[1])).size, 26);
for (const [, event] of calls) {
    const field = mappings.get(event);
    assert(field, `Missing resource mapping: ${event}`);
    assert(helper.includes(`case Event::Type::${event}: already_in_target_state = (oldStatus.${field} == bEnable);`));
    assert(helper.includes(`case Event::Type::${event}: newStatus.${field} = bEnable;`));
}
const garbageBlock = /#if defined\(SCENE_INTELLIGENCE\) \|\| CAP_AI_GARBAGE_DETECT\s+case Event::Type::GARBAGE_EXPOSURE:[\s\S]*?#endif/g;
assert.equal([...helper.matchAll(garbageBlock)].length, 2);
console.log('PASS: all 26 scene setters mapped; shared save ordering, rollback and garbage-only guards');
