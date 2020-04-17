
# export the model:
spm_export_vocab --model spiece.model --output spiece.model.exportvocab.txt --output_format txt 

# produce pos.dict.utf8 file and tagset.txt:
cat spiece.model.exportvocab.txt | awk 'BEGIN {FS="\t"} NF == 2 { if (NR > 1) { print $1 "\tWORD_ID_" NR-1 "\t" $2; } print "WORD_ID_" NR " " NR > "tagset.txt"; }' > pos.dict.utf8

# zip it:
zip pos.dict.utf8.zip pos.dict.utf8

# build as usual
make -f Makefile.gnu lang=xlnet all
