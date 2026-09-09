/* Static contracts only: this script does not compile or execute IPC firmware. */
'use strict';
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');

const root = path.resolve(__dirname, '../../..');
const read = relative => fs.readFileSync(path.join(root, relative), 'utf8');
const stripComments = text => text.replace(
    /("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|\/\*[\s\S]*?\*\/|\/\/[^\r\n]*/g,
    (match, literal) => literal || '');
const callbacks = stripComments(read('protocols/tvsdk/src/callbacks/tvsdk_callbacks.cpp'));
const conversions = stripComments(read('protocols/tvsdk/src/convert/tvsdk_convert.cpp'));
const json = stripComments(read('common/convert/alarm_convert.cpp'));
const definitions = stripComments(read('common/define/alarm_define.h'));
const sdkHeader = stripComments(read('protocols/tvsdk/include/NetTVSDKServer.h'));
const convertHeader = stripComments(read('protocols/tvsdk/src/convert/tvsdk_convert.h'));
const events = [
  {
    "suffix": "cross_line_alarm",
    "sdk": "NET_CrossLineAlarmInfo_S",
    "ipc": "BoundaryDetection_S",
    "ipcRule": "BoundaryPlane_S",
    "converter": "ToBoundaryDetection",
    "fill": "FillCrossLineAlarmInfo",
    "action": "AC_SET_LINE_CROSSING_DETECT_INFO",
    "get": "cb_get_cross_line_alarm"
  },
  {
    "suffix": "intrusion_alarm",
    "sdk": "NET_IntrusionAlarmInfo_S",
    "ipc": "FieldDetection_S",
    "ipcRule": "Intrusion_S",
    "converter": "ToFieldDetection",
    "fill": "FillIntrusionAlarmInfo",
    "action": "AC_SET_REGIONAL_INTRUSION_DETECT_INFO",
    "get": "cb_get_intrusion_alarm"
  },
  {
    "suffix": "loitering_alarm",
    "sdk": "NET_LoiteringAlarmInfo_S",
    "ipc": "LoiteringDetection_S",
    "ipcRule": "LoiteringRule_S",
    "converter": "ToLoiteringDetection",
    "fill": "FillLoiteringAlarmInfo",
    "action": "AC_SET_LOITERING_DETECT_INFO",
    "get": "cb_get_loitering_alarm"
  },
  {
    "suffix": "crowd_gathering_alarm",
    "sdk": "NET_CrowdGatheringAlarmInfo_S",
    "ipc": "CrowdGathering_S",
    "ipcRule": "CrowdGatheringRule_S",
    "converter": "ToCrowdGathering",
    "fill": "FillCrowdGatheringAlarmInfo",
    "action": "AC_SET_CROWD_GATHERING_DETECT_INFO",
    "get": "cb_get_crowd_gathering_alarm"
  },
  {
    "suffix": "enter_region_alarm",
    "sdk": "NET_EnterRegionAlarmInfo_S",
    "ipc": "EntranceDetection_S",
    "ipcRule": "EnterExitIntrusion_S",
    "converter": "ToEntranceDetection",
    "fill": "FillEnterRegionAlarmInfo",
    "action": "AC_SET_ENTER_REGION_DETECT_INFO",
    "get": "cb_get_enter_region_alarm"
  },
  {
    "suffix": "leave_region_alarm",
    "sdk": "NET_LeaveRegionAlarmInfo_S",
    "ipc": "ExitingDetection_S",
    "ipcRule": "EnterExitIntrusion_S",
    "converter": "ToExitingDetection",
    "fill": "FillLeaveRegionAlarmInfo",
    "action": "AC_SET_LEAVE_REGION_DETECT_INFO",
    "get": "cb_get_leave_region_alarm"
  },
  {
    "suffix": "climb_fence_info",
    "sdk": "NET_ClimbFenceInfo_S",
    "ipc": "FenceClimbingDetection_S",
    "ipcRule": "FenceClimbingRule_S",
    "converter": "ToClimbFence",
    "fill": "FillClimbFenceInfo",
    "action": "AC_SET_CLIMB_FENCE_INFO",
    "get": "cb_get_climb_fence_info"
  },
  {
    "suffix": "dimission_info",
    "sdk": "NET_DimissionInfo_S",
    "ipc": "LeavePostDetection_S",
    "ipcRule": "LeavePostRule_S",
    "converter": "ToDimission",
    "fill": "FillDimissionInfo",
    "action": "AC_SET_DIMISSION_INFO",
    "get": "cb_get_dimission_info"
  },
  {
    "suffix": "illegal_lane_info",
    "sdk": "NET_IllegalLaneInfo_S",
    "ipc": "IllegalLaneChangeDetection_S",
    "ipcRule": "IllegalLaneChangeRule_S",
    "converter": "ToIllegalLane",
    "fill": "FillIllegalLaneInfo",
    "action": "AC_SET_ILLEGAL_LANE_INFO",
    "get": "cb_get_illegal_lane_info"
  },
  {
    "suffix": "retrograde_info",
    "sdk": "NET_RetrogradeInfo_S",
    "ipc": "DrivingAgainstTrafficDetection_S",
    "ipcRule": "DrivingAgainstTrafficRule_S",
    "converter": "ToRetrograde",
    "fill": "FillRetrogradeInfo",
    "action": "AC_SET_RETROGRADE_INFO",
    "get": "cb_get_retrograde_info"
  },
  {
    "suffix": "nonmotor_vehicle_intrusion_info",
    "sdk": "NET_NonmotorVehicleIntrusionInfo_S",
    "ipc": "NonMotorVehicleIntrusionDetection_S",
    "ipcRule": "NonMotorVehicleIntrusionRule_S",
    "converter": "ToNonmotorVehicleIntrusion",
    "fill": "FillNonmotorVehicleIntrusionInfo",
    "action": "AC_SET_NONMOROT_VEHIINTRU_INFO",
    "get": "cb_get_nonmotor_vehicle_intrusion_info"
  },
  {
    "suffix": "occupation_emergency_info",
    "sdk": "NET_OccupationEmergencyInfo_S",
    "ipc": "EmergencyLaneOccupancyDetection_S",
    "ipcRule": "EmergencyLaneOccupancyRule_S",
    "converter": "ToOccupationEmergency",
    "fill": "FillOccupationEmergencyInfo",
    "action": "AC_SET_OCCUPATION_EMERGENCY_INFO",
    "get": "cb_get_occupation_emergency_info"
  },
  {
    "suffix": "pedestrian_intrusion_info",
    "sdk": "NET_PedestrianIntrusionInfo_S",
    "ipc": "PedestrianIntrusionDetection_S",
    "ipcRule": "PedestrianIntrusionRule_S",
    "converter": "ToPedestrianIntrusion",
    "fill": "FillPedestrianIntrusionInfo",
    "action": "AC_SET_PEDESTRAN_INTRUSION_INFO",
    "get": "cb_get_pedestrian_intrusion_info"
  },
  {
    "suffix": "parking_detect_alarm",
    "sdk": "NET_ParkingAlarmInfo_S",
    "ipc": "ParkingDetection_S",
    "ipcRule": "ParkingRule_S",
    "converter": "ToParkingDetection",
    "fill": "FillParkingDetectAlarmInfo",
    "action": "AC_SET_PARKING_DETECT_INFO",
    "get": "cb_get_parking_detect_alarm"
  },
  {
    "suffix": "unattended_object_alarm",
    "sdk": "NET_UnattendedObjectAlarmInfo_S",
    "ipc": "UnattendedObject_S",
    "ipcRule": "UnattendedObjectRule_S",
    "converter": "ToUnattendedObject",
    "fill": "FillUnattendedObjectAlarmInfo",
    "action": "AC_SET_UNATTENDED_OBJECT_DETECT_INFO",
    "get": "cb_get_unattended_object_alarm"
  },
  {
    "suffix": "object_removal_alarm",
    "sdk": "NET_ObjectRemovalAlarmInfo_S",
    "ipc": "ObjectRemoval_S",
    "ipcRule": "ObjectRemovalRule_S",
    "converter": "ToObjectRemoval",
    "fill": "FillObjectRemovalAlarmInfo",
    "action": "AC_SET_OBJECT_REMOVAL_DETECT_INFO",
    "get": "cb_get_object_removal_alarm"
  }
];

let checks = 0;
function verify(condition, label) {
    assert.ok(condition, label);
    ++checks;
}
function escape(text) {
    return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}
function block(source, start) {
    const begin = source.indexOf('{', start);
    assert.ok(begin >= 0);
    let depth = 0;
    for (let end = begin; end < source.length; ++end) {
        if (source[end] === '{') ++depth;
        if (source[end] === '}' && --depth === 0) {
            return source.slice(start, end + 1);
        }
    }
    throw new Error('Unbalanced block');
}
function func(source, name, signaturePart = '') {
    const expression = new RegExp('^[ \\t]*(?:static )?(?:[\\w:<>*&]+\\s+)+' +
        '(?:TvSdkConvert::)?' + escape(name) + '\\s*\\([^;{]*\\)[^{;]*\\{', 'gm');
    for (const match of source.matchAll(expression)) {
        if (match[0].includes(signaturePart)) return block(source, match.index);
    }
    throw new Error('Missing function: ' + name + ' ' + signaturePart);
}
function structure(source, alias) {
    const match = new RegExp('}\\s*' + escape(alias) + ';').exec(source);
    const end = match ? match.index : -1;
    assert.ok(end >= 0, alias);
    const start = source.lastIndexOf('typedef struct', end);
    assert.ok(start >= 0, alias);
    return source.slice(start, end);
}

for (const event of events) {
    const set = func(callbacks, 'cb_set_' + event.suffix);
    const get = func(callbacks, event.get);
    const toIpc = func(conversions, event.converter);
    const toSdk = func(conversions, event.fill);
    const config = structure(sdkHeader, event.sdk);
    const ruleArray = config.match(/(NET_\w+_S)\s+(stRule|astRule)\[(\d+)\]/);
    verify(ruleArray, event.suffix + ': fixed-size SDK rule array');
    verify(set.includes('pInBuffer == nullptr'), event.suffix + ': null input rejected');
    verify(set.includes('const ' + event.sdk + ' *pConfig'), event.suffix + ': input type');
    verify(set.includes(event.sdk + ' stNormalized = *pConfig'), event.suffix + ': private request copy');
    verify(new RegExp('tvsdk_preserve_event_rules\\(\\s*nChannelId, stNormalized, ' +
        event.action + ', ' + event.get + '\\)').test(set), event.suffix + ': correct normalizer and GET');
    verify(set.includes('Alarm::' + event.ipc + ' stConfig = {}'), event.suffix + ': IPC output type');
    verify(set.includes('TvSdkConvert::' + event.converter + '(stNormalized, stConfig)'),
        event.suffix + ': only normalized data converted');
    verify(set.includes('return tvsdk_set_event_config(' + event.action + ','),
        event.suffix + ': actual task result returned');
    verify(!set.includes('s_taskManage->execute('), event.suffix + ': no dispatch-only success');
    verify(!/\.bEnable\s*=|\buRuleCount\s*=/.test(set), event.suffix + ': preserve enable and count');
    verify(get.includes('Alarm::' + event.ipc) && get.includes('TvSdkConvert::' + event.fill + '('),
        event.suffix + ': matching GET conversion');
    verify(toIpc.includes('const ' + event.sdk + ' &src, Alarm::' + event.ipc + ' &dst'),
        event.suffix + ': conversion implementation signature');
    verify(convertHeader.includes(event.converter + '(const ' + event.sdk + ' &src, Alarm::' + event.ipc + ' &dst)'),
        event.suffix + ': conversion declaration signature');
    verify(toIpc.includes('dst.aRule.push_back(out)'), event.suffix + ': save rule slots');
    verify(!/\b(?:r|out)\.bEnable/.test(toIpc), event.suffix + ': ignore per-rule enable');
    verify(toIpc.includes('ToLinkageList(src.stLinkageList, dst.stLinkageList)') &&
        toSdk.includes('FillLinkageList(src.stLinkageList, dst.stLinkageList)'),
        event.suffix + ': bidirectional linkage');
    verify(toIpc.includes('dst.aAlarmTime') && toSdk.includes('src.aAlarmTime'),
        event.suffix + ': schedule mapping');
    verify(toSdk.includes('dst.uRuleCount++'), event.suffix + ': count all GET slots');
    const rule = structure(definitions, event.ipcRule);
    const ruleJson = func(json, 'Convert::deal', 'Alarm::' + event.ipcRule + ' &stInfo');
    const fields = [...new Set([...toIpc.matchAll(/\bout\.(\w+)/g)].map(match => match[1]))];
    for (const field of fields) {
        verify(ruleJson.includes('stInfo.' + field), event.suffix + ': JSON field ' + field);
        verify(new RegExp(escape(field) + '\\s*=\\s*(?:x|stSource)\\.' + escape(field)).test(rule),
            event.suffix + ': copy field ' + field);
    }
    console.log('PASS ' + event.suffix);
}
const normalize = func(callbacks, 'tvsdk_preserve_event_rules');
verify(/stConfig.uRuleCount < 0 \|\| stConfig.uRuleCount > nMaxRules/.test(normalize),
    'Reject unsupported counts before reading rules');
verify(normalize.indexOf('return NET_E_INVALID_PARAM') < normalize.indexOf('for (INT32 nIndex'),
    'Count rejection precedes rule traversal');
verify(/std::min\(TVSDK_IPC_RULE_MAX, nCapacity\)/.test(normalize), 'Respect IPC and array capacity');
verify(/TVSDK_IPC_RULE_MAX = 4;/.test(callbacks), 'IPC capacity stays four, including parking');
verify(!/\buRuleCount\s*=|\.bEnable\s*=|erase\(|resize\(/.test(normalize),
    'Normalizer never changes count, enable or slot positions');
verify(normalize.indexOf('tvsdk_valid_event_rule(aRules[nIndex]') <
    normalize.indexOf('if (!bPreviousLoaded)'), 'Valid requests do not need old configuration');
verify(/if \(!bPreviousLoaded\)/.test(normalize) && /bPreviousLoaded = true;/.test(normalize),
    'Load previous configuration at most once');
verify(/fnGetConfig\(nChannelId, &stPrevious\) != NET_E_SUCCEED/.test(normalize),
    'Read failure aborts before SET');
verify(/nIndex < stPrevious.uRuleCount && tvsdk_valid_event_rule\(aPreviousRules\[nIndex\]/.test(normalize),
    'Old slot must exist and be valid before reuse');
verify(normalize.includes('aRules[nIndex] = aPreviousRules[nIndex]'),
    'Same-index fallback without compaction');
verify(normalize.includes('tvsdk_default_event_rule(aRules[nIndex], nActionCode)'),
    'Missing or invalid old slot uses default');
verify(!callbacks.includes('compact_region_rules') && !callbacks.includes('is_empty_region_rule'),
    'Obsolete per-rule-enable filtering removed');
const polygon = func(callbacks, 'tvsdk_valid_polygon');
verify(polygon.indexOf('stRule.uPointCount > nCapacity') < polygon.indexOf('stRule.afPointX[nIndex]'),
    'Check point capacity before indexing');
verify(polygon.includes('std::isfinite') && polygon.includes('stPoint.IsValid()') &&
    polygon.includes('return bHasPoint'), 'Validate finite pixel coordinates and reject zero-only polygons');
const targets = func(callbacks, 'tvsdk_valid_rule_targets');
verify(targets.includes('stRule.uDetectionTargetCount > nCapacity') &&
    targets.includes('NET_TARGET_ALL') && targets.includes('NET_TARGET_OTHER'),
    'Validate target capacity and enumeration');
const defaults = func(callbacks, 'tvsdk_default_event_rule', 'TRule &stRule');
verify(defaults.includes('stRule = {}') && defaults.includes('TVSDK_EMPTY_REGION_POINTS') &&
    defaults.includes('tvsdk_rule_min_time(nActionCode)'), 'Empty polygon defaults meet event thresholds');
verify(/TVSDK_EMPTY_REGION_POINTS = 4;/.test(callbacks), 'Empty polygon has four zero points');
const minTime = func(callbacks, 'tvsdk_rule_min_time');
verify(minTime.includes('AC_SET_UNATTENDED_OBJECT_DETECT_INFO') &&
    minTime.includes('AC_SET_OBJECT_REMOVAL_DETECT_INFO') && minTime.includes('TVSDK_OBJECT_MIN_TIME = 12'),
    'Object-event minimum remains twelve seconds');
const retrograde = func(callbacks, 'cb_set_retrograde_info');
verify(retrograde.includes('stRule.enCrossDirection != Alarm::A_TO_B') &&
    retrograde.includes('stRule.enCrossDirection != Alarm::B_TO_A') &&
    retrograde.includes('return NET_E_INVALID_PARAM'), 'Nonempty reverse-direction rule rejects two-way mode');
for (const type of ['EnterExitIntrusion_S', 'FenceClimbingRule_S']) {
    verify(/nTimeThreshold\(10\)/.test(structure(definitions, type)),
        type + ': initialize existing time field for legacy configurations');
}
for (const alias of ['NET_CrowdGatheringAlarmInfo_S', 'NET_ParkingAlarmInfo_S']) {
    verify(func(callbacks, 'tvsdk_rule_array', alias + ' &stConfig').includes('return stConfig.astRule;'),
        alias + ': correct array accessor overload');
}
console.log('PASS ' + checks + ' static contracts across ' + events.length + ' event paths.');
console.log('Target compilation, firmware execution and browser round-trip are not performed.');
