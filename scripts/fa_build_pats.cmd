@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_build_pats [OPTIONS]

This program exracts 100% correct patterns from the annotated input. Input
format:  a[XX]b[YY]...c[ZZ], where a,b,c - are non-empty input strings and
[XX],[YY],[ZZ] - are optional output symbols, (see FATrWordIOTools_utf8).


  --in=<input> - input file name, should be the actual file name (as it has
    to be read two times), data should be in UTF-8 encoding

  --out=<output-file> - generated patterns, writes to the <output-file>,
    if omited stdout is used

  --out-unsolved=<unsolved> - writes unsolved entries to the <unsolved> file,
    if omited does not print them

  --min-length=N - sets up mimimal pattern length, 3 is used by default

  --max-length=N - sets up maximal pattern length, 8 is used by default

  --spec-l-anchor=N - specifies weight for the beginning of the sequence,
    the default is 94 ('^')

  --spec-r-anchor=N - specifies weight for the end of the sequence,
    the default is 94 ('^')

  --ignore-case - converts input symbols to the lower case,
    uses simple case folding algorithm due to Unicode 4.1.0

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format

  --min-prec=N - sets up a mimimal pattern precision, 100.0 is used by default

  --min-freq=N - sets up a mimimal pattern frequency, 1 is used by default

  --no-empty - will not produce patterns without hypenation points

  --dont-care-pats - generates patterns with "don't care" output symbols

  --max-context=N - specifies the maximum left context for the "don't care"
    patterns, 4 is used by default

  --big-fsms - should be specified in case of big intermediate machines

  --take-all-pats - takes all patterns, skips the stage of calculating a subset
    this key should be used to improve recall on unknown words

  --verbose - prints additional information during the compilation
EOM

}


$input = "input.txt" ;
$output = "" ;
$out_file = "";
$out_unsolved = "" ;
$ignore_case = "" ;
$left_achor = "--spec-l-anchor=94" ;
$right_achor = "--spec-r-anchor=94" ;
$min_len = "--min-length=3" ;
$max_len = "--max-length=8" ;
$min_prec = "--min-prec=100" ;
$min_freq = "--min-freq=1" ;
$no_empty = "";
$dont_care = "" ;
$max_context = "" ;
$format = "--format=dump" ;
$take_all_pats = "";
$verbose = "" ;


while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $input = $1;

    } elsif ($ARGV [0] =~ /^--out=(.+)/) {

        $output = $ARGV [0];
        $out_file = $1;

    } elsif ($ARGV [0] =~ /^--out-unsolved=./) {

        $out_unsolved = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--spec-l-anchor=./) {

        $left_achor = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--spec-r-anchor=./) {

        $right_achor = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--min-length=./) {

        $min_len = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--max-length=./) {

        $max_len = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--min-prec=./) {

        $min_prec = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--min-freq=./) {

        $min_freq = $ARGV [0];

    } elsif ("--no-empty" eq $ARGV [0]) {

        $no_empty = $ARGV [0];

    } elsif ("--dont-care-pats" eq $ARGV [0]) {

        $dont_care = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--max-context=./) {

        $max_context = $ARGV [0];

    } elsif ("--big-fsms" eq $ARGV [0]) {

        $format = "--format=txt";

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ("--verbose" eq $ARGV [0]) {

        $verbose = "--verbose";

    } elsif ("--take-all-pats" eq $ARGV [0]) {

        $take_all_pats = $ARGV [0] ;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_build_pats --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


#
# Create temporary files
#

($fh, $tmp1) = mkstemp ("fa_build_pats_XXXXX");
close $fh;
($fh, $tmp2) = mkstemp ("fa_build_pats_XXXXX");
close $fh;
($fh, $tmp3) = mkstemp ("fa_build_pats_XXXXX");
close $fh;
($fh, $tmp4) = mkstemp ("fa_build_pats_XXXXX");
close $fh;

#
# build all patterns
#

$command = "".
  "fa_hyph2chains --in=$input $min_len $max_len $left_achor $right_achor $ignore_case | ".
  "sort | uniq -c | ".
  "fa_iwowsuff2pats $min_len $min_prec $min_freq $no_empty $dont_care $max_context --out=$tmp4 ";

`$command` ;

if ($verbose eq "--verbose") {
  print STDERR "\nfa_build_pats\tAll patterns have been extracted.\n" ;
}


#
# Store all patterns into the MPH-K2I-I2INO lexicon
#
`fa_build_dict --input-enc=UTF-8 --hyph --type=mph --in=$tmp4 --out-fsm=$tmp1 --out-k2i=$tmp2 --out-i2info=$tmp3 `;

if ($format eq "--format=dump") {

  `fa_fsm2fsm_pack --in=$tmp1 --out=$tmp4 --type=mealy-dfa --auto-test` ;
   rename $tmp4, $tmp1 ;

  `fa_fsm2fsm_pack --in=$tmp2 --out=$tmp4 --type=arr --auto-test` ;
   rename $tmp4, $tmp2 ;

  `fa_fsm2fsm_pack --in=$tmp3 --out=$tmp4 --type=mmap --auto-test` ;
   rename $tmp4, $tmp3 ;
}

if ($verbose eq "--verbose") {
  print STDERR "\nfa_build_pats\tAll patterns dictionary has been built.\n" ;
}


#
# Remove redundancies and print out unsolved entries, if asked
#
`fa_pats_select $format $no_empty --in=$input --fsm=$tmp1 --k2i=$tmp2 --i2info=$tmp3 $left_achor $right_achor $ignore_case $out_unsolved $take_all_pats $output` ;

if ($verbose eq "--verbose") {
  print STDERR "\nfa_build_pats\tSubset has been extracted and validated.\n" ;
}


#
#  Remove temporary files
#

END {
    if (-e $tmp1) {
        unlink ($tmp1);
    }
    if (-e $tmp2) {
        unlink ($tmp2);
    }
    if (-e $tmp3) {
        unlink ($tmp3);
    }
    if (-e $tmp4) {
        unlink ($tmp4);
    }
}
