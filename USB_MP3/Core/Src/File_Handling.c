/**
  ***************************************************************************************************************
  ***************************************************************************************************************
  ***************************************************************************************************************

  File:		  File_Handling.c
  Author:     ControllersTech.com
  Updated:    10th JAN 2021

  ***************************************************************************************************************
  Copyright (C) 2017 ControllersTech.com

  This is a free software under the GNU license, you can redistribute it and/or
  modify it under the terms of the GNU General Public License version 3 as
  published by the Free Software Foundation. This software library is shared
  with public for educational purposes, without WARRANTY and Author is not
  liable for any damages caused directly or indirectly by this software, read
  more about this on the GNU General Public License.

  ***************************************************************************************************************
*/

#include "File_Handling.h"

#include "stm32f4xx_hal.h"
#include "usb_host.h"

extern FILELIST_FileTypeDef FileList;
extern ApplicationTypeDef Appli_state;

extern char USBHPath[4]; /* USBH logical drive path */
extern FATFS USBHFatFS;  /* File system object for USBH logical drive */
extern FIL USBHFile;     /* File object for USBH */

FILINFO USBHfno;
FRESULT fresult;  // result
UINT br, bw;      // File read/write count

USBH_HandleTypeDef hUSBHost;
uint16_t NumObs = 0;

FRESULT AUDIO_StorageParse(void) {
  FRESULT res = FR_OK;
  FILINFO fno;
  DIR dir;
  char *fn;

  res = f_opendir(&dir, USBHPath);
  FileList.ptr = 0;

  if (res == FR_OK) {
    while (Appli_state == APPLICATION_READY) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) {
        break;
      }
      if (fno.fname[0] == '.') {
        continue;
      }

      fn = fno.fname;

      if (FileList.ptr < FILEMGR_LIST_DEPDTH) {
        if ((fno.fattrib & AM_DIR) == 0) {
          if ((strstr(fn, "wav")) || (strstr(fn, "WAV"))) {
            strncpy((char *)FileList.file[FileList.ptr].name, (char *)fn,
                    FILEMGR_FILE_NAME_SIZE);
            FileList.file[FileList.ptr].type = FILETYPE_FILE;
            FileList.ptr++;
          }
        }
      }
    }
  }
  NumObs = FileList.ptr;
  f_closedir(&dir);
  return res;
}

uint16_t AUDIO_GetWavObjectNumber(void) {
  if (AUDIO_StorageParse() == FR_OK) return NumObs;
}

void Mount_USB(void) { fresult = f_mount(&USBHFatFS, USBHPath, 1); }

void Unmount_USB(void) { fresult = f_mount(NULL, USBHPath, 1); }
