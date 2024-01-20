#include "UNIT_SCALES.h"

bool UNIT_SCALES::begin(TwoWire *wire, uint8_t sda, uint8_t scl, uint8_t addr) {
    _wire = wire;
    _addr = addr;
    _sda  = sda;
    _scl  = scl;
    _wire->begin(_sda, _scl, 400000UL);
    delay(10);
    _wire->beginTransmission(_addr);
    uint8_t error = _wire->endTransmission();
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

bool UNIT_SCALES::writeBytes(uint8_t addr, uint8_t reg, uint8_t *buffer,
                             uint8_t length) {
    _wire->beginTransmission(addr);
    _wire->write(reg);
    _wire->write(buffer, length);
    if (_wire->endTransmission() == 0) return true;
    return false;
}

bool UNIT_SCALES::readBytes(uint8_t addr, uint8_t reg, uint8_t *buffer,
                            uint8_t length) {
    uint8_t index = 0;
    _wire->beginTransmission(addr);
    _wire->write(reg);
    _wire->endTransmission(false);
    if (_wire->requestFrom(addr, length)) {
        for (uint8_t i = 0; i < length; i++) {
            buffer[index++] = _wire->read();
        }
        return true;
    }
    return false;
}

uint8_t UNIT_SCALES::getBtnStatus() {
    uint8_t data = 0;
    readBytes(_addr, UNIT_SCALES_BUTTON_REG, &data, 1);
    return data;
}

bool UNIT_SCALES::setLEDColor(uint32_t colorHEX) {
    uint8_t color[4] = {0};
    // RED
    color[0] = (colorHEX >> 16) & 0xff;
    // GREEN
    color[1] = (colorHEX >> 8) & 0xff;
    // BLUE
    color[2] = colorHEX & 0xff;
    return writeBytes(_addr, UNIT_SCALES_RGB_LED_REG, color, 3);
}

bool UNIT_SCALES::setLPFilter(uint8_t en) {
    uint8_t reg = UNIT_SCALES_FILTER_REG;

    return writeBytes(_addr, reg, (uint8_t *)&en, 1);
}

uint8_t UNIT_SCALES::getLPFilter(void) {
    uint8_t data;
    uint8_t reg = UNIT_SCALES_FILTER_REG;

    readBytes(_addr, reg, (uint8_t *)&data, 1);

    return data;
}

bool UNIT_SCALES::setAvgFilter(uint8_t avg) {
    uint8_t reg = UNIT_SCALES_FILTER_REG + 1;

    return writeBytes(_addr, reg, (uint8_t *)&avg, 1);
}

uint8_t UNIT_SCALES::getAvgFilter(void) {
    uint8_t data;
    uint8_t reg = UNIT_SCALES_FILTER_REG + 1;

    readBytes(_addr, reg, (uint8_t *)&data, 1);

    return data;
}

bool UNIT_SCALES::setEmaFilter(uint8_t ema) {
    uint8_t reg = UNIT_SCALES_FILTER_REG + 2;

    return writeBytes(_addr, reg, (uint8_t *)&ema, 1);
}

uint8_t UNIT_SCALES::getEmaFilter(void) {
    uint8_t data;
    uint8_t reg = UNIT_SCALES_FILTER_REG + 2;

    readBytes(_addr, reg, (uint8_t *)&data, 1);

    return data;
}

uint32_t UNIT_SCALES::getLEDColor() {
    uint8_t color[4]  = {0};
    uint32_t colorHEX = 0;
    if (readBytes(_addr, UNIT_SCALES_RGB_LED_REG, color, 3)) {
        colorHEX = color[0];
        colorHEX = (colorHEX << 8) | color[1];
        colorHEX = (colorHEX << 8) | color[2];
    };
    return colorHEX;
}

float UNIT_SCALES::getWeight() {
    uint8_t data[4];
    float c;
    uint8_t *p;

    if (readBytes(_addr, UNIT_SCALES_CAL_DATA_REG, data, 4)) {
        p = (uint8_t *)&c;
        memcpy(p, data, 4);
    };
    return c;
}

int32_t UNIT_SCALES::getWeightInt() {
    uint8_t data[4];

    readBytes(_addr, UNIT_SCALES_CAL_DATA_INT_REG, data, 4);
    return (data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
}

String UNIT_SCALES::getWeightString() {
    char *p;
    uint8_t data[16];
    String res;

    readBytes(_addr, UNIT_SCALES_CAL_DATA_STRING_REG, data, 16);
    p   = (char *)data;
    res = p;
    return res;
}

float UNIT_SCALES::getGapValue() {
    uint8_t data[4];
    float c;
    uint8_t *p;

    if (readBytes(_addr, UNIT_SCALES_SET_GAP_REG, data, 4)) {
        p = (uint8_t *)&c;
        memcpy(p, data, 4);
    };
    return c;
}

void UNIT_SCALES::setGapValue(float offset) {
    uint8_t datatmp[4];
    uint8_t *p;
    p = (uint8_t *)&offset;

    memcpy(datatmp, p, 4);

    writeBytes(_addr, UNIT_SCALES_SET_GAP_REG, datatmp, 4);
    delay(100);
}

void UNIT_SCALES::setOffset(void) {
    uint8_t datatmp[4];
    datatmp[0] = 1;

    writeBytes(_addr, UNIT_SCALES_SET_OFFESET_REG, datatmp, 1);
}

int32_t UNIT_SCALES::getRawADC() {
    uint8_t data[4] = {0};
    int rawADC      = 0;
    if (readBytes(_addr, UNIT_SCALES_RAW_ADC_REG, data, 4)) {
        rawADC = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    };

    return rawADC;
}

uint8_t UNIT_SCALES::setI2CAddress(uint8_t addr) {
    _wire->beginTransmission(_addr);
    _wire->write(I2C_ADDRESS_REG);
    _wire->write(addr);
    _wire->endTransmission();
    _addr = addr;
    return _addr;
}

uint8_t UNIT_SCALES::getI2CAddress(void) {
    _wire->beginTransmission(_addr);
    _wire->write(I2C_ADDRESS_REG);
    _wire->endTransmission();

    uint8_t RegValue;

    _wire->requestFrom(_addr, 1);
    RegValue = Wire.read();
    return RegValue;
}

uint8_t UNIT_SCALES::getFirmwareVersion(void) {
    _wire->beginTransmission(_addr);
    _wire->write(FIRMWARE_VERSION_REG);
    _wire->endTransmission();

    uint8_t RegValue;

    _wire->requestFrom(_addr, 1);
    RegValue = Wire.read();
    return RegValue;
}

void UNIT_SCALES::jumpBootloader(void) {
    uint8_t value = 1;

    writeBytes(_addr, JUMP_TO_BOOTLOADER_REG, (uint8_t *)&value, 1);
}
