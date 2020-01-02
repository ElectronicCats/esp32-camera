/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013-2019 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2019 Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * OV5640 driver.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sccb.h"
#include "ov5640.h"
#include "ov5640_regs.h"
#include "ov5640_settings.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
static const char* TAG = "ov5640";
#endif

static int read_reg(uint8_t slv_addr, const uint16_t reg){
    int ret = SCCB_Read16(slv_addr, reg);
#ifdef REG_DEBUG_ON
    if (ret < 0) {
        ESP_LOGE(TAG, "READ REG 0x%04x FAILED: %d", reg, ret);
    }
#endif
    return ret;
}

static int check_reg_mask(uint8_t slv_addr, uint16_t reg, uint8_t mask){
    return (read_reg(slv_addr, reg) & mask) == mask;
}

static int read_reg16(uint8_t slv_addr, const uint16_t reg){
    int ret = 0, ret2 = 0;
    ret = read_reg(slv_addr, reg);
    if (ret >= 0) {
        ret = (ret & 0xFF) << 8;
        ret2 = read_reg(slv_addr, reg+1);
        if (ret2 < 0) {
            ret = ret2;
        } else {
            ret |= ret2 & 0xFF;
        }
    }
    return ret;
}


static int write_reg(uint8_t slv_addr, const uint16_t reg, uint8_t value){
    int ret = 0;
#ifndef REG_DEBUG_ON
    ret = SCCB_Write16(slv_addr, reg, value);
#else
    int old_value = read_reg(slv_addr, reg);
    if (old_value < 0) {
        return old_value;
    }
    if ((uint8_t)old_value != value) {
        ESP_LOGI(TAG, "NEW REG 0x%04x: 0x%02x to 0x%02x", reg, (uint8_t)old_value, value);
        ret = SCCB_Write16(slv_addr, reg, value);
    } else {
        ESP_LOGD(TAG, "OLD REG 0x%04x: 0x%02x", reg, (uint8_t)old_value);
        ret = SCCB_Write16(slv_addr, reg, value);//maybe not?
    }
    if (ret < 0) {
        ESP_LOGE(TAG, "WRITE REG 0x%04x FAILED: %d", reg, ret);
    }
#endif
    return ret;
}

static int set_reg_bits(uint8_t slv_addr, uint16_t reg, uint8_t offset, uint8_t mask, uint8_t value)
{
    int ret = 0;
    uint8_t c_value, new_value;
    ret = read_reg(slv_addr, reg);
    if(ret < 0) {
        return ret;
    }
    c_value = ret;
    new_value = (c_value & ~(mask << offset)) | ((value & mask) << offset);
    ret = write_reg(slv_addr, reg, new_value);
    return ret;
}

static int write_regs(uint8_t slv_addr, const uint16_t (*regs)[2])
{
    int i = 0, ret = 0;
    while (!ret && regs[i][0] != REGLIST_TAIL) {
        if (regs[i][0] == REG_DLY) {
            vTaskDelay(regs[i][1] / portTICK_PERIOD_MS);
        } else {
            ret = write_reg(slv_addr, regs[i][0], regs[i][1]);
        }
        i++;
    }
    return ret;
}

static int write_reg16(uint8_t slv_addr, const uint16_t reg, uint16_t value)
{
    if (write_reg(slv_addr, reg, value >> 8) || write_reg(slv_addr, reg + 1, value)) {
        return -1;
    }
    return 0;
}

static int write_addr_reg(uint8_t slv_addr, const uint16_t reg, uint16_t x_value, uint16_t y_value)
{
    if (write_reg16(slv_addr, reg, x_value) || write_reg16(slv_addr, reg + 2, y_value)) {
        return -1;
    }
    return 0;
}

#define write_reg_bits(slv_addr, reg, mask, enable) set_reg_bits(slv_addr, reg, 0, mask, enable?mask:0)

static int reset(sensor_t *sensor)
{
    // Reset all registers
    write_reg(sensor->slv_addr, 0x3008, 0x42);
    // Delay 10 ms
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Write default regsiters
    //for (i=0, regs = default_regs; regs[i][0]; i++) {
    write_regs(sensor->slv_addr, default_regs);
    //}
    write_reg(sensor->slv_addr, 0x3008, 0x02);
    vTaskDelay(30 / portTICK_PERIOD_MS);
    // Write auto focus firmware
    //for (i=0, regs = OV5640_AF_REG; regs[i][0]; i++) {
    write_regs(sensor->slv_addr, OV5640_AF_REG);
    //}
    // Delay
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Enable auto focus
    write_reg(sensor->slv_addr, 0x3023, 0x01);
    write_reg(sensor->slv_addr, 0x3022, 0x04);

    vTaskDelay(30 / portTICK_PERIOD_MS);
    return 0;
}

static int sleep(sensor_t *sensor, int enable)
{
    uint8_t reg;
    if (enable) {
        reg = 0x42;
    } else {
        reg = 0x02;
    }
    // Write back register
    return write_reg(sensor->slv_addr, 0x3008, reg);
}

static int set_pixformat(sensor_t *sensor, pixformat_t pixformat)
{
    int ret=0;
    const uint16_t (*regs)[2] = 0;

    switch (pixformat) {
        case PIXFORMAT_RGB565:
            //write_reg(sensor->slv_addr, 0x4300, 0x61);//RGB565
            //write_reg(sensor->slv_addr, 0x501f, 0x01);//ISP RGB
            regs = sensor_fmt_rgb565;
            break;
        case PIXFORMAT_YUV422:
        case PIXFORMAT_GRAYSCALE:
            regs = sensor_fmt_grayscale;
            //write_reg(sensor->slv_addr, 0x4300, 0x10);//Y8
            //write_reg(sensor->slv_addr, 0x501f, 0x00);//ISP YUV
            break;
        case PIXFORMAT_BAYER:
            //reg = 0x00;//TODO: fix order
            break;
        default:
            ESP_LOGE(TAG, "Unsupported pixformat: %u", pixformat);
            return -1;
    }
    
    ret = write_regs(sensor->slv_addr, regs);
    if(ret == 0) {
        sensor->pixformat = pixformat;
        ESP_LOGD(TAG, "Set pixformat to: %u", pixformat);
    }
    return ret;
}

static int set_image_options(sensor_t *sensor)
{
    int ret = 0;
    uint8_t reg20 = 0;
    uint8_t reg21 = 0;
    uint8_t reg4514 = 0;
    uint8_t reg4514_test = 0;

    // compression
    if (sensor->pixformat == PIXFORMAT_JPEG) {
        reg21 |= 0x20;
    }

    // binning
    if (sensor->status.framesize > FRAMESIZE_SVGA) {
        reg20 |= 0x40;
    } else {
        reg20 |= 0x01;
        reg21 |= 0x01;
        reg4514_test |= 4;
    }

    // V-Flip
    if (sensor->status.vflip) {
        reg20 |= 0x06;
        reg4514_test |= 1;
    }

    // H-Mirror
    if (sensor->status.hmirror) {
        reg21 |= 0x06;
        reg4514_test |= 2;
    }

    switch (reg4514_test) {
        //no binning
        case 0: reg4514 = 0x88; break;//normal
        case 1: reg4514 = 0x88; break;//v-flip
        case 2: reg4514 = 0xbb; break;//h-mirror
        case 3: reg4514 = 0xbb; break;//v-flip+h-mirror
        //binning
        case 4: reg4514 = 0xaa; break;//normal
        case 5: reg4514 = 0xbb; break;//v-flip
        case 6: reg4514 = 0xbb; break;//h-mirror
        case 7: reg4514 = 0xaa; break;//v-flip+h-mirror
    }

    if(write_reg(sensor->slv_addr, TIMING_TC_REG20, reg20)
        || write_reg(sensor->slv_addr, TIMING_TC_REG21, reg21)
        || write_reg(sensor->slv_addr, 0x4514, reg4514)){
        ESP_LOGE(TAG, "Setting Image Options Failed");
        ret = -1;
    }

    ESP_LOGD(TAG, "Set Image Options: Compression: %u, Binning: %u, V-Flip: %u, H-Mirror: %u, Reg-4514: 0x%02x",
        sensor->pixformat == PIXFORMAT_JPEG, sensor->status.framesize <= FRAMESIZE_SVGA, sensor->status.vflip, sensor->status.hmirror, reg4514);
    return ret;
}

static int set_framesize(sensor_t *sensor, framesize_t framesize)
{
    int ret=0;
    /*framesize_t old_framesize = sensor->status.framesize;
    sensor->status.framesize = framesize;

    if(framesize >= FRAMESIZE_INVALID){
        ESP_LOGE(TAG, "Invalid framesize: %u", framesize);
        return -1;
    }
    */
    uint16_t w = resolution[framesize][0];
    uint16_t h = resolution[framesize][1];

    ret |= write_reg(sensor->slv_addr, TIMING_DVPHO, w>>8);
    ret |= write_reg(sensor->slv_addr, 0x3809,  w);
    ret |= write_reg(sensor->slv_addr, TIMING_DVPVO, h>>8);
    ret |= write_reg(sensor->slv_addr, 0x380b,  h);

    return ret;
}
/*
static int set_framerate(sensor_t *sensor, framerate_t framerate)
{
    return 0;
}
*/
static int set_contrast(sensor_t *sensor, int level)
{
    return 0;
}

static int set_brightness(sensor_t *sensor, int level)
{
    return 0;
}

static int set_saturation(sensor_t *sensor, int level)
{
    return 0;
}

static int set_gainceiling(sensor_t *sensor, gainceiling_t gainceiling)
{
    return 0;
}

static int set_colorbar(sensor_t *sensor, int enable)
{
    return 0;
}

static int set_auto_gain(sensor_t *sensor, int enable, float gain_db, float gain_db_ceiling)
{
    return 0;
}

static int get_gain_db(sensor_t *sensor, float *gain_db)
{
    return 0;
}

static int set_auto_exposure(sensor_t *sensor, int enable, int exposure_us)
{
    return 0;
}

static int get_exposure_us(sensor_t *sensor, int *exposure_us)
{
    return 0;
}

static int set_auto_whitebal(sensor_t *sensor, int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    return 0;
}

static int get_rgb_gain_db(sensor_t *sensor, float *r_gain_db, float *g_gain_db, float *b_gain_db)
{
    return 0;
}

static int set_hmirror(sensor_t *sensor, int enable)
{
    int ret = 0;
    sensor->status.hmirror = enable;
    ret = set_image_options(sensor);
    if (ret == 0) {
        ESP_LOGD(TAG, "Set h-mirror to: %d", enable);
    }
    return ret;
    /*
    uint8_t reg;
    int ret = read_reg(sensor->slv_addr, 0x3821, &reg);
    if (enable){
        ret |= write_reg(sensor->slv_addr, 0x3821, reg&0x06);
    } else {
        ret |= write_reg(sensor->slv_addr, 0x3821, reg|0xF9);
    }
    if (ret == 0) {
        ESP_LOGD(TAG, "Set h-mirror to: %d", enable);
    }
    return ret;
    */
}

static int set_vflip(sensor_t *sensor, int enable)
{
    
    int ret = 0;
    sensor->status.vflip = enable;
    ret = set_image_options(sensor);
    if (ret == 0) {
        ESP_LOGD(TAG, "Set v-flip to: %d", enable);
    }
    return ret;
    /*uint8_t reg;
    int ret = read_reg(sensor->slv_addr, 0x3820, &reg);
    if (enable){
        ret |= write_reg(sensor->slv_addr, 0x3820, reg&0xF9);
    } else {
        ret |= write_reg(sensor->slv_addr, 0x3820, reg|0x06);
    }
    if (ret == 0) {
        ESP_LOGD(TAG, "Set v-flip to: %d", enable);
    }
    return ret;
    */
}
/*
static int set_special_effect(sensor_t *sensor, sde_t sde)
{
    return 0;
}
*/
static int set_lens_correction(sensor_t *sensor, int enable, int radi, int coef)
{
    return 0;
}

int ov3660_init(sensor_t *sensor)
{

    sensor->gs_bpp              = 1;
    sensor->reset               = reset;
    sensor->sleep               = sleep;
    sensor->read_reg            = read_reg;
    sensor->write_reg           = write_reg;
    sensor->set_pixformat       = set_pixformat;
    sensor->set_framesize       = set_framesize;
    sensor->set_framerate       = set_framerate;
    sensor->set_contrast        = set_contrast;
    sensor->set_brightness      = set_brightness;
    sensor->set_saturation      = set_saturation;
    sensor->set_gainceiling     = set_gainceiling;
    sensor->set_colorbar        = set_colorbar;
    sensor->set_auto_gain       = set_auto_gain;
    sensor->get_gain_db         = get_gain_db;
    sensor->set_auto_exposure   = set_auto_exposure;
    sensor->get_exposure_us     = get_exposure_us;
    sensor->set_auto_whitebal   = set_auto_whitebal;
    sensor->get_rgb_gain_db     = get_rgb_gain_db;
    sensor->set_hmirror         = set_hmirror;
    sensor->set_vflip           = set_vflip;
    sensor->set_special_effect  = set_special_effect;
    sensor->set_lens_correction = set_lens_correction;

    // Set sensor flags
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_VSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_HSYNC, 0);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_PIXCK, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_FSYNC, 1);
    SENSOR_HW_FLAGS_SET(sensor, SENSOR_HW_FLAGS_JPEGE, 0);

    return 0;
}
