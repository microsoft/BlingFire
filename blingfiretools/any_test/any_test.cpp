/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAAllocator.h"
#include "FAFsmConst.h"
#include "FAUtils.h"
#include "FAException.h"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <assert.h>


#ifdef WIN32
#define FALoadLibrary(fileName) ::LoadLibrary(fileName)
#define FAGetProcAddress(handle, fnName) ::GetProcAddress((HMODULE)handle, fnName)
#define FAFreeLibrary(handle) ::FreeLibrary((HMODULE)handle)
const char * g_BlingFireModuleName = "./blingfiretokdll.dll";
#else
#include <dlfcn.h>
#define FALoadLibrary(fileName) ::dlopen(fileName, RTLD_LAZY)
#define FAGetProcAddress(handle, fnName) ::dlsym(handle, fnName)
#define FAFreeLibrary(handle) ::dlclose(handle)
const char * g_BlingFireModuleName = "./libblingfiretokdll.so";
#endif


using namespace BlingFire;

const char * __PROG__ = "";

typedef void* (__cdecl* _TLoadModelPtr)(const char *);
_TLoadModelPtr g_LoadModelPtr = NULL;

typedef const int (__cdecl* _TNormalizeSpacesPtr)(const char * , int , char * , const int , const int);
_TNormalizeSpacesPtr g_NormalizeSpacesPtr = NULL;

typedef const int (__cdecl* _TTextToIdsPtr)(void*, const char*, int, int32_t*,const int, const int);
_TTextToIdsPtr g_TextToIdsPtr = NULL;

typedef const int (__cdecl* _TTextToIdsWithOffsetsPtr)(void*, const char*, int, int32_t*, int*, int*, const int, const int);
_TTextToIdsWithOffsetsPtr g_TextToIdsWithOffsetsPtr = NULL;

typedef int (__cdecl* _TFreeModel)(void* ModelPtr);
_TFreeModel g_FreeModelPtr = NULL;

typedef int (__cdecl* _TTextToHashes)(const char *, int, int32_t *, const int, int, int);
_TTextToHashes g_TextToHashesPtr = NULL;

void* g_Module = NULL;

int __cdecl main (int argc, char ** argv)
{
    __PROG__ = argv [0];

    --argc, ++argv;

    ::FAIOSetup ();

    try {

        // load library and get the function pointers
        g_Module = FALoadLibrary(g_BlingFireModuleName);
        if (NULL == g_Module)
        {
            std::cerr << "ERROR: Failed to load libblingfiretokdll.so" << std::endl;
            return false;
        }

        g_LoadModelPtr = (_TLoadModelPtr)FAGetProcAddress(g_Module, "LoadModel");
        if (NULL == g_LoadModelPtr)
        {
            std::cerr << "ERROR: Cannot get address of LoadModel function" << std::endl;
            return false;
        }

        g_NormalizeSpacesPtr = (_TNormalizeSpacesPtr)FAGetProcAddress(g_Module, "NormalizeSpaces");
        if (NULL == g_NormalizeSpacesPtr)
        {
            std::cerr << "ERROR: Cannot get address of NormalizeSpaces function" << std::endl;
            return false;
        }

        g_TextToIdsPtr = (_TTextToIdsPtr)FAGetProcAddress(g_Module, "TextToIds");
        if (NULL == g_TextToIdsPtr)
        {
            std::cerr << "ERROR: Cannot get address of TextToIds function" << std::endl;
            return false;
        }

        g_TextToIdsWithOffsetsPtr = (_TTextToIdsWithOffsetsPtr)FAGetProcAddress(g_Module, "TextToIdsWithOffsets");
        if (NULL == g_TextToIdsWithOffsetsPtr)
        {
            std::cerr << "ERROR: Cannot get address of TextToIdsWithOffsets function" << std::endl;
            return false;
        }

        g_FreeModelPtr = (_TFreeModel)FAGetProcAddress(g_Module, "FreeModel");
        if (NULL == g_FreeModelPtr)
        {
            std::cerr << "ERROR: Cannot get address of FreeModel function" << std::endl;
            return false;
        }

        g_TextToHashesPtr = (_TTextToHashes)FAGetProcAddress(g_Module, "TextToHashes");
        if (NULL == g_TextToHashesPtr)
        {
            std::cerr << "ERROR: Cannot get address of TextToHashes function" << std::endl;
            return false;
        }

        // tests

        void* hModel = (*g_LoadModelPtr)("laser100k.bin");

        const int MaxIdCount = 128;
        int Ids [MaxIdCount];
        int Starts [MaxIdCount];
        int Ends [MaxIdCount];

        std::string in1 ("performance report‍‍‍‍‍‍‍‍\xe2\x80\x8d\xe2\x80\x8d\xe2\x80\x8d");

        char norm_out [FALimits::MaxWordLen];
        const int norm_len = (*g_NormalizeSpacesPtr)(in1.c_str(), (int)in1.length(), norm_out, FALimits::MaxWordLen, 0x20);

        int IdCount = (*g_TextToIdsPtr)(hModel, in1.c_str(), (int)in1.length(), Ids, MaxIdCount, 3);
        for(int i = 0; i < IdCount; ++i) {
            std::cout << Ids[i] << ' ';
        }
        std::cout << std::endl;

        IdCount = (*g_TextToIdsWithOffsetsPtr)(hModel, in1.c_str(), (int)in1.length(), Ids, Starts, Ends, MaxIdCount, 3);
        for(int i = 0; i < IdCount; ++i) {
            std::cout << Ids[i] << ' ';
        }
        std::cout << std::endl;

        for(int i = 0; i < IdCount; ++i) {

            const int _from = Starts[i];
            const int _to = Ends[i];

            if (0 <= _from && 0 <= _to && _to >= _from) {
                std::string s (in1.c_str() + _from, _to - _from + 1);
                std::cout << s << ':' << Ids[i] << ' ';
            } else {
                std::cout << "UNK" << ':' << Ids[i] << ' ';
            }
        }
        std::cout << std::endl;

        (*g_FreeModelPtr)(hModel);


        // test text to hashes
        //                  0         1         2
        //                  012345678901234567890123456789
        const char * pIn = "246694 4 4 4 4 4 4 4 4 4 4 4 4";
        const int ngram_order = 6;
        int output_buff [30*6];

        const int ActualCount = (*g_TextToHashesPtr)(pIn, (int)strlen(pIn), output_buff, (int)(sizeof(output_buff)/sizeof(output_buff[0])), ngram_order, 80000000);

        std::cout << "As an array:" << std::endl;
        for(int i = 0; i < ActualCount; ++i)
        {
            std::cout << output_buff[i] << ' ';
        }
        std::cout << std::endl;


        // reinterpret output_buff as a two dimentional C array, to make sure all dimantions are done right
        int (&m)[6][13] = *reinterpret_cast<int (*)[6][13]>(&output_buff[0]);

        std::cout << "As a matrix:" << std::endl;
        for(int i = 0; i < 6; ++i)
        {
            for(int j = 0; j < 13; ++j)
            {
                std::cout << m[i][j] << ' ';
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;

        // unload the .so/dll file
        FAFreeLibrary(g_Module);

    } catch (const FAException & e) {

        const char * const pErrMsg = e.GetErrMsg ();
        const char * const pFile = e.GetSourceName ();
        const int Line = e.GetSourceLine ();

        std::cerr << "ERROR: " << pErrMsg << " in " << pFile \
            << " at line " << Line << " in program " << __PROG__ << '\n';

        return 2;

    } catch (...) {

        std::cerr << "ERROR: Unknown error in program " << __PROG__ << '\n';

        return 1;
    }

    // print out memory leaks, if any
    FAPrintLeaks(&g_alloc, std::cerr);

    return 0;
}
