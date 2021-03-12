#
# This makefile compiles BlingFire code into WASM
#
# 1. Install emsdk:
# git clone https://github.com/emscripten-core/emsdk.git
# cd emsdk
# git pull
# ./emsdk install latest
# ./emsdk install latest
# ./emsdk activate latest
#
# 2. Setup PATH for emsdk:
# source "/home/sergei/tmp/emsdk/emsdk_env.sh"
#
# 3. make all
#

all: blingfire.js

blingfire.js:
	em++ ../blingfiretools/blingfiretokdll/blingfiretokdll.cpp ../blingfiretools/blingfiretokdll/*.cxx  ../blingfireclient.library/src/*.cpp  -s WASM=1 -s EXPORTED_FUNCTIONS="[_GetBlingFireTokVersion, _TextToSentences, _TextToWords, _TextToIds, _SetModel, _FreeModel, _WordHyphenationWithModel]" -s "EXTRA_EXPORTED_RUNTIME_METHODS=['lengthBytesUTF8', 'stackAlloc', 'stringToUTF8', 'UTF8ToString']" -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 -I ../blingfireclient.library/inc/ -I ../blingfirecompile.library/inc/ -DHAVE_ICONV_LIB -DHAVE_NO_SPECSTRINGS -D_VERBOSE -DBLING_FIRE_NOAP -DBLING_FIRE_NOWINDOWS -DNDEBUG -O3 --std=c++11 -o blingfire.js

clean:
	rm blingfire.js blingfire.wasm *.bin


# go to http://localhost:8000/
# open blingfire.html
http.server: asset_files
	python -m http.server


asset_files:
	cp ../dist-pypi/blingfire/bert_base_cased_tok.bin bert_base_cased_tok.bin
	cp ../dist-pypi/blingfire/bert_chinese.bin bert_chinese.bin
	cp ../dist-pypi/blingfire/laser100k.bin laser100k.bin
	cp ../dist-pypi/blingfire/laser500k.bin laser500k.bin
	cp ../dist-pypi/blingfire/xlm_roberta_base.bin xlm_roberta_base.bin
	cp ../dist-pypi/blingfire/xlnet_nonorm.bin xlnet_nonorm.bin
	cp ../dist-pypi/blingfire/bert_base_tok.bin bert_base_tok.bin
	cp ../dist-pypi/blingfire/bert_multi_cased.bin bert_multi_cased.bin
	cp ../dist-pypi/blingfire/laser100k.bin laser100k.bin
	cp ../dist-pypi/blingfire/laser250k.bin laser250k.bin
	cp ../dist-pypi/blingfire/laser500k.bin laser500k.bin
	cp ../dist-pypi/blingfire/uri100k.bin uri100k.bin
	cp ../dist-pypi/blingfire/uri250k.bin uri250k.bin
	cp ../dist-pypi/blingfire/uri500k.bin uri500k.bin
	cp ../dist-pypi/blingfire/wbd_chuni.bin wbd_chuni.bin
	cp ../dist-pypi/blingfire/xlnet.bin xlnet.bin
	cp ../dist-pypi/blingfire/gpt2.bin gpt2.bin
	cp ../dist-pypi/blingfire/roberta.bin roberta.bin
	cp ../dist-pypi/blingfire/syllab.bin syllab.bin
