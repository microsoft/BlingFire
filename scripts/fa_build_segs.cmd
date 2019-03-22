@perl -Sx %0 %*
@goto :eof
#!perl


sub usage {

print <<EOM;

Usage: fa_build_segs [OPTIONS]

This program reads a Word\\tCount\\n file then maps Counts into the weights
(probability or log probability) and builds equivalent automaton. The input
should be in UTF-8.

  --in=<input> - an input file with Word\\tCount\\n triplets (one perline),
    the text should be in the UTF-8 encoding, wt.utf8 is used by default

  --out=<fsa> - specifies the file name of the output automaton,
    w2tp.fsa.txt is used by default

  --cut-off=N - excludes segments with Count < N, 0 is used by default

  --log-scale - calculates natural logarithm before rescaling

  --max-prob=N - specifies the maximum integer value for the 1.0
    of the probability

  --out-minmax=<file-name> - prints out minimum and maximum input or log(input)
    values to the output file, does not print by default

  --dir=<direction> - specifies word reading direction:
    l2r - left to right
    r2l - right to left (the dafault value)

  --ignore-case - converts input symbols to the lower case,
    uses simple case folding algorithm due to Unicode 4.1.0

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format

  --raw - the counts are final already they should be stored into automaton 
    without modification (--cut-off=N, --log-scale, --max-prob=N are not used)

  --input-enc=<enc> - input encoding, "UTF-8" - is used by default

  --full-unicode - supports all the characters from 0 to 10FFFF, otherwise
    just works for the BMP characters only
EOM

}


#
# Iw classification
#

$ow_base = "65536" ;
$ow_max = "1000000" ;
$num_size = "" ;
$base1 = "" ;
$base2 = "" ;


#
# *** Process command line parameters ***
#
$input = "" ;
$output = "" ;
$ignore_case = "" ;
$dir = "--dir=r2l" ;
$log_scale = "" ;
$max_prob = 255 ;
$cutoff = 0 ;
$raw = "" ;
$out_minmax = "";
$input_enc = "--input-enc=UTF-8" ;
$full_unicode = "";


while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $input = $1;

    } elsif ($ARGV [0] =~ /^--out=(.+)/) {

        $output = $1;

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] eq "--log-scale") {

        $log_scale = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--max-prob=(.+)/) {

        $max_prob = 0 + $1;

    } elsif ($ARGV [0] =~ /^--out-minmax=./) {

        $out_minmax = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--cut-off=(.+)/) {

        $cutoff = 0 + $1;

    } elsif ($ARGV [0] eq "--dir=l2r") {

        $dir = $ARGV [0];

    } elsif ($ARGV [0] eq "--dir=r2l") {

        $dir = $ARGV [0];

    } elsif ($ARGV [0] eq "--raw") {

        $raw = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--input-enc=./) {

        $input_enc = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--full-unicode/) {

        $full_unicode = $ARGV [0];

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_build_segs --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


# full unicode range support, slows down the compilation
if("" ne $full_unicode) {
    # U10FFFFh + 1
    $ow_base = "1114112" ;
    # U10FFFFh + 1 + 1072627711
    $ow_max = "1073741823" ;
    # use 6 digit hex numbers for sorting
    $num_size = "--num-size=6" ;
    $base1 = "--base=16" ;
    $base2 = "--base=hex" ;
}


$command = "cat $input | ";

if ($raw eq "") {

  # 1.  apply case/character class normalization
  # 2.  merge counts together for duplicate words
  # 3.  cut off words with counts less than $cutoff
  # 4.  calculate word probabilities
  # 5.  map probability/log-prob into an integer scale

  $command = $command
    ."fa_line_format $ignore_case | "
    ."fa_merge_stat | "
    ."fa_cutoff --min=$cutoff | "
    ."fa_count2prob | "
    ."fa_num2int --field=2 --min=0 --max=$max_prob $log_scale $out_minmax | " ;
}


# 1. digitalize the words (possibly change the order)
# 2. sort chains
# 3. build MIN RS DFA
# 4. renumerate states
# 5. build a Moore DFA

$command = $command
  ."fa_line2chain_unicode $num_size $base1 $dir $ignore_case $input_enc --use-keys --key-base=$ow_base | "
  ."sort | "
  ."fa_chains2mindfa $base2 | "
  ."fa_fsm_renum --alg=remove-gaps --fsm-type=rs-dfa | "
  ."fa_fsm2fsm --in-type=rs-dfa --out-type=moore-dfa --ow-base=$ow_base --ow-max=$ow_max > $output " ;

`$command` ;

