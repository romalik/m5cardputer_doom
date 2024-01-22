/**
 * @file M5Cardputer_WebRadio.ino
 * @author Aurélio Avanzi
 * @brief M5Cardputer WebRadio
 * @version 0.1
 * @date 2023-12-12
 *
 * @Hardwares: M5Cardputer
 * @Platform Version: Arduino M5Stack Board Manager v2.0.7
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * ESP8266Audio: https://github.com/earlephilhower/ESP8266Audio/
 */

#define WIFI_SSID "IOTNetwork"
#define WIFI_PASS "fwintheshell"

#include "app_radio.h"
#include "spdlog/spdlog.h"

// #include <M5Cardputer.h>
// #include <M5Unified.h>
#include <HTTPClient.h>
#include <math.h>
#include <AudioOutput.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorAAC.h>
#include <AudioOutputI2S.h>

/// set web radio station url
#define FONT_DEFAULT &fonts::lgfxJapanGothic_12
#define FONT_CN &fonts::efontCN_12

#define AAC_URL_INDEXS (i == 2)
#define CN_FONT_URL_INDEXS (i == 0 || i == 7 || i == 8 || i == 9)
static constexpr const char* station_list[][2] = {
    {"北京文艺广播 FM87.6" , "http://lhttp.qtfm.cn/live/333/64k.mp3"},
    {"shadowfr69 Radio mp3-256k"  , "https://radio.shadowfr69.eu/mp3"},
    {"shadowfr69 Radio aac-192k", "https://radio.shadowfr69.eu/aac"},
    {"Lite Favorites"    , "http://naxos.cdnstream.com:80/1255_128"},
    {"MundoLivre FM"     , "http://rrdns-continental.webnow.com.br/mundolivre.mp3"},
    {"Tsubaki Web Radio" , "http://stream.tsubakianimeradio.com:9000/stream/1/"},
    {"Japanimradio - Osaka", "http://ais-edge104-live365-dal02.cdnstream.com/a53497"},
    {"959年代音乐怀旧好声音", "http://lhttp.qtfm.cn/live/5021381/64k.mp3"},
    {"动听音乐台",           "http://lhttp.qtfm.cn/live/5022107/64k.mp3"},
    {"星河音乐",            "http://lhttp.qtfm.cn/live/20210755/64k.mp3"},
    // {"thejazzstream"     , "http://wbgo.streamguys.net/thejazzstream"},
    {"u80s"              , "http://ice6.somafm.com/u80s-128-mp3"},
    {"Awesome80s"        , "http://listen.livestreamingservice.com/181-awesome80s_128k.mp3"},
    {"Metal Detector"    , "http://ice4.somafm.com/metal-128-mp3"},
    {"181-beatles_128k"  , "http://listen.181fm.com/181-beatles_128k.mp3"},
    {"Rock hairband"     , "http://listen.livestreamingservice.com/181-hairband_128k.mp3"},
    {"Dub Step Beyond"   , "http://ice1.somafm.com/dubstep-256-mp3"},
    {"illstreet-128-mp3" , "http://ice1.somafm.com/illstreet-128-mp3"},
    {"bootliquor-128-mp3", "http://ice1.somafm.com/bootliquor-128-mp3"},
    {"dronezone-128-mp3" , "http://ice1.somafm.com/dronezone-128-mp3"},
    {"Classic FM"        , "http://media-ice.musicradio.com:80/ClassicFMMP3"},
    //{"Morcegao FM"       , "http://tunein.com/radio/Morcego-FM-s112812/"},
    //{"Morcegao FM"       , "http://radio.morcegaofm.com.br/morcegao32/"},
};
static constexpr const size_t stations = sizeof(station_list) / sizeof(station_list[0]);

/// set M5Speaker virtual channel (0-7)
constexpr uint8_t m5spk_virtual_channel = 0;

constexpr const int preallocateBufferSize = 32 * 512;
constexpr const int preallocateCodecSize = 85332; // MP3 codec max mem needed

void* preallocateBuffer = nullptr;
void* preallocateCodec = nullptr;
constexpr size_t WAVE_SIZE = 320;
AudioOutputM5Speaker out(nullptr, m5spk_virtual_channel);
AudioGenerator *decoder = nullptr;
AudioFileSourceICYStream *file = nullptr;
AudioFileSourceBuffer *buff = nullptr;
fft_t fft;
bool fft_enabled = false;
bool wave_enabled = false;
uint16_t prev_y[(FFT_SIZE / 2)+1];
uint16_t peak_y[(FFT_SIZE / 2)+1];
int16_t wave_y[WAVE_SIZE];
int16_t wave_h[WAVE_SIZE];
int16_t raw_data[WAVE_SIZE * 2];
int header_height = 0;
size_t station_index = 0;
char stream_title[128] = { 0 };
const char* meta_text[2] = { nullptr, stream_title };
const size_t meta_text_num = sizeof(meta_text) / sizeof(meta_text[0]);
uint8_t meta_mod_bits = 0;
volatile size_t playindex = ~0u;
TaskHandle_t decode_task_handel;

// #define preallocateBuffer AppRadio::_current_data->preallocateBuffer
// #define preallocateCodec AppRadio::_current_data->preallocateCodec
// #define out (*(AppRadio::_current_data->out))
// #define decoder AppRadio::_current_data->decoder
// #define file AppRadio::_current_data->file
// #define buff AppRadio::_current_data->buff
// #define fft AppRadio::_current_data->fft
// #define fft_enabled AppRadio::_current_data->fft_enabled
// #define wave_enabled AppRadio::_current_data->wave_enabled
// #define prev_y AppRadio::_current_data->prev_y
// #define peak_y AppRadio::_current_data->peak_y
// #define wave_y AppRadio::_current_data->wave_y
// #define wave_h AppRadio::_current_data->wave_h
// #define raw_data AppRadio::_current_data->raw_data
// #define header_height AppRadio::_current_data->header_height
// #define station_index AppRadio::_current_data->station_index
// #define stream_title AppRadio::_current_data->stream_title
// #define meta_text AppRadio::_current_data->meta_text
// #define meta_text_num AppRadio::_current_data->meta_text_num
// #define meta_mod_bits AppRadio::_current_data->meta_mod_bits
// #define playindex AppRadio::_current_data->playindex


using namespace MOONCAKE::APPS;



static void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string) {
    // (void)cbData;
    spdlog::info("MDCallback {}={}", type, string);

    if ((strcmp(type, "StreamTitle") == 0) && (strcmp(stream_title, string) != 0)) {
        strncpy(stream_title, string, sizeof(stream_title));
        stream_title[127] = '\0';
        meta_mod_bits |= 2;
    }
}

static void stop(void) {
    if (decoder) {
        decoder->stop();
        delete decoder;
        decoder = nullptr;
    }
    if (buff) {
        buff->close();
        delete buff;
        buff = nullptr;
    }
    if (file) {
        file->close();
        delete file;
        file = nullptr;
    }

    out.stop();
}

void AppRadio::play(size_t index) {
    playindex = index;
}

static void decodeTask(void*) {
    for (;;) {
        delay(1);
        if (playindex != ~0u) {
            auto i = playindex;
            playindex = ~0u;
            stop();
            meta_text[0] = station_list[i][0];
            stream_title[0] = 0;
            meta_mod_bits = 3;
            file = new AudioFileSourceICYStream(station_list[i][1]);
            file->RegisterMetadataCB(MDCallback, (void*)"ICY");
            buff = new AudioFileSourceBuffer(file, preallocateBuffer, preallocateBufferSize);
            if (AAC_URL_INDEXS)
                decoder = new AudioGeneratorAAC(preallocateCodec, preallocateCodecSize);
            else
                decoder = new AudioGeneratorMP3(preallocateCodec, preallocateCodecSize);
            decoder->begin(buff, &out);
        }
        if (decoder && decoder->isRunning()) {
            if (!decoder->loop()) { decoder->stop(); }
        }
    }
}

uint32_t AppRadio::bgcolor(LGFX_Device* gfx, int y) {
    auto h = gfx->height();
    auto dh = h - header_height;
    int v = ((h - y)<<5) / dh;
    if (dh > 44) {
        int v2 = ((h - y - 1)<<5) / dh;
        if ((v >> 2) != (v2 >> 2)) {
            return 0x666666u;
        }
    }
    return gfx->color888(v + 2, v, v + 6);
}

static int _pervious_volume;
static int _fft_prev_x[2];
static int _fft_peak_x[2];
void AppRadio::gfxSetup(LGFX_Device* gfx) {
    if (gfx == nullptr) { return; }
    //if (gfx->width() < gfx->height())
    //{
    //  gfx->setRotation(gfx->getRotation()^1);
    //}
    gfx->setFont(FONT_DEFAULT);
    // gfx->setFont(&fonts::efontCN_12);
    gfx->setEpdMode(epd_mode_t::epd_fastest);
    gfx->setTextWrap(false);
    gfx->setCursor(0, 8);
    // gfx->println("WebRadio player");
    gfx->fillRect(0, 6, gfx->width(), 2, TFT_BLACK);

    header_height = (gfx->height() > 80) ? 33 : 21;
    fft_enabled = !gfx->isEPD();
    if (fft_enabled) {
        // wave_enabled = true;
        // wave_enabled = (gfx->getBoard() != m5gfx::board_M5UnitLCD);

        for (int y = header_height; y < gfx->height(); ++y) {
            gfx->drawFastHLine(0, y, gfx->width(), bgcolor(gfx, y));
        }
    }

    for (int x = 0; x < (FFT_SIZE/2)+1; ++x) {
        prev_y[x] = INT16_MAX;
        peak_y[x] = INT16_MAX;
    }
    for (int x = 0; x < WAVE_SIZE; ++x) {
        wave_y[x] = gfx->height();
        wave_h[x] = 0;
    }
    _pervious_volume = 0;
    _fft_prev_x[0] = _fft_prev_x[1] = 0;
    _fft_peak_x[0] = _fft_peak_x[1] = 0;
}

void AppRadio::gfxLoop(LGFX_Device* gfx) {
    if (gfx == nullptr) { return; }
    if (header_height > 32) {
        if (meta_mod_bits) {
            gfx->startWrite();
            for (int id = 0; id < meta_text_num; ++id) {
                if (0 == (meta_mod_bits & (1<<id))) { continue; }
                meta_mod_bits &= ~(1<<id);
                size_t y = id * 12;
                if (y+12 >= header_height) { continue; }
                gfx->setCursor(4, 8 + y);
                gfx->fillRect(0, 8 + y, gfx->width(), 12, gfx->getBaseColor());
                auto &i = station_index;
                if (id == 0) {
                    gfx->print("<");
                    gfx->print(station_index+1);
                    gfx->print("> ");
                }
                if (CN_FONT_URL_INDEXS) gfx->setFont(FONT_CN);
                gfx->print(meta_text[id]);
                if (CN_FONT_URL_INDEXS) gfx->setFont(FONT_DEFAULT);
                gfx->print(" "); // Garbage data removal when UTF8 characters are broken in the middle.
            }
            gfx->display();
            gfx->endWrite();
        }
    }
    else {
        static int title_x;
        static int title_id;
        static int wait = INT16_MAX;

        if (meta_mod_bits) {
            if (meta_mod_bits & 1) {
                title_x = 4;
                title_id = 0;
                gfx->fillRect(0, 8, gfx->width(), 12, gfx->getBaseColor());
            }
            meta_mod_bits = 0;
            wait = 0;
        }

        if (--wait < 0) {
            int tx = title_x;
            int tid = title_id;
            wait = 3;
            gfx->startWrite();
            uint_fast8_t no_data_bits = 0;
            do {
                if (tx == 4) { wait = 255; }
                gfx->setCursor(tx, 8);
                const char* meta = meta_text[tid];
                if (meta[0] != 0) {
                    gfx->print(meta);
                    gfx->print("  /  ");
                    tx = gfx->getCursorX();
                    if (++tid == meta_text_num) { tid = 0; }
                    if (tx <= 4) {
                        title_x = tx;
                        title_id = tid;
                    }
                }
                else {
                    if ((no_data_bits |= 1 << tid) == ((1 << meta_text_num) - 1)) {
                        break;
                    }
                    if (++tid == meta_text_num) { tid = 0; }
                }
            } while (tx < gfx->width());
            --title_x;
            gfx->display();
            gfx->endWrite();
        }
    }

    if (fft_enabled) {
        auto buf = out.getBuffer();
        if (buf) {
            memcpy(raw_data, buf, WAVE_SIZE * 2 * sizeof(int16_t)); // stereo data copy
            gfx->startWrite();

            // draw stereo level meter
            for (size_t i = 0; i < 2; ++i) {
                int32_t level = 0;
                for (size_t j = i; j < 640; j += 32) {
                    uint32_t lv = abs(raw_data[j]);
                    if (level < lv) { level = lv; }
                }

                int32_t x = (level * gfx->width()) / INT16_MAX;
                int32_t px = _fft_prev_x[i];
                if (px != x) {
                    gfx->fillRect(x, i * 3, px - x, 2, px < x ? 0xFF9900u : 0x330000u);
                    _fft_prev_x[i] = x;
                }
                px = _fft_peak_x[i];
                if (px > x) {
                    gfx->writeFastVLine(px, i * 3, 2, TFT_BLACK);
                    px--;
                }
                else {
                    px = x;
                }
                if (_fft_peak_x[i] != px) {
                    _fft_peak_x[i] = px;
                    gfx->writeFastVLine(px, i * 3, 2, TFT_WHITE);
                }
            }
            gfx->display();

            // draw FFT level meter
            fft.exec(raw_data);
            size_t bw = gfx->width() / 30;
            if (bw < 3) { bw = 3; }
            int32_t dsp_height = gfx->height();
            int32_t fft_height = dsp_height - header_height - 1;
            size_t xe = gfx->width() / bw;
            if (xe > (FFT_SIZE/2)) { xe = (FFT_SIZE/2); }
            int32_t wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[0] + raw_data[1])) * fft_height) >> 17);

            uint32_t bar_color[2] = { 0x000033u, 0x99AAFFu };

            for (size_t bx = 0; bx <= xe; ++bx) {
                size_t x = bx * bw;
                if ((x & 7) == 0) { gfx->display(); taskYIELD(); }
                int32_t f = fft.get(bx);
                int32_t y = (f * fft_height) >> 18;
                if (y > fft_height) { y = fft_height; }
                y = dsp_height - y;
                int32_t py = prev_y[bx];
                if (y != py) {
                    gfx->fillRect(x, y, bw - 1, py - y, bar_color[(y < py)]);
                    prev_y[bx] = y;
                }
                py = peak_y[bx] + 1;
                if (py < y) {
                    gfx->writeFastHLine(x, py - 1, bw - 1, bgcolor(gfx, py - 1));
                }
                else {
                    py = y - 1;
                }
                if (peak_y[bx] != py) {
                    peak_y[bx] = py;
                    gfx->writeFastHLine(x, py, bw - 1, TFT_WHITE);
                }


                if (wave_enabled) {
                    for (size_t bi = 0; bi < bw; ++bi) {
                        size_t i = x + bi;
                        if (i >= gfx->width() || i >= WAVE_SIZE) { break; }
                        y = wave_y[i];
                        int32_t h = wave_h[i];
                        bool use_bg = (bi+1 == bw);
                        if (h>0) { /// erase previous wave.
                            gfx->setAddrWindow(i, y, 1, h);
                            h += y;
                            do {
                                uint32_t bg = (use_bg || y < peak_y[bx]) ? bgcolor(gfx, y)
                                                        : (y == peak_y[bx]) ? 0xFFFFFFu
                                                        : bar_color[(y >= prev_y[bx])];
                                gfx->writeColor(bg, 1);
                            } while (++y < h);
                        }
                        size_t i2 = i << 1;
                        int32_t y1 = wave_next;
                        wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[i2] + raw_data[i2 + 1])) * fft_height) >> 17);
                        int32_t y2 = wave_next;
                        if (y1 > y2) {
                            int32_t tmp = y1;
                            y1 = y2;
                            y2 = tmp;
                        }
                        y = y1;
                        h = y2 + 1 - y;
                        wave_y[i] = y;
                        wave_h[i] = h;
                        if (h>0) { /// draw new wave.
                            gfx->setAddrWindow(i, y, 1, h);
                            h += y;
                            do {
                                uint32_t bg = (y < prev_y[bx]) ? 0xFFCC33u : 0xFFFFFFu;
                                gfx->writeColor(bg, 1);
                            } while (++y < h);
                        }
                    }
                }
            }
            gfx->display();
            gfx->endWrite();
        }
    }

    if (!gfx->displayBusy()) { // draw volume bar
        uint8_t v = _data.hal->Speaker()->getVolume();
        int x = v * (gfx->width()) >> 8;
        if (_pervious_volume != x) {
            gfx->fillRect(x, 6, _pervious_volume - x, 2, _pervious_volume < x ? 0xAAFFAAu : 0u);
            gfx->display();
            _pervious_volume = x;
        }
    }
}

// void setup(void)
void AppRadio::_setup(void) {
    out._m5sound = _data.hal->Speaker();
    // auto cfg = M5.config();

    // If you want to play sound from ModuleDisplay, write this
    //  cfg.external_speaker.module_display = true;

    // If you want to play sound from ModuleRCA, write this
    //  cfg.external_speaker.module_rca     = true;

    // If you want to play sound from HAT Speaker, write this
    // cfg.external_speaker.hat_spk        = true;

    // If you want to play sound from HAT Speaker2, write this
    //cfg.external_speaker.hat_spk2       = true;

    // If you want to play sound from ATOMIC Speaker, write this
    //cfg.external_speaker.atomic_spk     = true;

    // M5Cardputer.begin(cfg);

    _data.hal->display()->setRotation(1);
    _data.hal->display()->clear();
    _data.hal->display()->setFont(FONT_DEFAULT);
    _data.hal->display()->setTextColor(TFT_WHITE);
    _data.hal->display()->setTextSize(1);
    _data.hal->display()->setCursor(0, 0);

    preallocateBuffer = malloc(preallocateBufferSize);
    preallocateCodec = malloc(preallocateCodecSize);
    if (!preallocateBuffer || !preallocateCodec) {
        _data.hal->display()->printf("FATAL ERROR:  \nUnable to preallocate \n%d bytes for app\n\n", preallocateBufferSize + preallocateCodecSize);
        _data.hal->display()->printf("free: %d, max couns: %d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT), heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
        if (preallocateBuffer) free(preallocateBuffer);
        if (preallocateCodec) free(preallocateCodec);
        delay(3000);
        return;
    }

    _data.hal->Speaker()->end();

    { /// custom setting
        auto spk_cfg = _data.hal->Speaker()->config();
        /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
        // spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
        spk_cfg.task_pinned_core = PRO_CPU_NUM;
        _data.hal->Speaker()->config(spk_cfg);
    }

    _data.hal->Speaker()->begin();


    _data.hal->display()->println("WebRadio APP launched");
    _data.hal->display()->println("press [esc] to fully exit");
    _data.hal->display()->println("press [Home (G0)] play in background");
    _data.hal->display()->println("arrow key, 0-9: set volume or channel");
    _data.hal->display()->println("");
    _data.hal->display()->println("Connecting to WiFi");

    if (!_data._wifi_already_connected) {
        WiFi.disconnect();
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        WiFi.begin(_data.wifi_ssid.c_str(), _data.wifi_password.c_str());
    }

    // Try forever
    int _try = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if (_try++ > 200) break;
        _data.hal->display()->print(".");
        delay(100);
    }
    if (WiFi.status() == WL_CONNECTED) {
        _data.hal->display()->println("");
        _data.hal->display()->println("WiFi conectada.");
        // _data.hal->display()->println(_data.wifi_ssid.c_str());
        _data.hal->display()->println("Endereço de IP: ");
        _data.hal->display()->println(WiFi.localIP());
        // _data.hal->display()->println();
        _data.hal->display()->setTextColor(TFT_GREEN, TFT_BLACK);
        _data.hal->display()->setTextSize(2);
        _data.hal->display()->setCursor(125, 105);
        _data.hal->display()->println("CONNECTED");
        _data.hal->display()->setTextColor(TFT_WHITE);
        _data.hal->display()->setTextSize(1);
        _data.hal->Speaker()->tone(1000, 100);
        delay(2000);
    } else {
        // _data.hal->display()->println("");
        // _data.hal->display()->println("");
        _data.hal->display()->setTextColor(TFT_RED, TFT_BLACK);
        _data.hal->display()->setTextSize(2);
        _data.hal->display()->setCursor(160, 105);
        _data.hal->display()->println("FAILED");
        _data.hal->Speaker()->tone(1200, 200);
        delay(5000);

        free(preallocateBuffer);
        free(preallocateCodec);
        WiFi.disconnect(true);
        return;
    }
    _data.hal->display()->clear();
    gfxSetup(_data.hal->display());
    play(station_index);
    xTaskCreatePinnedToCore(decodeTask, "decodeTask", 4096, nullptr, 1, &decode_task_handel, APP_CPU_NUM);
    __running = true;
}

// void loop(void)
void AppRadio::_loop(void) {
    wave_enabled = false;
    gfxLoop(_data.hal->display());
    {
        static int prev_frame;
        int frame;
        do {
            delay(1);
        } while (prev_frame == (frame = millis() >> 3)); /// 8 msec cycle wait
        prev_frame = frame;
    }

    // M5Cardputer.update();
    _data.hal->keyboard()->updateKeyList();
    auto v = _data.hal->Speaker()->getVolume();
    if (_data.last_key_num != _data.hal->keyboard()->keyList().size()) {
        auto pressing_key = _data.hal->keyboard()->getKey();
        _data.hal->Speaker()->tone(550, 50);
        if (!_data.last_key_num) {
            if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_KPSLASH) {
                delay(200);
                _data.hal->Speaker()->tone(1000, 100);
                if (++station_index >= stations) { station_index = 0; }
                play(station_index);
            } else if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_COMMA) {
                delay(200);
                _data.hal->Speaker()->tone(800, 100);
                if (--station_index >= stations) { station_index = 0; }
                play(station_index);
            } else if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_SEMICOLON) {
                if (v <= 245) v += 10; else v = 255;
                _data.hal->Speaker()->setVolume(v);
            } else if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_DOT) {
                if (v >= 10) v -= 10; else v = 0;
                _data.hal->Speaker()->setVolume(v);
            } else if (_data.hal->keyboard()->getKeyValue(pressing_key).value_num_first == KEY_GRAVE) {
                vTaskDelete(decode_task_handel);
                stop();
                __running = false;
                free(preallocateBuffer);
                free(preallocateCodec);
                if (!_data._wifi_already_connected)
                    WiFi.disconnect(true);
            } else if (KEY_1 <= _data.hal->keyboard()->getKeyValue(pressing_key).value_num_first
                && _data.hal->keyboard()->getKeyValue(pressing_key).value_num_first <= KEY_0)
            {
                delay(200);
                int i = _data.hal->keyboard()->getKeyValue(pressing_key).value_num_first - KEY_1;
                if (i >= stations) i = stations - 1;
                station_index = i;
                play(station_index);
                _data.hal->Speaker()->tone(900, 100);
            }
        }
        _data.last_key_num = _data.hal->keyboard()->keyList().size();
    }

    // if (M5Cardputer.BtnA.wasPressed())
    // {
    //   _data.hal->Speaker()->tone(440, 50);
    // }
    // if (M5Cardputer.BtnA.wasDecideClickCount())
    // {
    //   switch (M5Cardputer.BtnA.getClickCount())
    //   {
    //   case 1:
    //     _data.hal->Speaker()->tone(1000, 100);
    //     if (++station_index >= stations) { station_index = 0; }
    //     play(station_index);
    //     break;

    //   case 2:
    //     _data.hal->Speaker()->tone(800, 100);
    //     if (station_index == 0) { station_index = stations; }
    //     play(--station_index);
    //     break;
    //   }
    // }
    // if (M5Cardputer.BtnA.isHolding() || M5.BtnB.isPressed() || M5.BtnC.isPressed())
    // {
    //   int add = (M5.BtnB.isPressed()) ? -1 : 1;
    //   if (M5.BtnA.isHolding())
    //   {
    //     add = M5.BtnA.getClickCount() ? -1 : 1;
    //   }
    //   v += add;
    //   if (v <= 255)
    //   {
    //     _data.hal->Speaker()->setVolume(v);
    //   }
    // }
}
