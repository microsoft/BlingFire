#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <assert.h>

namespace BlingFire 
{

// keep model data together
struct FAModelData
{
    // image of the loaded file
    FAImageDump m_Img;
    FALDB m_Ldb;

    // data and const processor for tokenization
    FAWbdConfKeeper m_Conf;
    FALexTools_t < int > m_Engine;
    bool m_hasWbd;

    // data and const processor for tokenization
    FADictConfKeeper m_DictConf;
    // indicates that a pos-dict data are present in the bin LDB file
    bool m_hasSeg;

    // Unigram LM algorithm
    FATokenSegmentationTools_1best_t < int > m_SegEngine;
    // BPE (with separate merge ranks, ID's are ranks) runtime
    FATokenSegmentationTools_1best_bpe_t < int > m_SegEngineBpe;
    // BPE (with separate merge ranks) runtime
    FATokenSegmentationTools_1best_bpe_with_merges_t < int > m_SegEngineBpeWithMerges;
    // one selected algorithm for this bin file
    const FATokenSegmentationToolsCA_t < int > * m_pAlgo;
    // indicates wether characters are bytes of the UTF-8 rather than the Unicode symbols
    bool m_useRawBytes;

    // Hyphenation / Syllabification data
    bool m_hasHy;
    FAHyphConfKeeper m_HyConf;
    FAHyphInterpreter_core_t < int > m_HyEngine;

    // id2word lexicon data
    bool m_hasI2w;
    FAStringArray_pack m_i2w;
    int m_min_token_id; // min regular token id, needed to separate special tokens
    int m_max_token_id; // max regular token id, needed to separate special tokens


    FAModelData ():
        m_hasWbd (false),
        m_hasSeg (false),
        m_pAlgo (NULL),
        m_useRawBytes (false),
        m_hasHy (false),
        m_hasI2w (false),
        m_min_token_id (0),
        m_max_token_id (FALimits::MaxArrSize)
    {}
};

// SENTENCE PIECE DELIMITER
#define __FASpDelimiter__ 0x2581

// DEFAULT HYPHEN
#define __FADefaultHyphen__ 0x2012

// WHITESPACE [\x0004-\x0020\x007F-\x009F\x00A0\x2000-\x200F\x202F\x205F\x2060\x2420\x2424\x3000\xFEFF]
#define __FAIsWhiteSpace__(C) ( \
        (C <=  0x20 || C == 0xa0   || (C >= 0x2000 && C <= 0x200f) || \
        C == 0x202f || C == 0x205f || C == 0x2060 || C == 0x2420 || \
        C == 0x2424 || C == 0x3000 || C == 0xfeff) \
    )

extern "C"
{
const int GetBlingFireTokVersion();
const int TextToSentencesWithOffsetsWithModel(const char * pInUtf8Str, int InUtf8StrByteCount,
    char * pOutUtf8Str, int * pStartOffsets, int * pEndOffsets, const int MaxOutUtf8StrByteCount,
    void * hModel);
const int TextToSentencesWithOffsets(const char * pInUtf8Str, int InUtf8StrByteCount,
    char * pOutUtf8Str, int * pStartOffsets, int * pEndOffsets, const int MaxOutUtf8StrByteCount);
const int TextToSentencesWithModel(const char * pInUtf8Str, int InUtf8StrByteCount, char * pOutUtf8Str, const int MaxOutUtf8StrByteCount, void * hModel);
const int TextToSentences(const char * pInUtf8Str, int InUtf8StrByteCount, char * pOutUtf8Str, const int MaxOutUtf8StrByteCount);
const int TextToWordsWithOffsetsWithModel(const char * pInUtf8Str, int InUtf8StrByteCount,
    char * pOutUtf8Str, int * pStartOffsets, int * pEndOffsets, const int MaxOutUtf8StrByteCount,
    void * hModel);
const int TextToWordsWithOffsets(const char * pInUtf8Str, int InUtf8StrByteCount,
    char * pOutUtf8Str, int * pStartOffsets, int * pEndOffsets, const int MaxOutUtf8StrByteCount);
const int TextToWordsWithModel(const char * pInUtf8Str, int InUtf8StrByteCount,
    char * pOutUtf8Str, const int MaxOutUtf8StrByteCount, void * hModel);
const int TextToWords(const char * pInUtf8Str, int InUtf8StrByteCount, char * pOutUtf8Str, const int MaxOutUtf8StrByteCount);
const int NormalizeSpaces(const char * pInUtf8Str, int InUtf8StrByteCount, char * pOutUtf8Str, const int MaxOutUtf8StrByteCount, const int uSpace = __FASpDelimiter__);
const int TextToHashes(const char * pInUtf8Str, int InUtf8StrByteCount, int32_t * pHashArr, const int MaxHashArrLength, int wordNgrams, int bucketSize = 2000000);
const int WordHyphenationWithModel(const char * pInUtf8Str, int InUtf8StrByteCount,
    char * pOutUtf8Str, const int MaxOutUtf8StrByteCount, void * hModel, const int uHy = __FADefaultHyphen__);
void* SetModelData(FAModelData * pNewModelData, const unsigned char * pImgBytes);
void* SetModel(const unsigned char * pImgBytes, int ModelByteCount);
void* LoadModel(const char * pszLdbFileName);
const int TextToIdsWithOffsets_wp(
        void* ModelPtr,
        const char * pInUtf8Str,
        int InUtf8StrByteCount,
        int32_t * pIdsArr, 
        int * pStartOffsets, 
        int * pEndOffsets,
        const int MaxIdsArrLength,
        const int UnkId = 0
);
const int TextToIds_wp(
        void* ModelPtr,
        const char * pInUtf8Str,
        int InUtf8StrByteCount,
        int32_t * pIdsArr,
        const int MaxIdsArrLength,
        const int UnkId = 0
);
const int TextToIdsWithOffsets_sp(
        void* ModelPtr,
        const char * pInUtf8Str,
        int InUtf8StrByteCount,
        int32_t * pIdsArr,
        int * pStartOffsets, 
        int * pEndOffsets,
        const int MaxIdsArrLength,
        const int UnkId = 0
);
const int TextToIds_sp(
        void* ModelPtr,
        const char * pInUtf8Str,
        int InUtf8StrByteCount,
        int32_t * pIdsArr,
        const int MaxIdsArrLength,
        const int UnkId = 0
);
const int TextToIdsWithOffsets(
        void* ModelPtr,
        const char * pInUtf8Str,
        int InUtf8StrByteCount,
        int32_t * pIdsArr,
        int * pStartOffsets, 
        int * pEndOffsets,
        const int MaxIdsArrLength,
        const int UnkId = 0
);
const int TextToIds(
        void* ModelPtr,
        const char * pInUtf8Str,
        int InUtf8StrByteCount,
        int32_t * pIdsArr,
        const int MaxIdsArrLength,
        const int UnkId = 0
);
int FreeModel(void* ModelPtr);
int SetNoDummyPrefix(void* ModelPtr, bool fNoDummyPrefix);
int IdsToText (void* ModelPtr, const int32_t * pIdsArr, const int IdsCount, char * pOutUtf8Str, const int MaxOutUtf8StrByteCount, bool SkipSpecialTokens);
}
}