# build everything and call set_env from the BlingFire directory to set the PATH (see wiki)

# change directory to the directory with source files
cd <BlingFire>/ldbsrc/xlnet

# export the model:
spm_export_vocab --model spiece.model --output spiece.model.exportvocab.txt --output_format txt 

# produce pos.dict.utf8 file and tagset.txt:
cat spiece.model.exportvocab.txt | awk 'BEGIN {FS="\t"} NF == 2 { if (NR > 1) { print $1 "\tWORD_ID_" NR-1 "\t" ($2 == 0 ? "-0.00001" : $2); }  print "WORD_ID_" NR " " NR > "tagset.txt"; }' > pos.dict.utf8

# zip it:
zip pos.dict.utf8.zip pos.dict.utf8

# optional step: create a charmap for NFC --> NFKC normalization or anything else
python ./generate_charmap.py > charmap.utf8

# create options.small and ldb.conf.small by example

# build all as usual
cd <BlingFire>/ldbsrc
make -f Makefile.gnu lang=xlnet all

# after the succuessful compilation there should be a new file xlnet.bin inside the ldb directory
ls -l ldb/xlnet.bin

# let's run the parity verification
cat input.utf8 | python ../scripts/test_bling_with_offsets.py -m ldb/xlnet.bin -p xlnet/spiece.model > output.utf8

# count number of mismatches
> cat output.utf8 | awk '/ERROR:/' |  wc -l
2133

# count total number of input texts
> wc -l input.utf8
2052515 input.utf8

Input level parity is close to 99.9%, token level parity will be much higher than this. Upon inspection, 
most of the disparities are due to sentence piece treating some Unicode control characters as a letter and some as a space,
Bling Fire treats them all as a space.



Measure perf:

> time -p cat input.utf8 | python ../scripts/test_sp.py -m spiece.model -s 1 
real 218.04
user 217.69
sys 1.19
> time -p cat input.utf8 | python ../scripts/test_bling.py -m xlnet.bin  -s 1 
real 101.53
user 101.33
sys 1.42

