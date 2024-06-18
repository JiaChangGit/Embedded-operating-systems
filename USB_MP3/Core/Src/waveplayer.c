/**
 ******************************************************************************
 * @file    Audio/Audio_playback_and_record/Src/waveplayer.c
 * @author  MCD Application Team
 * @brief   This file provides the Audio Out (playback) interface API
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
 * All rights reserved.</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/**
  ***************************************************************************************************************
  ***************************************************************************************************************
  ***************************************************************************************************************

  File:		  Waveplayer.c
  Modifier:   ControllersTech.com
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
/* Includes ------------------------------------------------------------------*/
#include "waveplayer.h"

#include "AUDIO.h"
#include "File_Handling.h"
#include "fatfs.h"

static uint32_t uwVolume = 60;

/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> NO CHANGES AFTER THIS
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<*/

#define OUTPUT_DEVICE_SPEAKER 1
#define OUTPUT_DEVICE_HEADPHONE 2
#define OUTPUT_DEVICE_BOTH 3
#define OUTPUT_DEVICE_AUTO 4

static AUDIO_OUT_BufferTypeDef BufferCtl;

AUDIO_PLAYBACK_StateTypeDef AudioState;

static int16_t FilePos = 0;

FILELIST_FileTypeDef FileList;

WAVE_FormatTypeDef WaveFormat;

FIL WavFile;

/* Private function prototypes -----------------------------------------------*/
uint8_t PlayerInit(uint32_t AudioFreq) {
  /* Initialize the Audio codec and all related peripherals (I2S, I2C,
   * IOExpander, IOs...) */
  if (AUDIO_OUT_Init(OUTPUT_DEVICE_BOTH, uwVolume, AudioFreq) != 0) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * @brief  Starts Audio streaming.
 * @param  idx: File index
 * @retval Audio error
 */
AUDIO_ErrorTypeDef AUDIO_PLAYER_Start(uint8_t idx) {
  uint bytesread;

  f_close(&WavFile);
  if (AUDIO_GetWavObjectNumber() > idx) {
    // Open WAV file
    f_open(&WavFile, (char *)FileList.file[idx].name, FA_READ);
    // Read WAV file Header
    f_read(&WavFile, &WaveFormat, sizeof(WaveFormat), &bytesread);

    /*Adjust the Audio frequency */
    PlayerInit(WaveFormat.SampleRate);

    BufferCtl.state = BUFFER_OFFSET_NONE;

    /* Get Data from USB Flash Disk */
    f_lseek(&WavFile, 0);

    /* Fill whole buffer at first time */
    if (f_read(&WavFile, &BufferCtl.buff[0], AUDIO_OUT_BUFFER_SIZE,
               (void *)&bytesread) == FR_OK) {
      AudioState = AUDIO_STATE_PLAY;
      if (bytesread != 0) {
        AUDIO_OUT_Play((uint16_t *)&BufferCtl.buff[0], AUDIO_OUT_BUFFER_SIZE);
        BufferCtl.fptr = bytesread;
        return AUDIO_ERROR_NONE;
      }
    }
  }
  return AUDIO_ERROR_IO;
}

/**
 * @brief  Manages Audio process.
 * @param  None
 * @retval Audio error
 */
AUDIO_ErrorTypeDef AUDIO_PLAYER_Process(bool isLoop) {
  uint32_t bytesread;
  AUDIO_ErrorTypeDef audio_error = AUDIO_ERROR_NONE;

  switch (AudioState) {
    case AUDIO_STATE_PLAY:
      if (BufferCtl.fptr >= WaveFormat.FileSize) {
        AUDIO_OUT_Stop(CODEC_PDWN_SW);
        AudioState = AUDIO_STATE_NEXT;
      }

      if (BufferCtl.state == BUFFER_OFFSET_HALF) {
        if (f_read(&WavFile, &BufferCtl.buff[0], AUDIO_OUT_BUFFER_SIZE / 2,
                   (void *)&bytesread) != FR_OK) {
          AUDIO_OUT_Stop(CODEC_PDWN_SW);
          return AUDIO_ERROR_IO;
        }
        BufferCtl.state = BUFFER_OFFSET_NONE;
        BufferCtl.fptr += bytesread;
      }

      if (BufferCtl.state == BUFFER_OFFSET_FULL) {
        if (f_read(&WavFile, &BufferCtl.buff[AUDIO_OUT_BUFFER_SIZE / 2],
                   AUDIO_OUT_BUFFER_SIZE / 2, (void *)&bytesread) != FR_OK) {
          AUDIO_OUT_Stop(CODEC_PDWN_SW);
          return AUDIO_ERROR_IO;
        }

        BufferCtl.state = BUFFER_OFFSET_NONE;
        BufferCtl.fptr += bytesread;
      }
      break;

    case AUDIO_STATE_STOP:
      AUDIO_OUT_Stop(CODEC_PDWN_SW);
      AudioState = AUDIO_STATE_IDLE;
      audio_error = AUDIO_ERROR_IO;
      break;

    case AUDIO_STATE_NEXT:
      if (++FilePos >= AUDIO_GetWavObjectNumber()) {
        if (isLoop) {
          FilePos = 0;
        } else {
          AudioState = AUDIO_STATE_STOP;
        }
      }
      AUDIO_OUT_Stop(CODEC_PDWN_SW);
      AUDIO_PLAYER_Start(FilePos);
      break;

    case AUDIO_STATE_PREVIOUS:
      if (--FilePos < 0) {
        FilePos = AUDIO_GetWavObjectNumber() - 1;
      }
      AUDIO_OUT_Stop(CODEC_PDWN_SW);
      AUDIO_PLAYER_Start(FilePos);
      break;

    case AUDIO_STATE_PAUSE:
      AUDIO_OUT_Pause();
      //HAL_Delay(20);
      AudioState = AUDIO_STATE_WAIT;
      break;

    case AUDIO_STATE_RESUME:
      AUDIO_OUT_Resume();
      //HAL_Delay(20);
      AudioState = AUDIO_STATE_PLAY;
      break;

    case AUDIO_STATE_VOLUME_UP:
      if (uwVolume <= 90) {
        uwVolume += 10;
      }
      AUDIO_OUT_SetVolume(uwVolume);
      //HAL_Delay(20);
      AudioState = AUDIO_STATE_PLAY;
      break;

    case AUDIO_STATE_VOLUME_DOWN:
      if (uwVolume >= 10) {
        uwVolume -= 10;
      }
      AUDIO_OUT_SetVolume(uwVolume);
      //HAL_Delay(20);
      AudioState = AUDIO_STATE_PLAY;
      break;

    case AUDIO_STATE_WAIT:
    case AUDIO_STATE_IDLE:
    case AUDIO_STATE_INIT:
    default:
      /* Do Nothing */
      break;
  }
  return audio_error;
}

/**
 * @brief  Stops Audio streaming.
 * @param  None
 * @retval Audio error
 */
AUDIO_ErrorTypeDef AUDIO_PLAYER_Stop(void) {
  AudioState = AUDIO_STATE_STOP;
  FilePos = 0;

  AUDIO_OUT_Stop(CODEC_PDWN_SW);
  f_close(&WavFile);
  return AUDIO_ERROR_NONE;
}

/**
 * @brief  Calculates the remaining file size and new position of the pointer.
 * @param  None
 * @retval None
 */
void AUDIO_OUT_TransferComplete_CallBack(void) {
  if (AudioState == AUDIO_STATE_PLAY) {
    BufferCtl.state = BUFFER_OFFSET_FULL;
  }
}

/**
 * @brief  Manages the DMA Half Transfer complete interrupt.
 * @param  None
 * @retval None
 */
void AUDIO_OUT_HalfTransfer_CallBack(void) {
  if (AudioState == AUDIO_STATE_PLAY) {
    BufferCtl.state = BUFFER_OFFSET_HALF;
  }
}

// JIA change
uint32_t get_uwVolume(void) { return uwVolume; }
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
