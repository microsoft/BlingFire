# Bling Fire WebAssembly

This folder contains webassembly make file for Bling Fire. This is work in progress and we likely to change the files. Help needed!

The web assembly and JS code implement the following functions:

  1. TextToSentences -- breaks JS string into sentences using default model
  2. TextToWords -- breaks JS string into NLTK style tokens using default model 
  3. async LoadModel -- given a URL fetches it and loads a model, returns a handle to the loaded model
  4. FreeModel -- destroys model object given a model handle
  5. TextToIds -- given a model handle and a JS string returns an array of ids of int type
  6. WordHyphenation -- given a model handle and a word as a JS string and an optional hyphenation unicode code returns a hyphenated word

## To compile

Note: we have tesed this on Linux only, however after compiled it suppose to work on any platform.

### One time install emsdk:

1. git clone https://github.com/emscripten-core/emsdk.git
2. cd emsdk
3. git pull
4. ./emsdk install latest
5. ./emsdk install latest
6. ./emsdk activate latest

### Using Makefile

#### Setup Emscripten environment before using the Makefile

1. source "/home/sergei/tmp/emsdk/emsdk_env.sh"

#### Recompile

1. make all
2. add word "export" without qoutes to generated blingfire.js

### Using CMake

If you want to integrate BlingFire into your library, using cmake to build bingfire is a better choice.

The following code is just to demonstrate how build WebAssembly version of blingfiretokdll with cmake.

```sh
cmake -DCMAKE_TOOLCHAIN_FILE=/home/sergei/tmp/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake .. -B build

cmake --build build --target blingfiretokdll 
```

## To Test

### Start a simple Python based web server:

1. make http.server
2. open in a browser http://0.0.0.0:8000/
3. select example.html or blingfire.html

As you click buttons in example.html a corresponding APIs are called and results are printed to console.log. You will see something like this:

#### Press GetBlingFireVersion:

Version: 6001

#### Press TextToWords:

I saw a girl with a telescope . Я видел девушку с телескопом .

#### Press TextToSentences

I saw a girl with a telescope.
Я видел девушку с телескопом.

#### Press LoadModel

Model handle: [object Object]

#### Press TextToIds

Int32Array(21) [ 109, 29, 425, 13, 8710, 66, 1087, 13, 83595, 9, … ]

#### Press TextToIds again

Int32Array(21) [ 109, 29, 425, 13, 8710, 66, 1087, 13, 83595, 9, … ]

#### Press FreeModel

Model Freed!

#### Press TextToIds after the model is freed

Load the model first!

## To integrate

To integrate into a client web page and use in a browser you need three files:

1. blingfire.js            -- generated JS from Emscripten, contains some useful and not useful helper function and glue code
2. blingfire.wasm          -- compiled WASM of Blig Fire
3. blingfire_wrapper.js    -- manually written wrapper JS to implement APIs above