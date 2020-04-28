# m.model is a BPE (not unigram lm!) model from the sentence piece library 
# Note: for unigram lm model see <BlingFire>/ldbsrc/xlnet and <BlingFire>/ldbsrc/xlnet _nonorm examples

# export the model:
spm_export_vocab --model m.model --output spiece.model.exportvocab.txt --output_format txt 

# produce pos.dict.utf8 file and tagset.txt:
cat spiece.model.exportvocab.txt | awk 'BEGIN {FS="\t"} NF == 2 { if (NR > 1) { print $1 "\tWORD_ID_" NR-1 "\t" $2; } print "WORD_ID_" NR " " NR > "tagset.txt"; }' > pos.dict.utf8

# zip it:
zip pos.dict.utf8.zip pos.dict.utf8

# build as usual
make -f Makefile.gnu lang=bpe_example all

# test parity between m.model and bpe_example.bin :
> cat input.utf8 | python ../scripts/test_bling_with_offsets.py -m bpe_example.bin -p m.model -u 3 -k 1 | awk '/ERROR/' | wc -l
3881

> wc -l input.utf8
2052515 input.utf8

99.8% parity
