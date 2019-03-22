@perl -Sx %0 %*
@goto :eof
#!perl


use File::Temp qw/ :mktemp  /;


sub usage {

print <<EOM;

Usage: fa_count2prob_ngram [OPTIONS]

This program reads a Ngram\\tTag\\tCount\\n file and calculates and prints
to stdout Ngram\\tTag\\tProb\\n lines, where Prob is P(Ngr|T) or P(T|Ngr).


  --in=<input> - an input file with Word\\tTag\\tCount\\n triplets (one per 
    line), the text should be in the UTF-8 encoding, stdin is used by default

  --out=<output> - output file, output.txt is used by default

  --kn-cut-off=N - treats all words with the word frequency N and higher as
    "known", and calculates their P(T|W) probabilities based on counts,
    5 is used by default

  --calc-pwt - calculates P(Ngr|T) instead of P(T|Ngr)

Transformation:

  --in-tr=<trs> - specifies input transformation type
    <trs> is comma-separated array of the following:
      hyph-redup - hyphenated reduplication
      hyph-redup-rev - reverse hyphenated reduplication
      pref - prefix transformation: represents special prefixes as suffixes
      pref-rev - reversed prefix transformation
      ucf - encodes upper-case-first symbol in a suffix
      ucf-rev - reversed UCF transformation

  --redup-delim=N - reduplication delimiter.

  --pref-delim=N - prefix transformation delimiter

  --pref-fsm=<fsm> - keeps dictionary of prefixes to be treated as suffix,
    used only with --in-tr=pref or --out-tr=pref

  --ignore-case - converts input symbols to the lower case,
    uses simple case folding algorithm due to Unicode 4.1.0

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format
EOM

}


$input = "" ;
$output = "" ;
$kn_cut_off = 5 ;
$normalization = "" ;
$calc_pwt = 0;


while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $input = $1;

    } elsif ($ARGV [0] =~ /^--out=(.+)/) {

        $output = $1;

    } elsif ($ARGV [0] =~ /^--kn-cut-off=(.+)/) {

        $kn_cut_off = 0 + $1;

    } elsif ("--calc-pwt" eq $ARGV [0]) {

        $calc_pwt = 1;

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $normalization = $normalization . " " . $ARGV [0];

    } elsif ($ARGV [0] =~ /^--in-tr=./) {

        $normalization = $normalization . " " . $ARGV [0];

    } elsif ($ARGV [0] =~ /^--redup-delim=./) {

        $normalization = $normalization . " " . $ARGV [0];

    } elsif ($ARGV [0] =~ /^--ucf-delim=./) {

        $normalization = $normalization . " " . $ARGV [0];

    } elsif ($ARGV [0] =~ /^--pref-delim=./) {

        $normalization = $normalization . " " . $ARGV [0];

    } elsif ($ARGV [0] =~ /^--pref-fsm=./) {

        $normalization = $normalization . " " . $ARGV [0];

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $normalization = $normalization . " " . $ARGV [0];

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_count2prob_ngram --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


#
# Swaps field 1 and field 2 in tab delimited records
#
# Input:
#   <f1>\t<f2>\t<f3>\n
#   ...
#
# Output:
#   <f2>\t<f1>\t<f3>\n
#   ...
#

$proc1 = <<'EOF';

while(<>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split("\t", $_, 9999);

    if (2 != $#Fld) {
        print STDERR "ERROR: Invalid input line: \"$_\"";
        exit (1);
    }

    print "$Fld[1]\t$Fld[0]\t$Fld[2]\n";
}

EOF


($fh, $tmp1) = mkstemp ("fa_count2prob_ngram_XXXXXXXX");
print $fh $proc1;
close $fh;



#
# 1. Normalize the input (case, charmap, etc.)
# 2. Merge counts
# 3. Apply cut-off
# 4. Calculate the probabilities 
#

$command = ""
  ."cat $input | "
  ."fa_line_format --wts $normalization | "
  ."fa_merge_stat | "
  ."fa_cutoff --min=$kn_cut_off | " ;


if(1 == $calc_pwt) {

  $command .= ""
    ."perl $tmp1 | "
    ."fa_count2prob | "
    ."perl $tmp1 > $output" ;

} else {

  $command .= ""
    ."fa_count2prob > $output" ;
}


`$command` ;


#
# delete temporary files
#

END {
    if ($tmp1 && -e $tmp1) {
        unlink ($tmp1);
    }
}
