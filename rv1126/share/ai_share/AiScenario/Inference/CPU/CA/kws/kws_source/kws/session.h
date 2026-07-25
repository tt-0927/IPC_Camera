// kws/session.h
//
// Copyright (c)  2023  Xiaomi Corporation

#ifndef SHERPA_ONNX_CSRC_SESSION_H_
#define SHERPA_ONNX_CSRC_SESSION_H_

#include "onnxruntime_cxx_api.h"  // NOLINT
#include "kws/online-model-config.h"


namespace sherpa_onnx {
Ort::SessionOptions GetSessionOptions(const OnlineModelConfig &config);
}  // namespace sherpa_onnx

#endif  // SHERPA_ONNX_CSRC_SESSION_H_
