# build everything and call set_env from the BlingFire directory to set the PATH (see wiki)

# change directory to the directory with source files
cd <BlingFire>/ldbsrc/uri250k

# produce pos.dict.utf8 file and tagset.txt:
cat uri250k.vocab | awk 'BEGIN {FS="\t"} NF == 2 { if (NR > 1) { print $1 "\tWORD_ID_" NR-1 "\t" ($2 == 0 ? "-0.00001" : $2); }  print "WORD_ID_" NR " " NR > "tagset.txt"; }' > pos.dict.utf8

# zip it:
zip pos.dict.utf8.zip pos.dict.utf8

# optional step: create a charmap which contains case folding for characters that are used
python ./generate_charmap.py > charmap.utf8

# create options.small and ldb.conf.small by example

# build all as usual
cd <BlingFire>/ldbsrc
make -f Makefile.gnu lang=uri250k all

# after that you should get a new uri250k.bin under the ldb directory

# let's use it to change URLs into a sequence of IDs
cat seedtest.txt | python ../scripts/test_bling.py -m ldb/uri250k.bin --no-padding true --max-length 128 --single-space true  > qq
paste seedtest.txt qq

https://github.com/microsoft    7 6 8 1434 3 9 4 1736
https://github.com/facebookresearch     7 6 8 1434 3 9 4 63 1547
https://github.com/OpenBuilds   7 6 8 1434 3 9 4 757 2235 6
https://www.cnn.com/    7 6 8 10 3 81 42 3 9 4
https://www.cnn.com/interactive/2021/01/politics/us-capitol-siege/      7 6 8 10 3 81 42 3 9 4 7636 170277 152 4 3922 4 99 5 16012 5 23548 4
https://www.cnn.com/2021/01/07/tech/facebook-trump-restrictions/index.html      7 6 8 10 3 81 42 3 9 170277 152 4 133 4 561 4 63 5 6069 5 114302 4 79 3 19
https://coupons.cnn.com/        7 6 8 6957 3 81 42 3 9 4
https://coupons.cnn.com/categories/bikes        7 6 8 6957 3 81 42 3 9 4 642 4 5007
https://coupons.cnn.com/categories/electronics  7 6 8 6957 3 81 42 3 9 4 642 4 1429
https://coupons.cnn.com/categories/diapers      7 6 8 6957 3 81 42 3 9 4 642 4 94281

# the URL tokenization was found helpful in many URL classification tasks
