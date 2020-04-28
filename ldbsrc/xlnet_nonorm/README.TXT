This is a version of the xlnet tokenization model without normalization. 

This model assumes someone normalizes entire input before calling TextToIds.

See README.TXT in <BlingFire>/ldbsrc/xlnet directory for details.


Parity testing:

> cat input.utf8 | python ../scripts/test_bling_with_offsets.py -m xlnet_nonorm.bin -p spiece.model -k 1  > output.utf8

> cat output.utf8 | awk '/ERROR:/' | wc -l
3976

> wc -l input.utf8 
2052515 input.utf8


99.8% parity without normalization




Measure perf:

> time -p cat input.utf8 | python ../scripts/test_sp.py -m spiece.model -s 1 
real 218.04
user 217.69
sys 1.19
>  time -p cat input.utf8 | python ../scripts/test_bling.py -m xlnet_nonorm.bin  -s 1 
real 99.57
user 99.32
sys 1.48
