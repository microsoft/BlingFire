# set environment variable path
export PATH=$pwd../../build:$pwd../../scripts:$PATH
echo $PATH
# build ldb. Make sure you have BlingFireTools built
if [ -d "tmp" ]; then 
  rm -rf "tmp" 
fi
mkdir tmp

if [ -d "out" ]; then 
  rm -rf "out" 
fi
mkdir out

fa_build_conf  --in=ldb.conf.small  --out=tmp/ldb.mmap.small.txt
fa_fsm2fsm_pack --type=mmap  --in=tmp/ldb.mmap.small.txt  --out=tmp/ldb.conf.small.dump  --auto-test
fa_build_lex --dict-root=. --in=wbd.lex.utf8  --tagset=wbd.tagset.txt --out-fsa=tmp/wbd.rules.fsa.txt  --out-fsa-iwmap=tmp/wbd.rules.fsa.iwmap.txt  --out-map=tmp/wbd.rules.map.txt &&
fa_fsm2fsm_pack --alg=triv --type=moore-dfa --remap-iws --use-iwia  --in=tmp/wbd.rules.fsa.txt  --iw-map=tmp/wbd.rules.fsa.iwmap.txt  --out=tmp/wbd.fsa.small.dump &&
fa_fsm2fsm_pack --alg=triv --type=mmap  --in=tmp/wbd.rules.map.txt  --out=tmp/wbd.mmap.small.dump &&
fa_merge_dumps --out=out/tp3.sbd.bin  tmp/ldb.conf.small.dump tmp/wbd.fsa.small.dump  tmp/wbd.mmap.small.dump &&
fa_dump2cpp --in=out/tp3.sbd.bin --out=BlingFireTokLibSbdData.cxx --name=g_dumpBlingFireTokLibSbdData