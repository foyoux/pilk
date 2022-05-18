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

/*
 * https://docs.python.org/zh-cn/3/c-api/intro.html#include-files
 * 官方推荐总是定义 **PY_SSIZE_T_CLEAN** 宏
 * */
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include "silk/pilk_encode.h"
#include "silk/pilk_decode.h"

// 声明自定义错误
PyObject *PilkError;

void SKP_assert(int b) {
//    if (!b) {
//        PyErr_SetString(PilkError, "silk file error");
//    }
}

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