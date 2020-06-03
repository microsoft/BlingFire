1. Download WikiMatrix [1] corpus, extract only unique sentences, sample 30M-50M of them.


2. Normalize the spaces in sentences, since we will use "identity" as normalization

  cat all_plain_text.30m.txt | python ../../scripts/normalize_corpus.py > normalized_corpus.txt


3. Learn the unigram lm model from the corpus:

  spm_train --input=normalized_corpus.txt --model_prefix=laser_100k --vocab_size=100000 --character_coverage=0.9999 --model_type=unigram --normalization_rule_name=identity --num_threads=10 --input_sentence_size=50000000 --shuffle_input_sentence=true


4.  Create pos.dict.utf8.zip :

  cat laser_100k.vocab | awk 'BEGIN {FS="\t"} NF == 2 { if (NR > 1) { print $1 "\tWORD_ID_" NR-1 "\t" ($2 == 0 ? "-0.00001" : $2); } print "WORD_ID_" NR " " NR > "tagset.txt"; }' > pos.dict.utf8
  zip pos.dict.utf8.zip pos.dict.utf8

5. Add options.small and ldb.conf.small by example (use xlnet_nonorm since we don't use normalization).

6. Build all as usual (one time compile release version of the tools, run set_env to add the tools to the PATH first):

    cd <BlingFire>/ldbsrc
    make -f Makefile.gnu lang=laser100k all



7. Run tokenization with parity verification:

  cat laser100k/normalized_corpus.txt | python ../scripts/test_bling_with_offsets.py -m ldb/laser100k.bin -p laser100k/laser_100k.model > output.txt

  cat output.txt | awk '/ERROR:/' | wc -l 
  8

  cat output.txt | awk '/INPUT:/' | wc -l 
  720826

  This indicates 99.999% parity with sentence piece model.


8. Measure the perf:

  BlingFire/ldbsrc$ time -p cat test.norm1m.txt | python ../scripts/test_sp.py -m laser100k/laser_100k.model -s 1
  real 63.70
  user 63.45
  sys 0.46

  BlingFire/ldbsrc$ time -p cat test.norm1m.txt | python ../scripts/test_bling.py -m ldb/laser100k.bin -s 1
  real 34.98
  user 34.64
  sys 0.42


9. Play with it from command line:

BlingFire/ldbsrc$ echo "I saw a girl with a telescope. Я видел девушку с телескопом." | python ../scripts/test_bling_with_offsets.py -m ldb/laser100k.bin  
INPUT:
I saw a girl with a telescope. Я видел девушку с телескопом.
IDS:
[  109    29   425    13  8710    66  1087    13 83595     9     3
  4784  1667  5847 42701 17019   887   178 87510   299     3]
TOKENS FROM OFFSETS:
[I] [ sa] [w] [ a] [ gir] [l] [ with] [ a] [ telescop] [e] [.] [ Я] [ ви] [дел] [ дев] [уш] [ку] [ с] [ телескоп] [ом] [.] 

BlingFire/ldbsrc$ echo "I saw a girl with a telescope. Я видел девушку с телескопом." | python ../scripts/test_bling_with_offsets.py -m ldb/laser250k.bin  
INPUT:
I saw a girl with a telescope. Я видел девушку с телескопом.
IDS:
[    93  50326     12  53290    803     12 140741     10      3   3624
 120881 147968     83    175  90465    310      3]
TOKENS FROM OFFSETS:
[I] [ saw] [ a] [ girl] [ with] [ a] [ telescop] [e] [.] [ Я] [ видел] [ девушк] [у] [ с] [ телескоп] [ом] [.] 



Links:

[1] Holger Schwenk, Vishrav Chaudhary, Shuo Sun, Hongyu Gong and Paco Guzman, WikiMatrix: Mining 135M Parallel Sentences in 1620 Language Pairs from Wikipedia arXiv, July 11 2019.
