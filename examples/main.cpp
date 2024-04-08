#include <xaudio2Redist.h>
#include <audio_buffer/audio_buffer.h>

#define BUFFER_SIZE 4096

void throw_if_failed(HRESULT res) {
    if(FAILED(res)) {
        throw res;
    }
}


int main() {
    CoInitialize(NULL);
    IXAudio2* engine;
    throw_if_failed(XAudio2Create(&engine,XAUDIO2_DEBUG_ENGINE));
    XAUDIO2_DEBUG_CONFIGURATION debug{
        XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_INFO | XAUDIO2_LOG_DETAIL,
        XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_INFO | XAUDIO2_LOG_DETAIL
    };
    engine->SetDebugConfiguration(&debug);
    IXAudio2MasteringVoice* master_voice;
    throw_if_failed(engine->CreateMasteringVoice(&master_voice));
    auto* stream = rdx_create_audio_stream(BUFFER_SIZE, malloc, free); // buffer size here
    void* bytes;
    size_t size;
    rdx_open(stream, "test.wav", &bytes,&size);
    XAUDIO2_BUFFER buffer {
        0x0,
        (UINT32)size,
        (BYTE*)bytes,
        0,
        0,
        0,
        0,
        0,
        NULL
    };
    int channels = rdx_get_number_of_channels(stream);
    int rate = rdx_get_sample_rate(stream);
    int bits = rdx_get_bits_per_sample(stream);
    auto sample_format = rdx_get_sample_format(stream);
    bool float_fmt = sample_format == F32;
    WAVEFORMATEX wave_fmt {
        float_fmt ? (WORD)WAVE_FORMAT_IEEE_FLOAT : (WORD)WAVE_FORMAT_PCM,
        static_cast<WORD>(channels),
        static_cast<DWORD>(rate),
        static_cast<DWORD>(rate*((bits*channels)/8)),
        static_cast<WORD>((bits*channels)/8),
        static_cast<WORD>(bits),
        sizeof(WAVEFORMATEX)
    };
    IXAudio2SourceVoice* audio_voice;
    throw_if_failed(engine->CreateSourceVoice(&audio_voice, &wave_fmt));
    throw_if_failed(audio_voice->SubmitSourceBuffer(&buffer));
    audio_voice->Start();

    // infinite loop for quick demo
    while(true) {
        XAUDIO2_VOICE_STATE state;
        audio_voice->GetState(&state,0);
        if(state.BuffersQueued <= 1) {

            auto status = rdx_fill_next_buffer(stream, &bytes, &size);
            buffer.AudioBytes =(UINT32)size;
            buffer.pAudioData = (BYTE*)bytes;
            buffer.Flags = status == AB_LAST_FRAME ? XAUDIO2_END_OF_STREAM : 0x0;
            audio_voice->SubmitSourceBuffer(&buffer);
            if(status == AB_LAST_FRAME) {
                break;
            }
        }
    }
}