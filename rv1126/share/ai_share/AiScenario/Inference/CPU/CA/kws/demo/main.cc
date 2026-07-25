
#include "alsa.h"
#include "KWS.hpp"

bool stop = false;

static void Handler(int sig)
{
    stop = true;
    fprintf(stderr, "\nCaught Ctrl + C. Exiting...\n");
}

int main(int32_t argc, char* argv[])
{

    InferenceV1_0_NS::CCAInferenceBase* pBase = new InferenceV1_0_NS::CKWS(
        "/opt/bl/model/VoiceWakeUp/tokens.txt",
        "/opt/bl/model/VoiceWakeUp/encoder.onnx",
        "/opt/bl/model/VoiceWakeUp/decoder.onnx",
        "/opt/bl/model/VoiceWakeUp/joiner.onnx",
        "/opt/bl/model/VoiceWakeUp/keywords.txt");
    pBase->init();

    signal(SIGINT, Handler);

    std::string       device_name = "plughw:1,0";
    sherpa_onnx::Alsa alsa(device_name.c_str());
    fprintf(stderr, "Use recording device: %s\n", device_name.c_str());

    int32_t expected_sample_rate = 48000;
    if (alsa.GetExpectedSampleRate() != expected_sample_rate)
    {
        fprintf(stderr, "sample rate: %d != %d\n", alsa.GetExpectedSampleRate(),
                expected_sample_rate);
        exit(-1);
    }

    int32_t chunk = 0.1 * alsa.GetExpectedSampleRate();
    while (!stop)
    {
        std::string               sKeyWord;
        const std::vector<float>& samples = alsa.Read(chunk);
        printf("111111[%d]-[%f]\n", samples.size(), samples[10]);
        AiScenario_NS::CAData_S stInData;
        stInData.pData     = (int8_t*)samples.data();
        stInData.nDataSize = samples.size();
        stInData.nSample   = expected_sample_rate;

        pBase->inference(stInData, sKeyWord);
        if (!sKeyWord.empty())
        {
            printf("======== result: %s\n", sKeyWord.c_str());
        }
    }

    delete pBase;
    pBase = nullptr;

    return 0;
}
