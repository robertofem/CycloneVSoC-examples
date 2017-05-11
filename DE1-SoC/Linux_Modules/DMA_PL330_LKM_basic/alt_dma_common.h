/******************************************************************************
*
* Copyright 2013 Altera Corporation. All Rights Reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* 
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* 
* 3. Neither the name of the copyright holder nor the names of its contributors
* may be used to endorse or promote products derived from this software without
* specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
* 
******************************************************************************/

/*
 * $Id: //acds/rel/16.1/embedded/ip/hps/altera_hps/hwlib/include/alt_dma_common.h#1 $
 */

#ifndef __ALT_DMA_COMMON_H__
#define __ALT_DMA_COMMON_H__

#include "alt_dma_periph_cv_av.h"

#ifdef __cplusplus
extern "C"
{
#endif  /* __cplusplus */

/*!
 * \addtogroup ALT_DMA_COMMON DMA Controller Common API Definitions
 *
 * This module contains the common definitions for the DMA controller related
 * APIs.
 *
 * @{
 */

/*!
 * This type definition enumerates the DMA controller channel threads.
 */
typedef enum ALT_DMA_CHANNEL_e
{
    ALT_DMA_CHANNEL_0 = 0, /*!< DMA Channel Thread 0 */
    ALT_DMA_CHANNEL_1 = 1, /*!< DMA Channel Thread 1 */
    ALT_DMA_CHANNEL_2 = 2, /*!< DMA Channel Thread 2 */
    ALT_DMA_CHANNEL_3 = 3, /*!< DMA Channel Thread 3 */
    ALT_DMA_CHANNEL_4 = 4, /*!< DMA Channel Thread 4 */
    ALT_DMA_CHANNEL_5 = 5, /*!< DMA Channel Thread 5 */
    ALT_DMA_CHANNEL_6 = 6, /*!< DMA Channel Thread 6 */
    ALT_DMA_CHANNEL_7 = 7  /*!< DMA Channel Thread 7 */
}
ALT_DMA_CHANNEL_t;

/*!
 * This type enumerates the DMA security state options available.
 */
typedef enum ALT_DMA_SECURITY_e
{
    ALT_DMA_SECURITY_DEFAULT   = 0, /*!< Use the default security value (e.g. reset default) */
    ALT_DMA_SECURITY_SECURE    = 1, /*!< Secure */
    ALT_DMA_SECURITY_NONSECURE = 2  /*!< Non-secure */
}
ALT_DMA_SECURITY_t;

/*!
 * This type definition enumerates the DMA event-interrupt resources.
 */
typedef enum ALT_DMA_EVENT_e
{
    ALT_DMA_EVENT_0     = 0, /*!< DMA Event 0 */
    ALT_DMA_EVENT_1     = 1, /*!< DMA Event 1 */
    ALT_DMA_EVENT_2     = 2, /*!< DMA Event 2 */
    ALT_DMA_EVENT_3     = 3, /*!< DMA Event 3 */
    ALT_DMA_EVENT_4     = 4, /*!< DMA Event 4 */
    ALT_DMA_EVENT_5     = 5, /*!< DMA Event 5 */
    ALT_DMA_EVENT_6     = 6, /*!< DMA Event 6 */
    ALT_DMA_EVENT_7     = 7, /*!< DMA Event 7 */
    ALT_DMA_EVENT_ABORT = 8  /*!< DMA Abort Event */
}
ALT_DMA_EVENT_t;

/*!
 * @}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ALT_DMA_COMMON_H__ */
