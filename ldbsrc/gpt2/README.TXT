# For details of the original implementation see this https://huggingface.co/transformers/_modules/transformers/models/gpt2/tokenization_gpt2.html#GPT2Tokenizer
# Note: we will not use UTF-8 as input encoding for the dictionary, instead each symbol is represented as an integer
#  this is because GPT2 uses byte level alphabet. We need to specify --input-enc=DEC when we build the dictionary 
#  (see fa_line2chain_unicode --help for details).

# run this python script to generate tagset.txt and pos.dict.utf8 from vocab.json
python export_vocab.py

# zip it the dictionary file
zip pos.dict.utf8.zip pos.dict.utf8

# make sure the tools are compiled and are in the path, see wiki for details

# build LDB as usual, you should get an output like below and no error messages (do "clean" target if encounter errors)
~/BlingFire/ldbsrc$ make -f Makefile.gnu lang=gpt2 all 

fa_build_conf \
  --in=gpt2/ldb.conf.small \
  --out=gpt2/tmp/ldb.mmap.small.txt
fa_fsm2fsm_pack --type=mmap \
  --in=gpt2/tmp/ldb.mmap.small.txt \
  --out=gpt2/tmp/ldb.conf.small.dump \
  --auto-test
unzip -p gpt2/pos.dict.utf8.zip | \
fa_build_dict  --input-enc=DEC --type=mph --raw --tagset=gpt2/tagset.txt --float-nums \
  --out-fsm=gpt2/tmp/pos.dict.fsm.txt \
  --out-k2i=gpt2/tmp/pos.dict.k2i.txt \
  --out-i2info=gpt2/tmp/pos.dict.i2t.txt
fa_fsm2fsm_pack --alg=triv --type=mealy-dfa  --in=gpt2/tmp/pos.dict.fsm.txt --out=gpt2/tmp/pos.dict.fsm.small.dump --auto-test
fa_fsm2fsm_pack --alg=triv --type=arr --force-flat  --in=gpt2/tmp/pos.dict.k2i.txt --out=gpt2/tmp/pos.dict.k2i.small.dump --auto-test
fa_fsm2fsm_pack --alg=fixed --type=mmap  --in=gpt2/tmp/pos.dict.i2t.txt --out=gpt2/tmp/pos.dict.i2t.small.dump --auto-test
fa_merge_dumps --out=ldb/gpt2.bin gpt2/tmp/ldb.conf.small.dump gpt2/tmp/pos.dict.fsm.small.dump gpt2/tmp/pos.dict.k2i.small.dump gpt2/tmp/pos.dict.i2t.small.dump 

# you can also test if what we have indended to store into the finite state machine is actualy there
# use test_ldb for this, see test_ldb --help for details

~/BlingFire2/BlingFire/ldbsrc$ test_ldb --ldb=ldb/gpt2.bin
?
commands: pos-dict
pos-dict pedia
???50236 [ -951829504 ] as floats: [ -50236 ] as hex int32: c7443c00
pos-dict Revolution
???c43e [ c7443e00 ] as floats: [ -50238 ] as hex int32: c7443e00
pos-dict <|endoftext|>
???c451 [ c7445100 ] as floats: [ -50257 ] as hex int32: c7445100

# word "pedia" has ID 50236 (-1) and floating point score associated to it: -50236.0

