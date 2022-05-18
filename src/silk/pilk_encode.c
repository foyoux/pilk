//
// Created by foyou on 2022/5/18.
//
#include "pilk_encode.h"


/*****************************/
/* Silk encoder test program */
/*****************************/

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE    1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define codec specific settings */
#define ENCODE_MAX_BYTES_PER_FRAME     250 // Equals peak bitrate of 100 kbps
#define MAX_INPUT_FRAMES        5
#define FRAME_LENGTH_MS         20
#define MAX_API_FS_KHZ          48

#ifdef _SYSTEM_IS_BIG_ENDIAN
/* Function to convert a little endian int16 to a */
/* big endian int16 or vica verca                 */
void swap_endian(
    SKP_int16       vec[],              /*  I/O array of */
    SKP_int         len                 /*  I   length      */
)
{
    SKP_int i;
    SKP_int16 tmp;
    SKP_uint8 *p1, *p2;

    for( i = 0; i < len; i++ ){
        tmp = vec[ i ];
        p1 = (SKP_uint8 *)&vec[ i ]; p2 = (SKP_uint8 *)&tmp;
        p1[ 0 ] = p2[ 1 ]; p1[ 1 ] = p2[ 0 ];
    }
}
#endif

// 声明自定义错误
PyObject *PilkError;

PyObject *
silk_encode(PyObject *Py_UNUSED(module), PyObject *args, PyObject *keyword_args) {

    SKP_assert(0);
    static char *kwlist[] = {
            "pcm",
            "silk",
            "pcm_rate",
            "silk_rate",
            "tencent",
            "max_rate",
            "complexity",
            "packet_size",
            "packet_loss",
            "use_in_band_fec",
            "use_dtx",
            NULL
    };

    double filetime;
    size_t counter;
    SKP_int32 k, totPackets, totActPackets, ret;
    SKP_int16 nBytes;
    double sumBytes, sumActBytes, nrg;
    SKP_uint8 payload[ENCODE_MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES];
    SKP_int16 in[FRAME_LENGTH_MS * MAX_API_FS_KHZ * MAX_INPUT_FRAMES];
    PyObject *speechInFileName;
    PyObject *bitOutFileName;
    FILE *bitOutFile, *speechInFile;
    SKP_int32 encSizeBytes;
    void *psEnc;
#ifdef _SYSTEM_IS_BIG_ENDIAN
    SKP_int16 nBytes_LE;
#endif
    /* default settings */
    SKP_int32 API_fs_Hz = 24000;
    SKP_int32 max_internal_fs_Hz = 0;
    SKP_int32 targetRate_bps = -1;
    SKP_int32 smplsSinceLastPacket, packetSize_ms = 20;
    SKP_int32 frameSizeReadFromFile_ms = 20;
    SKP_int32 packetLoss_perc = 0;
#if LOW_COMPLEXITY_ONLY
    SKP_int32 complexity_mode = 0;
#else
    SKP_int32 complexity_mode = 2;
#endif
    SKP_int32 DTX_enabled = 0, INBandFEC_enabled = 0, tencent = 0;
    SKP_SILK_SDK_EncControlStruct encControl; // Struct for input to encoder
    SKP_SILK_SDK_EncControlStruct encStatus;  // Struct for status of encoder

    /*解析参数*/
    if (!PyArg_ParseTupleAndKeywords(args, keyword_args, "UU|iipiiiipp", kwlist,
                                     &speechInFileName, &bitOutFileName, &API_fs_Hz, &targetRate_bps, &tencent,
                                     &max_internal_fs_Hz, &complexity_mode, &packetSize_ms,
                                     &packetLoss_perc, &INBandFEC_enabled, &DTX_enabled)) {
        // 返回 null，无需手动设置错误，PyArg_ParseTupleAndKeywords 会自动处理
        return NULL;
    }

    // 处理 silk_rate
    if (targetRate_bps == -1) {
        targetRate_bps = API_fs_Hz;
    }

    /* If no max internal is specified, set to minimum of API fs and 24 kHz */
    if (max_internal_fs_Hz == 0) {
        max_internal_fs_Hz = 24000;
        if (API_fs_Hz < max_internal_fs_Hz) {
            max_internal_fs_Hz = API_fs_Hz;
        }
    }

    /* Open files */
//    speechInFile = fopen(speechInFileName, "rb");
    speechInFile = _Py_fopen_obj(speechInFileName, "rb");
    if (speechInFile == NULL) {

        return PyErr_Format(PyExc_OSError, "Error: could not open input file %s", speechInFileName);
    }


//    bitOutFile = fopen(bitOutFileName, "wb");
    bitOutFile = _Py_fopen_obj(bitOutFileName, "wb");
    if (bitOutFile == NULL) {

        return PyErr_Format(PyExc_OSError, "Error: could not open output file %s", bitOutFileName);
    }


    /* Add Silk header to stream */
    {
        if (tencent) {
            static const char Tencent_break[] = "";
            fwrite(Tencent_break, sizeof(char), strlen(Tencent_break), bitOutFile);
        }

        static const char Silk_header[] = "#!SILK_V3";
        fwrite(Silk_header, sizeof(char), strlen(Silk_header), bitOutFile);
    }

    /* Create Encoder */
    ret = SKP_Silk_SDK_Get_Encoder_Size(&encSizeBytes);
    if (ret) {

        return PyErr_Format(PilkError, "Error: SKP_Silk_create_encoder returned %d", ret);
    }


    psEnc = malloc(encSizeBytes);

    /* Reset Encoder */
    ret = SKP_Silk_SDK_InitEncoder(psEnc, &encStatus);
    if (ret) {

        return PyErr_Format(PilkError, "Error: SKP_Silk_reset_encoder returned %d", ret);
    }

    /* Set Encoder parameters */
    encControl.API_sampleRate = API_fs_Hz;
    encControl.maxInternalSampleRate = max_internal_fs_Hz;
    encControl.packetSize = (packetSize_ms * API_fs_Hz) / 1000;
    encControl.packetLossPercentage = packetLoss_perc;
    encControl.useInBandFEC = INBandFEC_enabled;
    encControl.useDTX = DTX_enabled;
    encControl.complexity = complexity_mode;
    encControl.bitRate = (targetRate_bps > 0 ? targetRate_bps : 0);

    /*校验参数*/
    if ((API_fs_Hz != 8000) &&
        (API_fs_Hz != 12000) &&
        (API_fs_Hz != 16000) &&
        (API_fs_Hz != 24000) &&
        (API_fs_Hz != 32000) &&
        (API_fs_Hz != 44100) &&
        (API_fs_Hz != 48000)) {

        PyErr_SetString(PyExc_ValueError,
                        "SKP_SILK_ENC_FS_NOT_SUPPORTED: pcm_rate must be in [8000, 12000, 16000, 24000, 32000, 44100, 48000]");
        return NULL;
    }

    if ((max_internal_fs_Hz != 8000) &&
        (max_internal_fs_Hz != 12000) &&
        (max_internal_fs_Hz != 16000) &&
        (max_internal_fs_Hz != 24000)) {
        PyErr_SetString(PyExc_ValueError,
                        "SKP_SILK_ENC_FS_NOT_SUPPORTED: max_rate must be in [8000, 12000, 16000, 24000]");
        return NULL;
    }

    totPackets = 0;
    totActPackets = 0;
    sumBytes = 0.0;
    sumActBytes = 0.0;
    smplsSinceLastPacket = 0;

    while (1) {
        /* Read input from file */
        counter = fread(in, sizeof(SKP_int16), (frameSizeReadFromFile_ms * API_fs_Hz) / 1000, speechInFile);
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( in, counter );
#endif
        if ((SKP_int) counter < ((frameSizeReadFromFile_ms * API_fs_Hz) / 1000)) {
            break;
        }

        /* max payload size */
        nBytes = ENCODE_MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES;


        /* Silk Encoder */
        ret = SKP_Silk_SDK_Encode(psEnc, &encControl, in, (SKP_int16) counter, payload, &nBytes);
        if (ret) {

            return PyErr_Format(PilkError, "SKP_Silk_Encode returned %d, pcm file error.", ret);
        }


        /* Get packet size */
        packetSize_ms = (SKP_int) ((1000 * (SKP_int32) encControl.packetSize) / encControl.API_sampleRate);

        smplsSinceLastPacket += (SKP_int) counter;

        if (((1000 * smplsSinceLastPacket) / API_fs_Hz) == packetSize_ms) {
            /* Sends a dummy zero size packet in case of DTX period  */
            /* to make it work with the decoder test program.        */
            /* In practice should be handled by RTP sequence numbers */
            totPackets++;
            sumBytes += nBytes;
            nrg = 0.0;
            for (k = 0; k < (SKP_int) counter; k++) {
                nrg += in[k] * (double) in[k];
            }
            if ((nrg / (SKP_int) counter) > 1e3) {
                sumActBytes += nBytes;
                totActPackets++;
            }

            /* Write payload size */
#ifdef _SYSTEM_IS_BIG_ENDIAN
            nBytes_LE = nBytes;
            swap_endian( &nBytes_LE, 1 );
            fwrite( &nBytes_LE, sizeof( SKP_int16 ), 1, bitOutFile );
#else
            fwrite(&nBytes, sizeof(SKP_int16), 1, bitOutFile);
#endif

            /* Write payload */
            fwrite(payload, sizeof(SKP_uint8), nBytes, bitOutFile);

            smplsSinceLastPacket = 0;

        }
    }

    /* Write dummy because it can not end with 0 bytes */
    nBytes = -1;

    /* Write payload size */
    if (!tencent) {
        fwrite(&nBytes, sizeof(SKP_int16), 1, bitOutFile);
    }

    /* Free Encoder */
    free(psEnc);

    fclose(speechInFile);
    fclose(bitOutFile);

    filetime = totPackets * 1e-3 * packetSize_ms;

    return PyLong_FromDouble(filetime);
}

