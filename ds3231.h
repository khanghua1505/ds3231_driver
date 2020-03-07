/* Copyright (c) 2019 - 2020, Khang Hua, email: khanghua1505@gmail.com
 * All right reserved.
 *
 * This file is written and modified by Khang Hua.
 *
 * This model is free software; you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License; either version 2.1 of the License, or (at your option)
 * any later version. See the GNU Lesser General Public License for more details,
 *
 * This model is distributed in the hope that it will be useful.
 */

#ifndef _DS3231_H_
#define _DS3231_H_

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define DS3231_ADDRESS    (0x68 << 1)

#define DS3231_TIME_REG_BASE_ADDRESS    (0x00)
#define DS3231_SECONDS_REG_ADDRESS      (0x00)
#define DS3231_MINUTES_REG_ADDRESS      (0x01)
#define DS3231_HOUR_REG_ADDRESS         (0x02)
#define DS3231_DAY_REG_ADDRESS          (0x03)
#define DS3231_DATE_REG_ADDRESS         (0x04)
#define DS3231_MONTH_REG_ADDRESS        (0x05)
#define DS3231_YEAR_REG_ADDRESS         (0x06)

#define DS3231_ALARM1_REG_BASE_ADDRESS       (0x07)
#define DS3231_ALARM1_SECONDS_REG_ADDRESS    (0x07)
#define DS3231_ALARM1_MINUTES_REG_ADDRESS    (0x08)
#define DS3231_ALARM1_HOUR_REG_ADDRESS       (0x09)
#define DS3231_ALARM1_DATE_REG_ADDRESS       (0x0A)

#define DS3231_ALARM2_REG_BASE_ADDRESS       (0x0B)
#define DS3231_ALARM2_MINUTES_REG_ADDRESS    (0x0B)
#define DS3231_ALARM2_HOUR_REG_ADDRESS       (0x0C)
#define DS3231_ALARM2_DATE_REG_ADDRESS       (0x0D)

#define DS3231_CONTROL1_REG_ADDRESS    (0x0E)
#define DS3231_CONTROL2_REG_ADDRESS    (0x0F)

typedef time_t epoch_time_t;

typedef ssize_t (*ds3231_write_t)(uint16_t dev_addr, uint8_t reg_addr, 
    const uint8_t *buffer, uint16_t len); 

typedef ssize_t (*ds3231_read_t)(uint16_t dev_addr, uint8_t reg_addr, 
    uint8_t *buffer, uint16_t len);
    
typedef void (*ds3231_alarm_callback_t)(void *args);

typedef enum {
  DS3231_OK,
  DS3231_ERROR_TIMEOUT,
  DS3231_ERROR_INVALID,
  DS3231_ERROR_NULLPTR,
  DS3231_ERROR_READ_FAILED,
  DS3231_ERROR_WRITE_FAILED,
}
ds3231_status_t;

typedef enum {
  DS3231_ALARM_1,
  DS3231_ALARM_2,
  DS3231_ALARM_TOTAL
}
ds3231_alarm_type_t;

enum {
  DS3231_ALARM1_MASK_PER_SECOND         = 0b1111,
  DS3231_ALARM1_MASK_SECOND_MATCH       = 0b1110,
  DS3231_ALARM1_MASK_MIN_SEC_MATCH      = 0b1100,
  DS3231_ALARM1_MASK_HOUR_MIN_SEC_MATCH = 0b1000,
};

enum {
  DS3231_ALARM2_MASK_PER_MIN            = 0b111,
  DS3231_ALARM2_MASK_MIN_MATCH          = 0b110,
  DS3231_ALARM2_MASK_HOUR_MIN_MATCH     = 0b100,
};

typedef struct {
  bool enabled;
  ds3231_alarm_callback_t cb;
  void *args;
}
ds3231_alarm_t;

typedef struct {
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t day;
  uint8_t date;
  uint8_t mon;
  uint8_t year;
}
ds3231_datetime_t;

typedef struct {
  ds3231_write_t write;
  ds3231_read_t read;
  ds3231_alarm_t alarms[DS3231_ALARM_TOTAL];
}
ds3231_handle_t;

ds3231_status_t ds3231_init(ds3231_handle_t *handle, ds3231_write_t writefn, 
    ds3231_read_t readfn);
ds3231_status_t ds3231_get_epochtime(ds3231_handle_t *handle, epoch_time_t *time);
ds3231_status_t ds3231_set_epochtime(ds3231_handle_t *handle, epoch_time_t time);
ds3231_status_t ds3231_set_alarm(ds3231_handle_t *handle, ds3231_alarm_type_t alarm, 
    uint8_t alarm_mask, ds3231_datetime_t time, ds3231_alarm_callback_t cb, void *args);
ds3231_status_t ds3231_get_alarm(ds3231_handle_t *handle, ds3231_alarm_type_t alarm, 
    ds3231_datetime_t *time);
ds3231_status_t ds3231_allow_alarm(ds3231_handle_t *handle, ds3231_alarm_type_t alarm,
    bool enabled);
ds3231_status_t ds3231_alarm_callback(ds3231_handle_t *handle);
ds3231_datetime_t ds3231_convert_epochtime(epoch_time_t time);
epoch_time_t ds3231_convert_datetime(ds3231_datetime_t time);

#ifdef __cpluscplus
}
#endif  // __cplusplus

#endif  // _DS3231_H_
