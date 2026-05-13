// kws/keyword-spotter-impl.cc
//
// Copyright (c)  2023-2024  Xiaomi Corporation

#include "kws/keyword-spotter-impl.h"
#include "kws/keyword-spotter-transducer-impl.h"

namespace sherpa_onnx {

std::unique_ptr<KeywordSpotterImpl> KeywordSpotterImpl::Create(
    const KeywordSpotterConfig &config) {
  if (!config.model_config.transducer.encoder.empty()) {
    return std::make_unique<KeywordSpotterTransducerImpl>(config);
  }

  SHERPA_ONNX_LOGE("Please specify a model");
  exit(-1);
}


}  // namespace sherpa_onnx
