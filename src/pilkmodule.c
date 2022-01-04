/***********************************************************************
Copyright (c) 2006-2012, Skype Limited. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, (subject to the limitations in the disclaimer below)
are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
- Neither the name of Skype Limited, nor the names of specific
contributors, may be used to endorse or promote products derived from
this software without specific prior written permission.
NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED
BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************/

#define PY_SSIZE_T_CLEAN

#include <Python.h>

// 声明自定义错误
static PyObject *PilkError;

void SKP_assert(int b) {
//    if (!b) {
//        PyErr_SetString(PilkError, "silk file error");
//    }
}

static PyObject *
silk_encode(PyObject *module, PyObject *args, PyObject *keyword_args);

static PyObject *
silk_decode(PyObject *module, PyObject *args, PyObject *keyword_args);

// https://docs.python.org/zh-cn/3/c-api/intro.html#c.PyDoc_STRVAR
PyDoc_STRVAR(encode_doc, "encode pcm to silk");
PyDoc_STRVAR(decode_doc, "decode silk to pcm");

// 模块方法表
static PyMethodDef PilkMethods[] = {
        {"encode", (PyCFunction) (void (*)(void)) silk_encode, METH_VARARGS | METH_KEYWORDS,
                        encode_doc},
        {"decode", (PyCFunction) (void (*)(void)) silk_decode, METH_VARARGS | METH_KEYWORDS,
                        decode_doc},
        {NULL, NULL, 0, NULL}
};

// 模块定义
static struct PyModuleDef moduleDef = {
        PyModuleDef_HEAD_INIT,
        "_pilk",
        "python silk library",
        -1,
        PilkMethods
};

PyMODINIT_FUNC
PyInit__pilk(void) {
    PyObject *m;

    m = PyModule_Create(&moduleDef);
    if (m == NULL)
        return NULL;

    PilkError = PyErr_NewException("pilk.error", NULL, NULL);
    Py_XINCREF(PilkError);
    if (PyModule_AddObject(m, "error", PilkError) < 0) {
        Py_XDECREF(PilkError);
        Py_CLEAR(PilkError);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

/*****************************/
/* Silk encoder test program */
/*****************************/

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE    1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SKP_Silk_SDK_API.h"

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

static PyObject *
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

    return Py_BuildValue("Si", bitOutFileName, (int) filetime);
}



/*****************************/
/* Silk decoder test program */
/*****************************/

#include "SKP_Silk_SigProc_FIX.h"

/* Define codec specific settings should be moved to h file */
#define DECODE_MAX_BYTES_PER_FRAME     1024
#define MAX_INPUT_FRAMES        5
#define MAX_FRAME_LENGTH        480
#define FRAME_LENGTH_MS         20
#define MAX_API_FS_KHZ          48
#define MAX_LBRR_DELAY          2

/* Seed for the random number generator, which is used for simulating packet loss */
static SKP_int32 rand_seed = 1;


static PyObject *
silk_decode(PyObject *Py_UNUSED(module), PyObject *args, PyObject *keyword_args) {

    static char *kwlist[] = {
            "silk",
            "pcm",
            "pcm_rate",
            "packet_loss",
            NULL
    };

    double filetime;
    size_t counter;
    SKP_int32 totPackets, i, k;
    SKP_int16 ret, len, tot_len;
    SKP_int16 nBytes;
    SKP_uint8 payload[DECODE_MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES * (MAX_LBRR_DELAY + 1)];
    SKP_uint8 *payloadEnd = NULL, *payloadToDec = NULL;
    SKP_uint8 FECpayload[DECODE_MAX_BYTES_PER_FRAME * MAX_INPUT_FRAMES], *payloadPtr;
    SKP_int16 nBytesFEC;
    SKP_int16 nBytesPerPacket[MAX_LBRR_DELAY + 1], totBytes;
    SKP_int16 out[((FRAME_LENGTH_MS * MAX_API_FS_KHZ) << 1) * MAX_INPUT_FRAMES], *outPtr;
    PyObject *bitInFileName;
    PyObject *speechOutFileName;
    FILE *bitInFile, *speechOutFile;
    SKP_int32 packetSize_ms = 0, API_Fs_Hz = 0;
    SKP_int32 decSizeBytes;
    void *psDec;
    SKP_float loss_prob;
    SKP_int32 frames, lost;
    SKP_SILK_SDK_DecControlStruct DecControl;


    /* default settings */
    loss_prob = 0.0f;

    /* get arguments */
    if (!PyArg_ParseTupleAndKeywords(args, keyword_args, "UU|ii", kwlist,
                                     &bitInFileName, &speechOutFileName, &API_Fs_Hz, &lost)) {
        // 返回 null，无需手动设置错误，PyArg_ParseTupleAndKeywords 会自动处理
        return NULL;
    }

    /* Open files */
    bitInFile = _Py_fopen_obj(bitInFileName, "rb");
    if (bitInFile == NULL) {

        return PyErr_Format(PyExc_OSError, "Error: could not open input file %s", bitInFileName);
    }

    /* Check Silk header */
    {
        char header_buf[50];
        fread(header_buf, sizeof(char), 1, bitInFile);
        header_buf[strlen("")] = '\0'; /* Terminate with a null character */
        if (strcmp(header_buf, "") != 0) {
            counter = fread(header_buf, sizeof(char), strlen("!SILK_V3"), bitInFile);
            header_buf[strlen("!SILK_V3")] = '\0'; /* Terminate with a null character */
            if (strcmp(header_buf, "!SILK_V3") != 0) {
                /* Non-equal strings */

                return PyErr_Format(PilkError, "Error: Wrong Header %s", header_buf);
            }
        } else {
            counter = fread(header_buf, sizeof(char), strlen("#!SILK_V3"), bitInFile);
            header_buf[strlen("#!SILK_V3")] = '\0'; /* Terminate with a null character */
            if (strcmp(header_buf, "#!SILK_V3") != 0) {
                /* Non-equal strings */

                return PyErr_Format(PilkError, "Error: Wrong Header %s", header_buf);
            }
        }
    }

//    speechOutFile = fopen(speechOutFileName, "wb");
    speechOutFile = _Py_fopen_obj(speechOutFileName, "wb");
    if (speechOutFile == NULL) {

        return PyErr_Format(PyExc_OSError, "Error: could not open output file %s", speechOutFileName);
    }

    /* Set the samplingrate that is requested for the output */
    if (API_Fs_Hz == 0) {
        DecControl.API_sampleRate = 24000;
    } else {
        DecControl.API_sampleRate = API_Fs_Hz;
    }

    /* Initialize to one frame per packet, for proper concealment before first packet arrives */
    DecControl.framesPerPacket = 1;

    /* Create decoder */
    SKP_Silk_SDK_Get_Decoder_Size(&decSizeBytes);

    psDec = malloc(decSizeBytes);

    /* Reset decoder */
    SKP_Silk_SDK_InitDecoder(psDec);

    totPackets = 0;
    payloadEnd = payload;

    /* Simulate the jitter buffer holding MAX_FEC_DELAY packets */
    for (i = 0; i < MAX_LBRR_DELAY; i++) {
        /* Read payload size */
        counter = fread(&nBytes, sizeof(SKP_int16), 1, bitInFile);
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( &nBytes, 1 );
#endif
        /* Read payload */
        counter = fread(payloadEnd, sizeof(SKP_uint8), nBytes, bitInFile);

        if ((SKP_int16) counter < nBytes) {
            break;
        }
        nBytesPerPacket[i] = nBytes;
        payloadEnd += nBytes;
        totPackets++;
    }

    while (1) {
        /* Read payload size */
        counter = fread(&nBytes, sizeof(SKP_int16), 1, bitInFile);
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( &nBytes, 1 );
#endif
        if (nBytes < 0 || counter < 1) {
            break;
        }

        /* Read payload */
        counter = fread(payloadEnd, sizeof(SKP_uint8), nBytes, bitInFile);
        if ((SKP_int16) counter < nBytes) {
            break;
        }

        /* Simulate losses */
        rand_seed = SKP_RAND(rand_seed);
        if ((((float) ((rand_seed >> 16) + (1 << 15))) / 65535.0f >= (loss_prob / 100.0f)) && (counter > 0)) {
            nBytesPerPacket[MAX_LBRR_DELAY] = nBytes;
            payloadEnd += nBytes;
        } else {
            nBytesPerPacket[MAX_LBRR_DELAY] = 0;
        }

        if (nBytesPerPacket[0] == 0) {
            /* Indicate lost packet */
            lost = 1;

            /* Packet loss. Search after FEC in next packets. Should be done in the jitter buffer */
            payloadPtr = payload;
            for (i = 0; i < MAX_LBRR_DELAY; i++) {
                if (nBytesPerPacket[i + 1] > 0) {
                    SKP_Silk_SDK_search_for_LBRR(payloadPtr, nBytesPerPacket[i + 1], (i + 1), FECpayload, &nBytesFEC);
                    if (nBytesFEC > 0) {
                        payloadToDec = FECpayload;
                        nBytes = nBytesFEC;
                        lost = 0;
                        break;
                    }
                }
                payloadPtr += nBytesPerPacket[i + 1];
            }
        } else {
            lost = 0;
            nBytes = nBytesPerPacket[0];
            payloadToDec = payload;
        }

        /* Silk decoder */
        outPtr = out;
        tot_len = 0;

        if (lost == 0) {
            /* No Loss: Decode all frames in the packet */
            frames = 0;
            do {
                /* Decode 20 ms */
                SKP_Silk_SDK_Decode(psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len);

                frames++;
                outPtr += len;
                tot_len += len;
                if (frames > MAX_INPUT_FRAMES) {
                    /* Hack for corrupt stream that could generate too many frames */
                    outPtr = out;
                    tot_len = 0;
                    frames = 0;
                }
                /* Until last 20 ms frame of packet has been decoded */
            } while (DecControl.moreInternalDecoderFrames);
        } else {
            /* Loss: Decode enough frames to cover one packet duration */
            for (i = 0; i < DecControl.framesPerPacket; i++) {
                /* Generate 20 ms */
                SKP_Silk_SDK_Decode(psDec, &DecControl, 1, payloadToDec, nBytes, outPtr, &len);
                outPtr += len;
                tot_len += len;
            }
        }

        packetSize_ms = tot_len / (DecControl.API_sampleRate / 1000);
        totPackets++;

        /* Write output to file */
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( out, tot_len );
#endif
        fwrite(out, sizeof(SKP_int16), tot_len, speechOutFile);

        /* Update buffer */
        totBytes = 0;
        for (i = 0; i < MAX_LBRR_DELAY; i++) {
            totBytes += nBytesPerPacket[i + 1];
        }
        /* Check if the received totBytes is valid */
        if (totBytes < 0 || totBytes > sizeof(payload)) {

            return PyErr_Format(PilkError, "Packets decoded: %d", totPackets);
        }
        SKP_memmove(payload, &payload[nBytesPerPacket[0]], totBytes * sizeof(SKP_uint8));
        payloadEnd -= nBytesPerPacket[0];
        SKP_memmove(nBytesPerPacket, &nBytesPerPacket[1], MAX_LBRR_DELAY * sizeof(SKP_int16));

    }

    /* Empty the recieve buffer */
    for (k = 0; k < MAX_LBRR_DELAY; k++) {
        if (nBytesPerPacket[0] == 0) {
            /* Indicate lost packet */
            lost = 1;

            /* Packet loss. Search after FEC in next packets. Should be done in the jitter buffer */
            payloadPtr = payload;
            for (i = 0; i < MAX_LBRR_DELAY; i++) {
                if (nBytesPerPacket[i + 1] > 0) {
                    SKP_Silk_SDK_search_for_LBRR(payloadPtr, nBytesPerPacket[i + 1], (i + 1), FECpayload, &nBytesFEC);
                    if (nBytesFEC > 0) {
                        payloadToDec = FECpayload;
                        nBytes = nBytesFEC;
                        lost = 0;
                        break;
                    }
                }
                payloadPtr += nBytesPerPacket[i + 1];
            }
        } else {
            lost = 0;
            nBytes = nBytesPerPacket[0];
            payloadToDec = payload;
        }

        /* Silk decoder */
        outPtr = out;
        tot_len = 0;

        if (lost == 0) {
            /* No loss: Decode all frames in the packet */
            frames = 0;
            do {
                /* Decode 20 ms */
                ret = SKP_Silk_SDK_Decode(psDec, &DecControl, 0, payloadToDec, nBytes, outPtr, &len);
                if (ret) {
                    printf("\nSKP_Silk_SDK_Decode returned %d", ret);
                }

                frames++;
                outPtr += len;
                tot_len += len;
                if (frames > MAX_INPUT_FRAMES) {
                    /* Hack for corrupt stream that could generate too many frames */
                    outPtr = out;
                    tot_len = 0;
                    frames = 0;
                }
                /* Until last 20 ms frame of packet has been decoded */
            } while (DecControl.moreInternalDecoderFrames);
        } else {
            /* Loss: Decode enough frames to cover one packet duration */

            /* Generate 20 ms */
            for (i = 0; i < DecControl.framesPerPacket; i++) {
                SKP_Silk_SDK_Decode(psDec, &DecControl, 1, payloadToDec, nBytes, outPtr, &len);
                outPtr += len;
                tot_len += len;
            }
        }

        packetSize_ms = tot_len / (DecControl.API_sampleRate / 1000);
        totPackets++;

        /* Write output to file */
#ifdef _SYSTEM_IS_BIG_ENDIAN
        swap_endian( out, tot_len );
#endif
        fwrite(out, sizeof(SKP_int16), tot_len, speechOutFile);

        /* Update Buffer */
        totBytes = 0;
        for (i = 0; i < MAX_LBRR_DELAY; i++) {
            totBytes += nBytesPerPacket[i + 1];
        }

        /* Check if the received totBytes is valid */
        if (totBytes < 0 || totBytes > sizeof(payload)) {

            return PyErr_Format(PilkError, "Packets decoded: %d", totPackets);
        }

        SKP_memmove(payload, &payload[nBytesPerPacket[0]], totBytes * sizeof(SKP_uint8));
        payloadEnd -= nBytesPerPacket[0];
        SKP_memmove(nBytesPerPacket, &nBytesPerPacket[1], MAX_LBRR_DELAY * sizeof(SKP_int16));

    }

    /* Free decoder */
    free(psDec);

    /* Close files */
    fclose(speechOutFile);
    fclose(bitInFile);

    filetime = totPackets * 1e-3 * packetSize_ms;

    return Py_BuildValue("Si", speechOutFileName, (int) filetime);
}
