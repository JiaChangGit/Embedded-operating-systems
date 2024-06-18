/**
  ***************************************************************************************************************
  ***************************************************************************************************************
  ***************************************************************************************************************

  File:		  File_Handling.h
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
#ifndef FILE_HANDLING_H_
#define FILE_HANDLING_H_

#include "fatfs.h"
#include "stdio.h"
#include "string.h"

#define FILEMGR_LIST_DEPDTH 24
#define FILEMGR_FILE_NAME_SIZE 40
#define FILEMGR_FULL_PATH_SIZE 256
#define FILEMGR_MAX_LEVEL 4
#define FILETYPE_DIR 0
#define FILETYPE_FILE 1

typedef struct _FILELIST_LineTypeDef {
  uint8_t type;
  uint8_t name[FILEMGR_FILE_NAME_SIZE];
} FILELIST_LineTypeDef;

typedef struct _FILELIST_FileTypeDef {
  FILELIST_LineTypeDef file[FILEMGR_LIST_DEPDTH];
  uint16_t ptr;
} FILELIST_FileTypeDef;

/* mounts the USB*/
void Mount_USB(void);

/* unmounts the USB*/
void Unmount_USB(void);

uint16_t AUDIO_GetWavObjectNumber(void);

#endif /* FILE_HANDLING_H_ */
