//
// Created by foyou on 2022/5/18.
//

#include "pilk_decode.h"
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

// 声明自定义错误
extern PyObject *PilkError;

PyObject *
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

    return PyLong_FromDouble(filetime);
}
