@perl -Sx %0 %*
@goto :eof
#!perl


use File::Temp qw/ :mktemp  /;


sub usage {

print <<EOM;

Usage: fa_build_ngrams [OPTIONS]

This program reads a Ngram\\tTag\\tCount\\n file and builds an automaton
which returns P(Ngr|T) or P(T|Ngr) probability values for all ngrams above
the specified cut-off. The resulting automaton can be used by the 
FAWordGuesser_prob_t interpreter.

  --in=<input> - an input file with Word\\tTag\\tCount\\n triplets (one per 
    line), the text should be in the UTF-8 encoding, stdin is used by default

  --input-enc=<enc> - input encoding, "UTF-8" - is used by default

  --tagset=<input-file> - reads input tagset from the <input-file>,
    tagset.txt is used by default

  --out=<fsa> - specifies the file name of the output automaton,
    w2tp.fsa.txt is used by default

  --kn-cut-off=N - treats all words with the word frequency N and higher as
    "known", and calculates their P(T|W) probabilities based on counts,
    5 is used by default

  --no-rescale - the input has already been scaled to the range given by
    <min-prob> and <max-prob>, no need to scale again

  --log-scale - calculates natural logarithm before rescaling, cannot be
    combined with --no-rescale

  --min-prob=<min-prob> - specifies the minimum integer value for the minimum
    of the probability, 0 is used by default

  --max-prob=<max-prob> - specifies the maximum integer value for the maximum
    of the probability, 255 is used by default

  --out-minmax=<file-name> - prints out minimum and maximum probability or
    log(probability) values to the output file, does not print by default,
    cannot be combined with --no-rescale

  --dir=<direction> - specifies classification direction:
    l2r - left to right
    r2l - right to left (the dafault value)

  --calc-pwt - calculates P(Ngr|T) instead of P(T|Ngr)

  --raw - stores precalculated probability vaules, the input should contain
    probability vaules not counts

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

Generalization:

  --no-key-delim - do not add delimiter 0 before keys

EOM

}


$input = "" ;
$tagset = "tagset.txt" ;
$output = "w2tp.fsa.txt" ;
$kn_cut_off = 5 ;
$min_prob = 0 ;
$max_prob = 255 ;
$log_scale = "" ;
$no_rescale = 0 ;
$normalization = "" ;
$dir = "--dir=r2l" ;
$input_enc = "--input-enc=UTF-8" ;
$no_key_delim = "";
$calc_pwt = 0;
$out_minmax = "";
$raw = 0;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--input-enc=./) {

        $input_enc = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $input = $1;

    } elsif ($ARGV [0] =~ /^--tagset=(.+)/) {

        $tagset = $1;

    } elsif ($ARGV [0] =~ /^--out=(.+)/) {

        $output = $1;

    } elsif ($ARGV [0] =~ /^--dir=./) {

        $dir = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--kn-cut-off=(.+)/) {

        $kn_cut_off = 0 + $1;

    } elsif ($ARGV [0] =~ /^--max-prob=(.+)/) {

        $max_prob = 0 + $1;

    } elsif ($ARGV [0] =~ /^--min-prob=(.+)/) {

        $min_prob = 0 + $1;

    } elsif ($ARGV [0] =~ /^--out-minmax=./) {

        $out_minmax = $ARGV [0];

    } elsif ($ARGV [0] eq "--log-scale") {

        $log_scale = $ARGV [0];

    } elsif ("--calc-pwt" eq $ARGV [0]) {

        $calc_pwt = 1;

    } elsif ("--no-rescale" eq $ARGV [0]) {

        $no_rescale = 1;

    } elsif ("--raw" eq $ARGV [0]) {

        $raw = 1;

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

    } elsif ($ARGV [0] eq "--no-key-delim") {

        $no_key_delim = $ARGV [0];

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_build_ngrams --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}

#
# Input:
#   <word>\t<tag>\t<prob>\n
#   ...
#
# Output1 (the new WT list):
#   <word>\t<tag>_<prob>\n
#   ...
#
# Output2 (the new tagset):
#   <tag>_<prob> <value>\n
#   ...
#

$proc1 = <<'EOF';

#
# Get the MaxProb value
#

$max_prob_value = $ARGV [0];
shift @ARGV;

#
# Read in the old tagset
#

$old_tf = $ARGV [0];
shift @ARGV;

open OLDTAGSET, "< $old_tf" ;

while(<OLDTAGSET>) {

    s/[\r\n]+$//;

    m/^(.+)[ ]+([0-9]+)$/;

    if (0 < 0 + $2) {
      $old_tagset{$1} = 0 + $2;
    }
}

close OLDTAGSET ;

#
# Read <W, T, P> list and build a <W, TxP> list and a <TxP> tagset.
#

$tf = $ARGV [0];
shift @ARGV;

while(<>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split("\t", $_, 9999);

    if (2 != $#Fld) {
        print STDERR "ERROR: Invalid input line: \"$_\"";
        exit (1);
    }

    print "$Fld[0]\t$Fld[1]_$Fld[2]\n";

    $newtag = "$Fld[1]_$Fld[2]";

    if(!(defined $tagset{$newtag})) {

      $oldval = $old_tagset{$Fld[1]};

      if(0 == 0 + $oldval) {
        print STDERR "ERROR: Unknown tag \"$Fld[1]\" in the \"$old_tf\" file.";
        exit (1);
      }

      $newval = ($oldval * ($max_prob_value + 1)) + (0 + $Fld[2]);

      $tagset{$newtag} = $newval;
    }
}

#
# Print the new (TxP) tagset
#

open TAGSET, ">$tf" ;

foreach $k (sort keys %tagset) {
    print TAGSET "$k $tagset{$k}\n";
}

close TAGSET ;

EOF


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

$proc2 = <<'EOF';

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


($fh, $tmp1) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
print $fh $proc1;
close $fh;

($fh, $tmp2) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
close $fh;

($fh, $tmp3) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
close $fh;

($fh, $tmp5) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
print $fh $proc2;
close $fh;


# 1. Swap tag and word, if needed

$swap_fields = "";

if(1 == $calc_pwt) {
  $swap_fields = "perl $tmp5 | ";
}


# 1. Normalize the input (case, charmap, etc.)
# 2. Merge counts
# 3. Measure the probabilities of words above the cut-off

$calc_prob = "";

if(0 == $raw) {
  $calc_prob = ""
    ."fa_line_format --wts $normalization | "
    ."fa_merge_stat | "
    .$swap_fields
    ."fa_count2prob --cutoff=$kn_cut_off | "
    .$swap_fields ;
}


# Rescale the probabilities to the range [min-prob, max-prob]
$rescale_prob = "";

if(0 == $no_rescale) {
  $rescale_prob = "fa_num2int --field=3 --min=$min_prob --max=$max_prob $log_scale $out_minmax | ";
}

# 1. Calculate the probabilities
# 2. Quantize the probabilities into a fixed range
# 3. Make a new TxP tagset and change the input word list

$command = ""
  ."cat $input | "
  .$calc_prob
  .$rescale_prob
  ."perl $tmp1 $max_prob $tagset $tmp2 > $tmp3 " ;

`$command` ;


#
# Build the guesser over TxP tagset without State -> Ows map extension
#

`fa_build_word_guesser --full-unicode --dict-mode $no_key_delim $input_enc $normalization $dir --in=$tmp3 --tagset=$tmp2 --out=$tmp1` ;

#
# Change [CxP1, ..., CxPn] Ows into [C1, ..., Cn, P1, ..., Pn] Ows
#

`fa_fsm_renum --in=$tmp1 --out=$output --fsm-type=moore-mdfa --alg=cxps-to-csps --max-prob=$max_prob` ;



#
# delete temporary files
#

END {
    if ($tmp1 && -e $tmp1) {
        unlink ($tmp1);
    }
    if ($tmp2 && -e $tmp2) {
        unlink ($tmp2);
    }
    if ($tmp3 && -e $tmp3) {
        unlink ($tmp3);
    }
    if ($tmp5 && -e $tmp5) {
        unlink ($tmp5);
    }
}
