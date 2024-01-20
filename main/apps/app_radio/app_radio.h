/**
 * @file app_radio.h
 * @author Forairaaaaa
 * @brief
 * @version 0.6
 * @date 2023-09-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <mooncake.h>
#include <AudioOutput.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

#include "../../hal/hal.h"
#include "../utils/theme/theme_define.h"
#include "../utils/anim/anim_define.h"
#include "../utils/icon/icon_define.h"

#include "assets/radio_big.h"
#include "assets/radio_small.h"


class AudioOutputM5Speaker : public AudioOutput
{
  public:
    AudioOutputM5Speaker(m5::Speaker_Class* m5sound, uint8_t virtual_sound_channel = 0)
    {
      _m5sound = m5sound;
      _virtual_ch = virtual_sound_channel;
    }
    virtual ~AudioOutputM5Speaker(void) {};
    void set_speaker(m5::Speaker_Class* m5sound) {_m5sound = m5sound;}
    virtual bool begin(void) override { return true; }
    virtual bool ConsumeSample(int16_t sample[2]) override
    {
      if (_tri_buffer_index < tri_buf_size)
      {
        _tri_buffer[_tri_index][_tri_buffer_index  ] = sample[0];
        _tri_buffer[_tri_index][_tri_buffer_index+1] = sample[1];
        _tri_buffer_index += 2;

        return true;
      }

      flush();
      return false;
    }
    virtual void flush(void) override
    {
      if (_tri_buffer_index)
      {
        _m5sound->playRaw(_tri_buffer[_tri_index], _tri_buffer_index, hertz, true, 1, _virtual_ch);
        _tri_index = _tri_index < 2 ? _tri_index + 1 : 0;
        _tri_buffer_index = 0;
        ++_update_count;
      }
    }
    virtual bool stop(void) override
    {
      flush();
      _m5sound->stop(_virtual_ch);
      for (size_t i = 0; i < 3; ++i)
      {
        memset(_tri_buffer[i], 0, tri_buf_size * sizeof(int16_t));
      }
      ++_update_count;
      return true;
    }

    const int16_t* getBuffer(void) const { return _tri_buffer[(_tri_index + 2) % 3]; }
    const uint32_t getUpdateCount(void) const { return _update_count; }

    m5::Speaker_Class* _m5sound;
  protected:
    uint8_t _virtual_ch;
    // static constexpr size_t tri_buf_size = 640;
    static constexpr size_t tri_buf_size = 1280;
    int16_t _tri_buffer[3][tri_buf_size];
    size_t _tri_buffer_index = 0;
    size_t _tri_index = 0;
    size_t _update_count = 0;
};


#define FFT_SIZE 256
class fft_t
{
  float _wr[FFT_SIZE + 1];
  float _wi[FFT_SIZE + 1];
  float _fr[FFT_SIZE + 1];
  float _fi[FFT_SIZE + 1];
  uint16_t _br[FFT_SIZE + 1];
  size_t _ie;

public:
  fft_t(void)
  {
#ifndef M_PI
#define M_PI 3.141592653
#endif
    _ie = logf( (float)FFT_SIZE ) / log(2.0) + 0.5;
    static constexpr float omega = 2.0f * M_PI / FFT_SIZE;
    static constexpr int s4 = FFT_SIZE / 4;
    static constexpr int s2 = FFT_SIZE / 2;
    for ( int i = 1 ; i < s4 ; ++i)
    {
    float f = cosf(omega * i);
      _wi[s4 + i] = f;
      _wi[s4 - i] = f;
      _wr[     i] = f;
      _wr[s2 - i] = -f;
    }
    _wi[s4] = _wr[0] = 1;

    size_t je = 1;
    _br[0] = 0;
    _br[1] = FFT_SIZE / 2;
    for ( size_t i = 0 ; i < _ie - 1 ; ++i )
    {
      _br[ je << 1 ] = _br[ je ] >> 1;
      je = je << 1;
      for ( size_t j = 1 ; j < je ; ++j )
      {
        _br[je + j] = _br[je] + _br[j];
      }
    }
  }

  void exec(const int16_t* in)
  {
    memset(_fi, 0, sizeof(_fi));
    for ( size_t j = 0 ; j < FFT_SIZE / 2 ; ++j )
    {
      float basej = 0.25 * (1.0-_wr[j]);
      size_t r = FFT_SIZE - j - 1;

      /// perform han window and stereo to mono convert.
      _fr[_br[j]] = basej * (in[j * 2] + in[j * 2 + 1]);
      _fr[_br[r]] = basej * (in[r * 2] + in[r * 2 + 1]);
    }

    size_t s = 1;
    size_t i = 0;
    do
    {
      size_t ke = s;
      s <<= 1;
      size_t je = FFT_SIZE / s;
      size_t j = 0;
      do
      {
        size_t k = 0;
        do
        {
          size_t l = s * j + k;
          size_t m = ke * (2 * j + 1) + k;
          size_t p = je * k;
          float Wxmr = _fr[m] * _wr[p] + _fi[m] * _wi[p];
          float Wxmi = _fi[m] * _wr[p] - _fr[m] * _wi[p];
          _fr[m] = _fr[l] - Wxmr;
          _fi[m] = _fi[l] - Wxmi;
          _fr[l] += Wxmr;
          _fi[l] += Wxmi;
        } while ( ++k < ke) ;
      } while ( ++j < je );
    } while ( ++i < _ie );
  }

  uint32_t get(size_t index)
  {
    return (index < FFT_SIZE / 2) ? (uint32_t)sqrtf(_fr[ index ] * _fr[ index ] + _fi[ index ] * _fi[ index ]) : 0u;
  }
};

extern uint8_t meta_mod_bits;


namespace MOONCAKE
{
    namespace APPS
    {
        class AppRadio : public APP_BASE
        {
            private:
                static bool __running;
                enum State_t
                {
                    state_init = 0,
                    state_wait_ssid,
                    state_wait_password,
                    state_setup,
                    state_started,
                };

                struct Data_t
                {
                    HAL::Hal* hal = nullptr;

                    int last_key_num = 0;
                    std::string repl_input_buffer;
                    bool is_caps = false;
                    char* value_buffer = nullptr;

                    uint32_t cursor_update_time_count = 0;
                    uint32_t cursor_update_period = 500;
                    bool cursor_state = false;

                    State_t current_state = state_init;
                    std::string wifi_ssid;
                    std::string wifi_password;

                    int64_t _last_update_battery_time = 0;
                    bool _wifi_already_connected;

                    // radio
                    // void* preallocateBuffer = nullptr;
                    // void* preallocateCodec = nullptr;
                    // AudioOutputM5Speaker *out = nullptr;
                    // AudioGenerator *decoder = nullptr;
                    // AudioFileSourceICYStream *file = nullptr;
                    // AudioFileSourceBuffer *buff = nullptr;
                    // fft_t fft;
                    // bool fft_enabled = false;
                    // bool wave_enabled = false;
                    // uint16_t prev_y[(FFT_SIZE / 2)+1];
                    // uint16_t peak_y[(FFT_SIZE / 2)+1];
                    // int16_t wave_y[WAVE_SIZE];
                    // int16_t wave_h[WAVE_SIZE];
                    // int16_t raw_data[WAVE_SIZE * 2];
                    // int header_height = 0;
                    // size_t station_index = 0;
                    // char stream_title[128] = { 0 };
                    // const char* meta_text[2] = { nullptr, stream_title };
                    // const size_t meta_text_num = sizeof(meta_text) / sizeof(meta_text[0]);
                    // uint8_t meta_mod_bits = 0;
                    // volatile size_t playindex = ~0u;
                };

                void _update_input();
                void _update_cursor();
                void _update_state();

                // void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);
                // void stop(void);
                void play(size_t index);
                // void decodeTask(void*);
                uint32_t bgcolor(LGFX_Device* gfx, int y);
                void gfxSetup(LGFX_Device* gfx);
                void gfxLoop(LGFX_Device* gfx);

                void _setup();
                void _loop();

                Data_t _data;
            public:
                // static Data_t *_current_data;
                void onCreate() override;
                void onResume() override;
                void onRunning() override;
                void onDestroy() override;
        };

        class AppRadio_Packer : public APP_PACKER_BASE
        {
            std::string getAppName() override { return "RADIO"; }
            void* getAppIcon() override { return (void*)(new AppIcon_t(image_data_radio_big, image_data_radio_small)); }
            void* newApp() override { return new AppRadio; }
            void deleteApp(void *app) override { delete (AppRadio*)app; }
        };
    }
}
