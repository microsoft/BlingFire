ruby export_vocab.rb

# produce pos.dict.utf8 file and tagset.txt:
cat spiece.model.exportvocab.txt | awk 'BEGIN {FS="\t"} NF == 2 { if (NR > 1) { print $1 "\tWORD_ID_" NR-1 "\t" ($2 == 0 ? "-0.00001" : $2); }  print "WORD_ID_" NR " " NR > "tagset.txt"; }' > pos.dict.utf8

# zip it:
zip pos.dict.utf8.zip pos.dict.utf8

# build as usual
make -f Makefile.gnu lang=gpt2 all
