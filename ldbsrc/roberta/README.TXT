# For details of the original implementation see this https://huggingface.co/transformers/_modules/transformers/models/roberta/tokenization_roberta.html#RobertaTokenizer

# Note: 
#  1. We will not use UTF-8 as input encoding for the dictionary, instead each symbol is represented as an integer
#  this is because Roberta uses byte level alphabet. We need to specify --input-enc=DEC when we build the dictionary 
#  (see fa_line2chain_unicode --help for details).
#  2. For whatever reason Roberta's IDs are different from merge.txt ranks and so we need use the rank for merging order
#  but ID when we return the tokens, therefore we did some modifications to export_vocab.py. Also in ldb.conf.small,
#  we should specify a different algorithm: "bpe-opt-with-merges"

# run this python script to generate tagset.txt and pos.dict.utf8 from vocab.json AND merges.txt
python export_vocab.py

# zip it the dictionary file
zip pos.dict.utf8.zip pos.dict.utf8

# make sure the tools are compiled and are in the path, see wiki for details

# build LDB as usual, you should get an output like below and no error messages (do "clean" target if encounter errors)
~/BlingFire/ldbsrc$ make -f Makefile.gnu lang=roberta all 

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


# Now we can check if what we have indended to store into the finite state machine is actualy there
# we can use test_ldb for this, see test_ldb --help for details

~/BlingFire/ldbsrc$ test_ldb --ldb=ldb/roberta.bin --tagset=roberta/tagset.txt 
?
commands: pos-dict
pos-dict pedia
WORD_ID_47746 [ -951895296 ] as floats: [ -49979 ] as hex int32: c7433b00
pos-dict Revolution
WORD_ID_45984 [ c7433d00 ] as floats: [ -49981 ] as hex int32: c7433d00
pos-dict <|endoftext|>
WORD_ID_50261 [ c7445500 ] as floats: [ -50261 ] as hex int32: c7445500

# So word "pedia" has ID 47746 (-1) and floating point score associated to it: -49979.0

