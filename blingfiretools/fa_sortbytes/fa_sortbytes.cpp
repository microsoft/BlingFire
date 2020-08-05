/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */


#include "FAConfig.h"
#include "FAUtils.h"
#include "FASecurity.h"


using namespace BlingFire;

class SortBytes {

protected:
    static int fReverse;
    static int fNumeric;
    static int fUnique;

private:
    typedef struct MERGEDATA
    {
        int iFile;              // number of temp file this string came from
        unsigned char *pwsz;          // pointer to the string
        FILE *pFile;            // handle to the open file
    } MERGEDATA;

    typedef struct SORTDATA
    {
        char pszTempDir[PATH_MAX];      // temporary directory for intermediate files
        char * pszTempFileName; // Space to hold a temporary file name
        unsigned char *pwszCoreLoad;  // buffer to hold the data to be sorted
        int cwchCoreLoadMax;    // absolute size of coreload buffer
        int cwchCoreLoadMac;    // currently used amount of coreload buffer
        int cwchCoreLoadLim;    // number of bytes of whole strings in coreload
        unsigned char **ppwszCorePointers; // buffer of pointers to beginning of strings
        int cCorePointersMax;   // size of pointers array
        int cCorePointersMac;   // number of pointers currently in use

        int cwchStringMac;      // length of longest string

        MERGEDATA rgMergeData[FOPEN_MAX];   // array of strings being merged.
        int cOpenFilesMax;      // Max number of input files we can have open at one time
        int cFilesMac;          // Total number of output files so far
        int cFilesMic;          // Lowest valid temporary file (we delete old ones)
    } SORTDATA;


public:
    static void Sort(FILE* pInput, int cMegsCoreLoad, const char* pszOutputFile);
    static void SetFReverse(int fR);
    static int GetFReverse();
    static void SetFNumeric(int fN);
    static int GetFNumeric();
    static void SetFUnique(int fU);
    static int GetFUnique();

protected:
    static char* MakeSortName(int iFile, SORTDATA *pSortData);

    static int KeyCmp(const unsigned char *pwsz1, const unsigned char *pwsz2) {
        int cmpValue;
        int strlen1;
        int strlen2;
        int minlen;

        if (fNumeric) {
            // note that if fNumeric, results are undefined unless all characters are the same signed or not
            cmpValue = strtol((const char*)pwsz1,NULL,10) - strtol((const char*)pwsz2,NULL,10);
        } else {
            // at this point the strings should be guaranteed null-terminated - compare their beginnings
            strlen1 = (int) strlen((const char*)pwsz1);
            strlen2 = (int) strlen((const char*)pwsz2);
            if (strlen2 < strlen1)
            {
                minlen = strlen2;
            }
            else {
                minlen = strlen1;
            }
            cmpValue = memcmp(pwsz1,pwsz2,minlen);

            if (0 == cmpValue) {
                if (strlen2 < strlen1)
                {
                    // strlen2 is a prefix of strlen1
                    cmpValue = 1;
                }
                else if (strlen1 < strlen2)
                {
                    // strlen1 is a prefix of strlen2
                    cmpValue = -1;
                }
                else
                {
                    // do nothing, these strings really are the same
                }
            }
        }

        if (fReverse) {
            cmpValue = -cmpValue;
        }

        return cmpValue;
    }

    static int __cdecl MergeCmp(const MERGEDATA *pMergeData1, const MERGEDATA *pMergeData2)
    {
        return KeyCmp(pMergeData1->pwsz, pMergeData2->pwsz);
    }


    static int __cdecl SortCmp(const unsigned char **ppwsz1, const unsigned char **ppwsz2)
    {
        return KeyCmp(*ppwsz1, *ppwsz2);
    }

    static void SortDump(const char *pszError, SORTDATA *pSortData);
    static int SortCoreLoad(FILE *pInput, SORTDATA *pSortData);
    static int SortPhase(FILE *pInput, SORTDATA *pSortData);
    static int MergeNextSet(const char *pszOutputFileName, SORTDATA *pSortData);
    static int MergePhase(const char *pOutputFileName, SORTDATA *pSortData);
    static void CountMaxOpenFiles(SORTDATA *pSortData);
    static unsigned char* GetLineInUChar(
        __out_ecount(cchMaxLen) unsigned char* pStr0,
        int cchMaxLen,
        FILE *pFile);
    static void PutLineInUChar(const unsigned char* pString, FILE *pFile);

};

int SortBytes::fReverse = 0;
int SortBytes::fNumeric = 0;
int SortBytes::fUnique = 0;

void SortBytes::SetFReverse(int fR)
{
    fReverse = fR;
}
int SortBytes::GetFReverse()
{
    return fReverse;
}
void SortBytes::SetFNumeric(int fN)
{
    fNumeric = fN;
}
int SortBytes::GetFNumeric()
{
    return fNumeric;
}
void SortBytes::SetFUnique(int fU)
{
    fUnique = fU;
}
int SortBytes::GetFUnique()
{
    return fUnique;
}

/* Convert an integer (a file number) into a temp file name, using the sort directory and the process
id to make a unique name.  \directory\sortPID.INDEX The space is allocated in the sort data structure
already, and this routine simply puts the name there.  It also returns a pointer to it, as a convenience.
This means you'd better not call this again until you're through with the last name! */

char* SortBytes::MakeSortName(int iFile, SORTDATA *pSortData)
{
#if _MSC_VER < 1400
    sprintf (pSortData->pszTempFileName, "%s\\sort%04X.%03X", pSortData->pszTempDir, _getpid(), iFile);
#else
    const int TmpBuffSize = (int) strlen (pSortData->pszTempDir) + 14; // includes space for the terminating zero
    sprintf_s (pSortData->pszTempFileName, TmpBuffSize, "%s\\sort%04X.%03X", pSortData->pszTempDir, _getpid(), iFile);
#endif
    return pSortData->pszTempFileName;
}

void SortBytes::SortDump(const char *pszError, SORTDATA *pSortData)
{
    int i;
    fprintf(stderr,"%s: cCorePointersMac/Max = %d/%d\n",pszError, pSortData->cCorePointersMac, pSortData->cCorePointersMax);

    for (i = 0; i < pSortData->cCorePointersMac; ++i) {
        fprintf(stderr,"\t%3d: '%s'\n",i,pSortData->ppwszCorePointers[i]);
    }
}


// Read a coreload, sort it, and write it to the output

int SortBytes::SortCoreLoad(FILE *pInput, SORTDATA *pSortData)
{
    FILE *pOutputFile;
    int i, len;

    // if there's data left over at end of buffer, copy it down

    if (pSortData->cwchCoreLoadMac > pSortData->cwchCoreLoadLim) {
        pSortData->cwchCoreLoadMac -= pSortData->cwchCoreLoadLim;
        memmove(pSortData->pwszCoreLoad,
            pSortData->pwszCoreLoad+pSortData->cwchCoreLoadLim,
            pSortData->cwchCoreLoadMac*sizeof(unsigned char));
        pSortData->cwchCoreLoadLim = 0;
    }
    pSortData->cwchCoreLoadMac -= pSortData->cwchCoreLoadLim;
    pSortData->cwchCoreLoadLim = 0;
    pSortData->ppwszCorePointers[0] = pSortData->pwszCoreLoad;
    pSortData->cCorePointersMac = 0;

    // now read strings until buffer full or EOF
    // actually, buffer needs 3 empty slots: char, nl, and null

    while (pSortData->cwchCoreLoadMax - pSortData->cwchCoreLoadMac > 2 &&
        GetLineInUChar(pSortData->pwszCoreLoad + pSortData->cwchCoreLoadMac,
            pSortData->cwchCoreLoadMax - pSortData->cwchCoreLoadMac,
            pInput)) {

        /* If there are no available pointer slots, double the allocation */

        if (pSortData->cCorePointersMac == pSortData->cCorePointersMax) {

            /* double the allocation */

            pSortData->cCorePointersMax *= 2;

            void * const ptr = realloc (pSortData->ppwszCorePointers, \
                pSortData->cCorePointersMax*sizeof(unsigned char *));

            if (NULL == ptr) {
                fprintf(stderr,"fa_sortbytes: out of memory\n");
                exit(-1);
            }

            pSortData->ppwszCorePointers = (unsigned char**) ptr;
        }

        /* Record the pointer to the beginning of the string we just read, unless
        this is the first string (possibly a continuation from the last coreload) */

        if (pSortData->cCorePointersMac) {
            pSortData->ppwszCorePointers[pSortData->cCorePointersMac] =
                pSortData->pwszCoreLoad + pSortData->cwchCoreLoadMac;
        }
        ++pSortData->cCorePointersMac;

        /* see how many characters we got, including the terminal \n, if any */

        while (pSortData->cwchCoreLoadMax > pSortData->cwchCoreLoadMac &&
            pSortData->pwszCoreLoad[pSortData->cwchCoreLoadMac]) {
            ++pSortData->cwchCoreLoadMac;
        }

        /* If we only got part of a string, bail out now */

        if (pSortData->pwszCoreLoad[pSortData->cwchCoreLoadMac-1] != L'\n') {
            --pSortData->cCorePointersMac;
            break;
        };

        pSortData->pwszCoreLoad[pSortData->cwchCoreLoadMac-1] = L'\x00';

        /* Okay, so we read an entire string. */

        pSortData->cwchCoreLoadLim = pSortData->cwchCoreLoadMac;

    }

    //  SortDump("Outside of the loop",pSortData);

    /* If we hit EOF with no data, return an indication that we are done */

    if (!pSortData->cwchCoreLoadLim) {
        return false;
    }

    /* At this point, cwchCoreLoadMac is the number of bytes read into the buffer, possibly the
    same as cwchCoreLoadMax, and cwchCoreLoadLim is the number of bytes of complete strings
    in the buffer. cCorePointersMac is the number of complete strings. */

    // Sort the pointers to the strings. A more advanced sort might do more here

    qsort(pSortData->ppwszCorePointers, pSortData->cCorePointersMac, sizeof(unsigned char *), reinterpret_cast<int(__cdecl *)(const void *, const void *)>(SortBytes::SortCmp));

    // SortDump("After the sort",pSortData);
    // Open an output file

    int res = fopen_s (&pOutputFile, MakeSortName(pSortData->cFilesMac,pSortData),"wb");
    if (NULL == pOutputFile || 0 != res) {
        fprintf(stderr,"Error opening coreload file %d\n",pSortData->cFilesMac);
        exit(-1);
    }

    pSortData->cFilesMac++;

    // Write the strings in order, tracking the max length

    for (i = 0; i < pSortData->cCorePointersMac; ++i) {
        PutLineInUChar(pSortData->ppwszCorePointers[i],pOutputFile);
        fwrite("\n", 1, sizeof(char), pOutputFile);
        len = (int) strlen((char*)(pSortData->ppwszCorePointers[i]));
        if (len > pSortData->cwchStringMac) {
            pSortData->cwchStringMac = len;
        }
    }

    /* Close our file and clean up */

    fclose(pOutputFile);

    return true;
}

int SortBytes::SortPhase(FILE *pInput, SORTDATA *pSortData)
{

    /* Allocate space for coreLoad */

    pSortData->pwszCoreLoad = (unsigned char*)malloc(pSortData->cwchCoreLoadMax*sizeof(unsigned char));
    pSortData->ppwszCorePointers = (unsigned char**)malloc(sizeof(unsigned char *));
    pSortData->cCorePointersMax = 1;
    if (!pSortData->pwszCoreLoad || !pSortData->ppwszCorePointers) {
        fprintf(stderr,"Out of memory in sort initialization\n");
        exit(-1);
    }

    while (SortCoreLoad(pInput,pSortData))
        ;

    free(pSortData->pwszCoreLoad);
    free(pSortData->ppwszCorePointers);

    return (true);

} // SortPhase

/* merge as many files as possible into a new one.  If ALL files are merged, write to
the output file instead of to a temp. The logic here is that every temporary file is sorted,
so any set of them can be merged into a new temporary file.  Since it doesn't matter much
which order we do them in, we start from the beginning.  */

int SortBytes::MergeNextSet(const char *pszOutputFileName, SORTDATA *pSortData)
{
    int i, cFiles, lastPass, cFiles0, CmpEntries, fSkip;
    MERGEDATA *pMergeData, mergeDataTmp;
    unsigned char *pwszCoreLoad;
    FILE *pOutputFile;

    // Open pSortData->cOpenFilesMax files for reading.  If there aren't that many, note that this is the last pass
    cFiles = cFiles0 = (pSortData->cOpenFilesMax <= pSortData->cFilesMac - pSortData->cFilesMic) ? \
        pSortData->cOpenFilesMax : \
        pSortData->cFilesMac - pSortData->cFilesMic ;

    lastPass = false;
    if (cFiles + pSortData->cFilesMic == pSortData->cFilesMac) {
        lastPass = true;
    }

    /* Now set up the mergeData structure.  Each string points to a space in the coreload area,
    the temporary file number is set, the temp file is opened, and the first string is read.
    Note that there must be at least one string in each file; there are no empty coreloads */

    pMergeData = pSortData->rgMergeData;
    pwszCoreLoad = pSortData->pwszCoreLoad;
    for (i = 0; i < cFiles; ++i) {
        pMergeData->iFile = pSortData->cFilesMic + i;
        pMergeData->pwsz = pwszCoreLoad;
        int res = fopen_s (&(pMergeData->pFile), MakeSortName(pMergeData->iFile,pSortData), "rb");
        if (NULL == pMergeData->pFile || 0 != res) {
            perror("Cannot open a temporary file.");
            exit(-1);
        }
        GetLineInUChar(pMergeData->pwsz,pSortData->cwchStringMac+1,pMergeData->pFile);
        ++pMergeData;
        pwszCoreLoad += pSortData->cwchStringMac+1;
    }

    // Open the output file.  Just another temp file, unless this is the last merge

    int res = 0;

    if (lastPass) {

        if (pszOutputFileName) {
            res = fopen_s (&pOutputFile, pszOutputFileName, "wb");
        } else {
            pOutputFile = stdout;
        }

    } else {
        res = fopen_s (&pOutputFile, MakeSortName(pSortData->cFilesMac,pSortData), "wb");
    }

    if (NULL == pOutputFile || 0 != res) {
        perror("Can't open merge output file");
        exit(-1);
    }

    // Sort the Merge list
    qsort(pSortData->rgMergeData,cFiles,sizeof(MERGEDATA),reinterpret_cast<int(__cdecl *)(const void *, const void *)>(SortBytes::MergeCmp));

    // While there are still input files,
    pMergeData = pSortData->rgMergeData;
    fSkip = false;
    while (cFiles) {

        // Write out the top string.

        if (!fSkip)
        {
            PutLineInUChar(pMergeData->pwsz,pOutputFile);
        }
        fSkip = false;

        // Read a replacement from the file it came from

        if (GetLineInUChar(pMergeData->pwsz,pSortData->cwchStringMac+1,pMergeData->pFile)) {

            // see where the new string fits alphabetically

            for (i = 1; i < cFiles; ++i)
            {
                CmpEntries = MergeCmp(pMergeData,pMergeData+i);
                if (0 > CmpEntries) break;
                else if (0 == CmpEntries)
                {
                    fSkip = fUnique;
                    break;
                }
            }

            // rearrange the array, if need be

            if (i > 1) {
                mergeDataTmp = *pMergeData;
                memcpy(pMergeData,pMergeData+1,(i-1)*sizeof(MERGEDATA));
                pMergeData[i-1] = mergeDataTmp;

            }
        } else {
            // We finished with the file.  Decrement the count, close and delete the file,
            // and shift the array up.

            --cFiles;
            res = fclose(pMergeData->pFile);
            res |= _unlink(MakeSortName(pMergeData->iFile,pSortData));
            DebugLogAssert (0 == res);
            memcpy(pMergeData,pMergeData+1,cFiles*sizeof(MERGEDATA));
        }
    }

    if (pOutputFile != stdout) {
        fclose(pOutputFile);
    }

    if (lastPass) {
        return (false);
    }

    // Adjust all the counts
    pSortData->cFilesMic += cFiles0;
    ++pSortData->cFilesMac;

    return (true);
} // MergeNextSet




int SortBytes::MergePhase(const char *pOutputFileName, SORTDATA *pSortData)
{
    // Set up memory allocations

    pSortData->pwszCoreLoad = (unsigned char*)malloc((pSortData->cwchStringMac+1)*(pSortData->cOpenFilesMax)*sizeof(unsigned char));
    if (!pSortData->pwszCoreLoad) {
        fprintf(stderr,"Out of memory in merge initialization\n");
        exit(-1);
    }

    // Merge until no files remain

    while (MergeNextSet(pOutputFileName,pSortData))
        ;

    free(pSortData->pwszCoreLoad);

    return (true);
} // MergePhase

void SortBytes::CountMaxOpenFiles(SORTDATA *pSortData)
{
    int i;
    FILE *rgFile[FOPEN_MAX];

    for (i = 0; i < FOPEN_MAX; ++i) {
        int res = fopen_s (&(rgFile[i]), MakeSortName(i,pSortData), "w");
        if (NULL == rgFile[i] || 0 != res) {
            break;
        }
    }

    // Now i is the total number of files we can open, but we must allow for an output file

    if (i < 3) {
        fprintf(stderr,"Can't open at least 3 files!\n");
        exit(-1);
    }

    pSortData->cOpenFilesMax = i-1;

    /* Now clean up after ourselves */

    while (i--) {
        int res = fclose(rgFile[i]);
        res |= _unlink(MakeSortName(i,pSortData));
        DebugLogAssert (0 == res);
    }
} // CountMaxOpenFiles

unsigned char* SortBytes::GetLineInUChar(
    __out_ecount(cchMaxLen) unsigned char* pStr0,
    int cchMaxLen,
    FILE *pFile)
{
    char ch;
    char* pStr;

    if (cchMaxLen <= 0)
        return NULL;

    pStr = (char*)pStr0;
    cchMaxLen--; // reserve room for 0
    while (cchMaxLen > 0)
    {
        ch = (char)fgetc(pFile);
        if (feof(pFile))
        {
            if (pStr == (char*)pStr0)
                return NULL;
            break;
        }

        switch (ch)
        {
        case '\r':
            break;

        default:
            *pStr++ = ch;
            cchMaxLen--;
            break;
        }

        if (ch == '\n')
            break;
    }

    *pStr = '\0';
    return(pStr0);
}

void SortBytes::PutLineInUChar (const unsigned char* pString, FILE *pFile)
{
    size_t len = strlen ((char*) pString);

    bool fNeedNewLine = false;

    if (len && pString [len - 1] == '\n') {
        --len;
        fNeedNewLine = true;
    }
    if (len && pString [len - 1] == '\r') {
        --len;
        fNeedNewLine = true;
    }

    fwrite (pString, len, sizeof (char), pFile);
    if (fNeedNewLine) {
        fwrite ("\n", 1, sizeof (char), pFile);
    }
}


int
__cdecl
main(int argc, char** argv)
{
    FILE *pInput;
    int i;
    int fError = 0;
    const char *pszOutputFile;
    int cMegsCoreLoad = 1;   // Multiply by 2 to get byte size of WCHAR working buffer
    int fNumeric = 0;
    int fReverse = 0;
    int fUnique = 0;

    ::FAIOSetup ();

    /* Loop through the argument list looking for flags */

    while (argc > 1) {
        if (*argv[1] != '-' && *argv[1] != '/') {
            break;
        }
        for (i = 1; argv[1][i] != '\0'; ++i) {
            switch (argv[1][i]) {
            case 'n': fNumeric = 1;
                break;
            case 'r': fReverse = 1;
                break;
            case 'u': fUnique = 1;
                break;
            case 'm': cMegsCoreLoad = 250;
                break;
            default:
                fError = 1;
            }
        }
        --argc;
        ++argv;
    }

    /* We allow no more than two non-flag arguments */

    if (argc > 3) {
        fError = 1;
    }

    /* Abend here if we found any errors */

    if (fError) {
        fprintf(stderr,"Usage: fa_sortbytes <-nrm> <input-file <output-file>>\n");
        fprintf(stderr,"       -n sort in numeric order\n");
        fprintf(stderr,"       -r sort in reverse order\n");
        fprintf(stderr,"       -u remove duplicate entries - each resulting entry will be unique\n");
        fprintf(stderr,"          (not implemented yet)\n");
        fprintf(stderr,"       -m use ~512 MB of RAM during sort phase (default ~2)\n");
        exit(-1);
    }

    pInput = stdin;
    int res = 0;

    if (argc > 1) {
        res = fopen_s (&pInput, argv[1], "rb");
    }

    if (NULL == pInput || 0 != res) {
          perror("Cannot open the input file.");
          exit(-1);
    }

    pszOutputFile = argc > 2 ? argv[2] : NULL;

    /* Suppress conversion to text mode */
    ::FAInputIOSetup ();

    SortBytes::SetFReverse(fReverse);
    SortBytes::SetFNumeric(fNumeric);
    SortBytes::SetFNumeric(fUnique);
    SortBytes::Sort(pInput,cMegsCoreLoad,pszOutputFile);

    exit (0);
}

/* pszOutputFile should be either the name of a file or NULL for stdout */

void SortBytes::Sort(FILE* pInput, int cMegsCoreLoad, const char* pszOutputFile)
{
    SORTDATA sortData;
    int cchFileNames;

    /* Algorithm: read file into memory, one coreload at a time.  Sort each coreload with qsort,
    then write each into a separate file, using names like sort0097.000, sort0097.001, etc.  Then merge
    the files.  Create the next sequential name for the output, then read the first element from each input
    file.  qsort the list, and write the best one out to the output.  Replace it from the file it
    came from (unless that file is at EOT).  And resort.  Repeat until all files are at EOT.  Then
    delete them.  Then repeat the merge operation on the next set of files.  When there is only
    one file left, that is the desired output file. */

    memset(&sortData,0,sizeof(sortData));
    sortData.cwchCoreLoadMax = cMegsCoreLoad * 1024*1024;

    // and use the current directory for now
    sortData.pszTempDir [0] = '.';
    sortData.pszTempDir [1] = 0;

    // dir plus backslash plus 8 letter name plus dot plus 3 letter ext plus null
    cchFileNames = (int) strlen(sortData.pszTempDir) + 14;
    sortData.pszTempFileName = (char*)malloc(cchFileNames*sizeof(char));

    CountMaxOpenFiles(&sortData);   // find out how many files can be open at once

    SortPhase(pInput, &sortData);
    if (pInput != stdin) {
        fclose(pInput);
    }
    MergePhase(pszOutputFile,&sortData);
}
