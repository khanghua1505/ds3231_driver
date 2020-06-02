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

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "ds3231.h"

#define DS3231_DATETIME_SIZE    (7)
#define DS3231_CONTROL_SIZE     (2)

#define DS3231_CONTROL1_INTCN    (1 << 2)
#define DS3231_CONTROL1_A1IE     (1 << 0)
#define DS3231_CONTROL1_A2IE     (1 << 1)
#define DS3231_CONTROL2_A1F      (1 << 0)
#define DS3231_CONTROL2_A2F      (1 << 1)

#define A1M1    0
#define A1M2    1
#define A1M3    2
#define A1M4    3

#define A2M2    0
#define A2M3    1
#define A2M4    2

static uint8_t m_buffer[16];

static inline
uint8_t bcd2dec(uint8_t c)
{
  return (c >> 4) * 10 + (c & 0x0f);
}

static inline
uint8_t dec2bcd(uint8_t c)
{
  return (c / 10)<<4 | (c % 10);
}

ds3231_status_t ds3231_init(ds3231_handle_t *handle, ds3231_write_t writefn,
    ds3231_read_t readfn)
{
  if (handle == NULL || writefn == NULL || readfn == NULL) {
    return DS3231_ERROR_NULLPTR;
  }

  int index;
  int byte_transferred;
  uint8_t ctrl_reg[2];

  handle->write = writefn;
  handle->read = readfn;
  for (index = DS3231_ALARM_1; index < DS3231_ALARM_TOTAL; index++) {
    handle->alarms[index].enabled = false;
    handle->alarms[index].cb = NULL;
  }

  byte_transferred = handle->read(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
      ctrl_reg, DS3231_CONTROL_SIZE);
  if (byte_transferred != DS3231_CONTROL_SIZE) {
    return DS3231_ERROR_READ_FAILED;
  }

  ctrl_reg[0] = ctrl_reg[0] | (DS3231_CONTROL1_INTCN);
  ctrl_reg[0] = ctrl_reg[0] & (~DS3231_CONTROL1_A1IE);
  ctrl_reg[0] = ctrl_reg[0] & (~DS3231_CONTROL1_A2IE);
  ctrl_reg[1] = ctrl_reg[1] & (~DS3231_CONTROL2_A1F);
  ctrl_reg[1] = ctrl_reg[1] & (~DS3231_CONTROL2_A2F);

  byte_transferred = handle->write(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
      ctrl_reg, DS3231_CONTROL_SIZE);
  if (byte_transferred != DS3231_CONTROL_SIZE) {
    return DS3231_ERROR_WRITE_FAILED;
  }

  return DS3231_OK;
}

ds3231_status_t ds3231_gettime(ds3231_handle_t *handle, epoch_time_t *time)
{
  if (handle == NULL || time == NULL || handle->read == NULL) {
    return DS3231_ERROR_NULLPTR;
  }

  int byte_transferred;

  byte_transferred = handle->read(DS3231_ADDRESS, DS3231_TIME_REG_BASE_ADDRESS,
      m_buffer, DS3231_DATETIME_SIZE);
  if (byte_transferred != DS3231_DATETIME_SIZE) {
    return DS3231_ERROR_READ_FAILED;
  }

  struct tm ts = {
    .tm_sec = bcd2dec(m_buffer[0]),
    .tm_min = bcd2dec(m_buffer[1]),
    .tm_hour = bcd2dec(m_buffer[2]),
    .tm_mday = bcd2dec(m_buffer[4]),
    .tm_mon = bcd2dec(m_buffer[5]) - 1,
    .tm_year = bcd2dec(m_buffer[6]) + 100,
    .tm_wday = bcd2dec(m_buffer[3]) - 1,
    .tm_isdst = -1,
  };

  *time = mktime(&ts);

  return DS3231_OK;
}

ds3231_status_t ds3231_settime(ds3231_handle_t *handle, epoch_time_t time)
{
  if (handle == NULL || handle->write == NULL) {
    return DS3231_ERROR_NULLPTR;
  }

  int byte_transferred;
  struct tm ts = *localtime(&time);

  m_buffer[0] = dec2bcd(ts.tm_sec);
  m_buffer[1] = dec2bcd(ts.tm_min);
  m_buffer[2] = dec2bcd(ts.tm_hour);
  m_buffer[3] = dec2bcd(ts.tm_wday + 1);
  m_buffer[4] = dec2bcd(ts.tm_mday);
  m_buffer[5] = dec2bcd(ts.tm_mon + 1);
  m_buffer[6] = dec2bcd(ts.tm_year-100);

  byte_transferred = handle->write(DS3231_ADDRESS, DS3231_TIME_REG_BASE_ADDRESS,
      m_buffer, DS3231_DATETIME_SIZE);
  if (byte_transferred != DS3231_DATETIME_SIZE) {
    return DS3231_ERROR_WRITE_FAILED;
  }

  return DS3231_OK;
}

ds3231_status_t ds3231_set_alarm(ds3231_handle_t *handle, ds3231_alarm_type_t alarm,
    uint8_t alarm_mask, ds3231_datetime_t time, ds3231_alarm_callback_t cb, void *args)
{
  if (handle == NULL || handle->read == NULL || handle->write == NULL) {
    return DS3231_ERROR_NULLPTR;
  }

  int byte_transferred;
  uint8_t ctrl_reg[2];
  uint8_t alarm_base;
  uint8_t alarm_time_size;
  bool flags = false;

  byte_transferred = handle->read(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
      ctrl_reg, DS3231_CONTROL_SIZE);
  if (byte_transferred != DS3231_CONTROL_SIZE) {
    return DS3231_ERROR_READ_FAILED;
  }

  if (alarm == DS3231_ALARM_1) {
    m_buffer[0] = dec2bcd(time.sec) | (((alarm_mask >> A1M1) & 0x1) << 7);
    m_buffer[1] = dec2bcd(time.min) | (((alarm_mask >> A1M2) & 0x1) << 7);
    m_buffer[2] = dec2bcd(time.hour) | (((alarm_mask >> A1M3) & 0x1) << 7);
    m_buffer[3] = dec2bcd(time.date) | (((alarm_mask >> A1M4) & 0x1) << 7);

    alarm_base = DS3231_ALARM1_REG_BASE_ADDRESS;
    alarm_time_size = 4;
  } else if (alarm == DS3231_ALARM_2) {
    m_buffer[0] = dec2bcd(time.min) | (((alarm_mask >> A2M2) & 0x1) << 7);
    m_buffer[1] = dec2bcd(time.hour) | (((alarm_mask >> A2M3) & 0x1) << 7);
    m_buffer[2] = dec2bcd(time.date) | (((alarm_mask >> A2M4) & 0x1) << 7);

    alarm_base = DS3231_ALARM2_REG_BASE_ADDRESS;
    alarm_time_size = 3;
  } else {
    return DS3231_ERROR_INVALID;
  }

  if (ctrl_reg[0] & DS3231_CONTROL1_INTCN) {
    // Turn off DS3231_CONTROL1_INTCN
    ctrl_reg[0] = ctrl_reg[0] & (~DS3231_CONTROL1_INTCN);

    byte_transferred = handle->write(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
        ctrl_reg, 1);
    if (byte_transferred != 1) {
      return DS3231_ERROR_WRITE_FAILED;
    }

    flags = true;
  }

  byte_transferred = handle->write(DS3231_ADDRESS, alarm_base,
      m_buffer, alarm_time_size);
  if (byte_transferred != alarm_time_size) {
    return DS3231_ERROR_WRITE_FAILED;
  }

  if (flags) {
    // Restore Control register.
    ctrl_reg[0] = ctrl_reg[0] | (DS3231_CONTROL1_INTCN);

    byte_transferred = handle->write(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
        ctrl_reg, 1);
    if (byte_transferred != 1) {
      return DS3231_ERROR_WRITE_FAILED;
    }
  }

  handle->alarms[alarm].cb = cb;
  handle->alarms[alarm].args = args;

  return DS3231_OK;
}

ds3231_status_t ds3231_get_alarm(ds3231_handle_t *handle, ds3231_alarm_type_t alarm,
    ds3231_datetime_t *time)
{
  if (handle == NULL || handle->read == NULL || time == NULL) {
    return DS3231_ERROR_NULLPTR;
  }

  int byte_transferred;
  uint8_t alarm_base;
  uint8_t alarm_time_size;

  if (alarm == DS3231_ALARM_1) {
    alarm_base = DS3231_ALARM1_REG_BASE_ADDRESS;
    alarm_time_size = 4;
  } else if (alarm == DS3231_ALARM_2) {
    alarm_base = DS3231_ALARM2_REG_BASE_ADDRESS;
    alarm_time_size = 3;
  } else {
    return DS3231_ERROR_INVALID;
  }

  byte_transferred = handle->read(DS3231_ADDRESS, alarm_base,
      m_buffer, alarm_time_size);
  if (byte_transferred != alarm_time_size) {
    return DS3231_ERROR_READ_FAILED;
  }

  if (alarm == DS3231_ALARM_1) {
    time->sec = bcd2dec(m_buffer[0] & 0b01111111);
    time->min = bcd2dec(m_buffer[1] & 0b01111111);
    time->hour = bcd2dec(m_buffer[2] & 0b01111111);
    time->date = bcd2dec(m_buffer[3] & 0b01111111);
  } else if (alarm == DS3231_ALARM_2) {
    time->sec = 0,
    time->min = bcd2dec(m_buffer[0] & 0b01111111);
    time->hour = bcd2dec(m_buffer[1] & 0b01111111);
    time->date = bcd2dec(m_buffer[2] & 0b01111111);
  }

  return DS3231_OK;
}

ds3231_status_t ds3231_allow_alarm(ds3231_handle_t *handle, ds3231_alarm_type_t alarm,
    bool enabled)
{
  if (handle == NULL || handle->read == NULL || handle->write == NULL) {
    return DS3231_ERROR_NULLPTR;
  }

  int byte_transferred;
  uint8_t ctrl_reg[2];

  byte_transferred = handle->read(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
      ctrl_reg, DS3231_CONTROL_SIZE);
  if (byte_transferred != DS3231_CONTROL_SIZE) {
    return DS3231_ERROR_READ_FAILED;
  }

  if (alarm  == DS3231_ALARM_1) {
    if (enabled) {
      ctrl_reg[0] = ctrl_reg[0] | DS3231_CONTROL1_INTCN;
      ctrl_reg[0] = ctrl_reg[0] | DS3231_CONTROL1_A1IE;
      handle->alarms[alarm].enabled = true;
    } else {
      ctrl_reg[0] = ctrl_reg[0] & (~DS3231_CONTROL1_A1IE);
      handle->alarms[alarm].enabled = false;
    }
  } else if (alarm  == DS3231_ALARM_2) {
    if (enabled) {
      ctrl_reg[0] = ctrl_reg[0] | DS3231_CONTROL1_INTCN;
      ctrl_reg[0] = ctrl_reg[0] | DS3231_CONTROL1_A2IE;
      handle->alarms[alarm].enabled = true;
    } else {
      ctrl_reg[0] = ctrl_reg[0] & (~DS3231_CONTROL1_A2IE);
      handle->alarms[alarm].enabled = false;
    }
  }

  byte_transferred = handle->write(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
      ctrl_reg, DS3231_CONTROL_SIZE);
  if (byte_transferred != DS3231_CONTROL_SIZE) {
    return DS3231_ERROR_WRITE_FAILED;
  }

  return DS3231_OK;
}

ds3231_status_t ds3231_alarm_callback(ds3231_handle_t *handle)
{
  if (handle == NULL || handle->read == NULL || handle->write == NULL) {
    return DS3231_ERROR_NULLPTR;
  }

  int byte_transferred;
  uint8_t ctrl_reg[2];

  byte_transferred = handle->read(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
      ctrl_reg, DS3231_CONTROL_SIZE);
  if (byte_transferred != DS3231_CONTROL_SIZE) {
    return DS3231_ERROR_READ_FAILED;
  }

  if ((ctrl_reg[1] & DS3231_CONTROL2_A1F) && handle->alarms[0].enabled){
	if(handle->alarms[0].cb != NULL) {
	  handle->alarms[0].cb(handle->alarms[0].args);
	}
    ctrl_reg[1] = ctrl_reg[1] & (~DS3231_CONTROL2_A1F);
  }

  if ((ctrl_reg[1] & DS3231_CONTROL2_A2F) && handle->alarms[1].enabled) {
	if(handle->alarms[1].cb != NULL) {
	  handle->alarms[1].cb(handle->alarms[1].args);
	}
    ctrl_reg[1] = ctrl_reg[1] & (~DS3231_CONTROL2_A2F);
  }

  byte_transferred = handle->write(DS3231_ADDRESS, DS3231_CONTROL1_REG_ADDRESS,
      ctrl_reg, DS3231_CONTROL_SIZE);
  if (byte_transferred != DS3231_CONTROL_SIZE) {
    return DS3231_ERROR_WRITE_FAILED;
  }

  return DS3231_OK;
}

ds3231_datetime_t ds3231_time2datetime(epoch_time_t time)
{
  ds3231_datetime_t retval;
  struct tm ts = *localtime(&time);

  retval.sec = ts.tm_sec;
  retval.min = ts.tm_min;
  retval.hour = ts.tm_hour;
  retval.day = ts.tm_wday + 1;
  retval.date = ts.tm_mday;
  retval.mon = ts.tm_mon + 1;
  retval.year = ts.tm_year - 100;

  return retval;
}

epoch_time_t ds3231_datetime2time(ds3231_datetime_t time)
{
  struct tm ts;

  ts.tm_sec = time.sec;
  ts.tm_min = time.min;
  ts.tm_hour = time.hour;
  ts.tm_mday = time.date;
  ts.tm_mon = time.mon - 1;
  ts.tm_year = time.year + 100;

  return mktime(&ts);
}
