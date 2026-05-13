# 版本限定
cmake_minimum_required(VERSION 3.10.2)

set(MODULE_NAME "CpuCAInferenceModel_KWS")


# 语音唤醒的功能代码
set(sources
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/base64-decode.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/cat.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/context-graph.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/features.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/file-utils.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/hypothesis.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/keyword-spotter-impl.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/keyword-spotter.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/online-model-config.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/online-stream.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/online-transducer-decoder.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/online-transducer-model-config.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/online-transducer-model.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/online-zipformer2-transducer-model.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/onnx-utils.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/parse-options.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/provider.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/resample.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/session.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/symbol-table.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/text-utils.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/transducer-keyword-decoder.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/unbind.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kws/utils.cc

    ${CMAKE_CURRENT_LIST_DIR}/KWS.cpp
)

# 第三方库
set(fbank_sources
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/feature-fbank.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/feature-functions.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/feature-window.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/fftsg.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/kaldi-math.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/mel-computations.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/online-feature.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/rfft.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-native-fbank/csrc/whisper-feature.cc
)
set(decoder_sources
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-decoder/csrc/decodable-ctc.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-decoder/csrc/eigen.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-decoder/csrc/faster-decoder.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-decoder/csrc/lattice-faster-decoder.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-decoder/csrc/lattice-simple-decoder.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/kaldi-decoder/csrc/simple-decoder.cc
)
set(fst_sources
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/compat.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/flags.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/fst.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/properties.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/symbol-table.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/util.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/symbol-table-ops.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/mapped-file.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/weight.cc
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/fst/${HEADER_FILES}
)
# 定义一个静态库
add_library(${MODULE_NAME} STATIC)

# 设置的源文件
target_sources(${MODULE_NAME} PRIVATE ${sources} ${fbank_sources} ${decoder_sources} ${fst_sources})

target_include_directories(${MODULE_NAME} PUBLIC 
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../
    ${CMAKE_CURRENT_LIST_DIR}/../../../
    ${CMAKE_CURRENT_LIST_DIR}/../../Common/
    ${CMAKE_CURRENT_LIST_DIR}/../../../../
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/ 
    ${CMAKE_CURRENT_LIST_DIR}/kws_source/onnxruntime/include)


target_link_directories(${MODULE_NAME} PUBLIC 
${CMAKE_CURRENT_LIST_DIR}/kws_source/onnxruntime/lib)

target_link_libraries(${MODULE_NAME}
  -lonnxruntime
  -pthread 
  -ldl 
  -lm 
)

message("==========> [" ${PROJECT_NAME} "] 使用[" ${MODULE_NAME} "]模块 <==========")

