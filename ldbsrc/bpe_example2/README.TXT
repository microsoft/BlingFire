# m.model is a BPE (not unigram lm!) model from the sentence piece library 
# Note: for unigram lm model see <BlingFire>/ldbsrc/xlnet and <BlingFire>/ldbsrc/xlnet _nonorm examples

# export the model:
spm_export_vocab --model m.model --output spiece.model.exportvocab.txt --output_format txt 

# produce pos.dict.utf8 file and tagset.txt:
cat spiece.model.exportvocab.txt | awk 'BEGIN {FS="\t"} NF == 2 { if (NR > 1) { print $1 "\tWORD_ID_" NR-1 "\t" ($2 == 0 ? "-0.00001" : $2); }  print "WORD_ID_" NR " " NR > "tagset.txt"; }' > pos.dict.utf8

# zip it:
zip pos.dict.utf8.zip pos.dict.utf8

# optional step: create a charmap for NFC --> NFKC normalization or anything else
python ./generate_charmap.py > charmap.utf8

# cd ldbsrc and build as usual, see wiki
make -f Makefile.gnu lang=bpe_example all

# test parity between m.model and bpe_example.bin :

> ~/BlingFire/ldbsrc$ cat bpe_example2/input.utf8 | python ../scripts/test_bling_with_offsets.py -m ldb/bpe_example2.bin -p bpe_example2/m.model -u 1 | awk '/ERROR:/' | wc -l 
105

> ~/BlingFire2/BlingFire/ldbsrc$ wc -l bpe_example2/input.utf8 
28703 bpe_example2/input.utf8


> ~/BlingFire/ldbsrc$ cat test.txt | python ../scripts/test_bling_with_offsets.py -m ldb/bpe_example2.bin -p bpe_example2/m.model -u 1 | awk '/ERROR:/' | wc -l 
206
> ~/BlingFire/ldbsrc$ wc -l test.txt 
100000 test.txt


> ~/BlingFire/ldbsrc$ cat test.multi.txt | python ../scripts/test_bling_with_offsets.py -m ldb/bpe_example2.bin -p bpe_example2/m.model -u 1 | awk '/ERROR:/' | wc -l 
54
> ~/BlingFire/ldbsrc$ wc -l test.multi.txt
100000 test.multi.txt


Currently parity is between 99.6% (first corpus) and 99.9% (last corpus, checked in under ldbsrc/bert_multi_cased directory).


