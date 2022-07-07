// import emscripten genenrated module
import { Module } from './blingfire.js';

// returns version
export function GetVersion() {
  return Module["_GetBlingFireTokVersion"]();
}


// breaks to words, takes a JS string and returns a JS string
export function TextToWords(s) {

  var len = Module["lengthBytesUTF8"](s);

  var inUtf8 = Module["_malloc"](len + 1); // if we don't do +1 this library won't copy the last character
  Module["stringToUTF8"](s, inUtf8, len + 1); //  since it always also needs a space for a 0-char

  var MaxOutLength = (len << 1) + 1; // worst case every character is a token
  var outUtf8 = Module["_malloc"](MaxOutLength);

  try
  {
    var actualLen = Module["_TextToWords"](inUtf8, len, outUtf8, MaxOutLength);
    if(0 > actualLen || actualLen > MaxOutLength) {
      return null;
    }
  }
  finally
  {
    if (inUtf8 != 0)
    {
      Module["_free"](inUtf8);
    }

    if (outUtf8 != 0)
    {
      Module["_free"](outUtf8);
    }
  }

  return Module["UTF8ToString"](outUtf8);
}

// breaks to sentences, takes a JS string and returns a JS string
export function TextToSentences(s) {

  var len = Module["lengthBytesUTF8"](s);

  var inUtf8 = Module["_malloc"](len + 1); // if we don't do +1 this library won't copy the last character
  Module["stringToUTF8"](s, inUtf8, len + 1); //  since it always also needs a space for a 0-char

  var MaxOutLength = (len << 1) + 1; // worst case every character is a token
  var outUtf8 = Module["_malloc"](MaxOutLength);

  try
  {
    var actualLen = Module["_TextToSentences"](inUtf8, len, outUtf8, MaxOutLength);
    if(0 > actualLen || actualLen > MaxOutLength) {
      return null;
    }
  }
  finally
  {
    if (inUtf8 != 0)
    {
      Module["_free"](inUtf8);
    }

    if (outUtf8 != 0)
    {
      Module["_free"](outUtf8);
    }
  }

  return Module["UTF8ToString"](outUtf8);
}


// loads model by URL
export async function LoadModel(url) {

    var wasmMem = 0;
    var h = 0;

    try {
      const response = await fetch(url);
      var bytes = await response.arrayBuffer();

      const byteArray = new Uint8Array(bytes);
      wasmMem = Module["_malloc"](bytes.byteLength);
      Module["HEAPU8"].set(byteArray, wasmMem);

      h = Module["_SetModel"](wasmMem, bytes.byteLength);

      return { h, wasmMem };

    } catch(e) {

      console.error('Error loading model: ' + url);

      if(wasmMem != 0) {
        Module["_free"](wasmMem);
      }

      return null;
    }
}


// free model
export function FreeModel(handle) {

  if(handle == null) {
    return;
  }

  var { h, wasmMem } = handle;

  if(h != 0) {
    Module["_FreeModel"](h);
  }

  if(wasmMem != 0) {
    Module["_free"](wasmMem);
  }
}


// for the loaded model and input text returns an integer array with IDs
export function TextToIds(handle, s, max_len, unk = 0) {

  if(handle == null) {
    return;
  }

  // get the handle parsed
  var { h, wasmMem } = handle;

  // convert input JS string to UTF-8
  var len = Module["lengthBytesUTF8"](s);
  var inUtf8 = Module["_malloc"](len + 1); // if we don't do +1 this library won't copy the last character
  Module["stringToUTF8"](s, inUtf8, len + 1); //  since it always also needs a space for a 0-char

  var MaxOutLength = max_len;
  var IdsOut = Module["_malloc"](MaxOutLength * 4); // sizeof(int)

  try
  {
    // get the IDS from BlingFire
    var actualLen = Module["_TextToIds"](h, inUtf8, len, IdsOut, MaxOutLength, unk);
    if(0 >= actualLen) {
      return null;
    }

    // get the smallest between actualLen and MaxOutLength
    var actualLenOrMax = actualLen < MaxOutLength ? actualLen : MaxOutLength;

    // read bytes 
    var tmp = Module["HEAPU8"].subarray(IdsOut, IdsOut + (actualLenOrMax * 4));

    // allocate JS array
    var ids = new Int32Array(actualLenOrMax);

    // decode bytes into int array (I could not find how to make a cast here)
    var i = 0;
    var j = 0;
    for(; i < actualLenOrMax; i++, j+=4) {
      ids[i] = tmp[j] + (tmp[j + 1] * 256) + (tmp[j + 2] * 65536) + (tmp[j + 3] * 16777216);
    }
  }
  finally
  {
    if (inUtf8 != 0)
    {
      Module["_free"](inUtf8);
    }

    if (IdsOut != 0)
    {
      Module["_free"](IdsOut);
    }
  }

  return ids;
}


// breaks to words, takes a JS string and returns a JS string
export function WordHyphenation(handle, s, hyp = 0x2D) {

  if(handle == null) {
    return;
  }

  // get the handle parsed
  var { h, wasmMem } = handle;

  var len = Module["lengthBytesUTF8"](s);

  var inUtf8 = Module["_malloc"](len + 1); // if we don't do +1 this library won't copy the last character
  Module["stringToUTF8"](s, inUtf8, len + 1); //  since it always also needs a space for a 0-char

  var MaxOutLength = (len << 1) + 1; // worst case hyphen after every character
  var outUtf8 = Module["_malloc"](MaxOutLength);

  try
  {
    var actualLen = Module["_WordHyphenationWithModel"](inUtf8, len, outUtf8, MaxOutLength, h, hyp);
    if(0 > actualLen || actualLen > MaxOutLength) {
      return null;
    }
  }
  finally
  {
    if (inUtf8 != 0)
      {
        Module["_free"](inUtf8);
      }

      if (outUtf8 != 0)
      {
        Module["_free"](outUtf8);
      }
  }

  return Module["UTF8ToString"](outUtf8);
}
