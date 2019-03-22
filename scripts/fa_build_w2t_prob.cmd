@perl -Sx %0 %*
@goto :eof
#!perl


use File::Temp qw/ :mktemp  /;


sub usage {

print <<EOM;

Usage: fa_build_w2t_prob [OPTIONS]

This program reads a Word\\tTag\\tCount\\n file and builds an automaton which 
returns P(T|W) probability values. Two passes are done. First, tag sets and 
probabilities are measured for "known" words (the frequency is not lower than
the kn-cut-off). Second, tag sets and probabilities are estimated for "unknown"
words by words with the frequency un-cut-off or lower.


  --in=<input> - an input file with Word\\tTag\\tCount\\n triplets (one per line),
    the text should be in the UTF-8 encoding, wt.utf8 is used by default

  --input-enc=<enc> - input encoding, "UTF-8" - is used by default

  --tagset=<input-file> - reads input tagset from the <input-file>,
   tagset.txt is used by default

  --out=<fsa> - specifies the file name of the output automaton,
    w2tp.fsa.txt is used by default

  --kn-cut-off=N - treats all words with the word frequency N and higher as
    "known", and calculates their P(T|W) probabilities based on counts,
    5 is used by default

  --un-cut-off=M - uses all words with the word frequency M and lower to 
    calculate the P(T|S) for all states S in the automaton not corresponding
    to any "known" words, 15 is used by default

  --max-prob=<max-prob> - specifies the maximum integer value for the 1.0 of
    the P(T|W) or P(T|S) probability, 255 is used by default

  --dir=<direction> - specifies classification direction:
    l2r - left to right
    r2l - right to left (the dafault value)

  --dict-mode - does not shrink the paths, and does not modify State -> Ows map
    (this also makes unknown words cut off to be 0)

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


$input = "wt.utf8" ;
$tagset = "tagset.txt" ;
$output = "w2tp.fsa.txt" ;
$kn_cut_off = 5 ;
$un_cut_off = 15 ;
$max_prob = 255 ;
$normalization = "" ;
$dir = "--dir=r2l" ;
$dict_mode = "";
$input_enc = "--input-enc=UTF-8" ;


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

    } elsif ($ARGV [0] =~ /^--dict-mode/) {

        $dict_mode = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--dir=./) {

        $dir = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--kn-cut-off=(.+)/) {

        $kn_cut_off = 0 + $1;

    } elsif ($ARGV [0] =~ /^--un-cut-off=(.+)/) {

        $un_cut_off = 0 + $1;

    } elsif ($ARGV [0] =~ /^--max-prob=(.+)/) {

        $max_prob = 0 + $1;

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

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_build_w2t_prob --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}

if ("" ne $dict_mode) {
  $un_cut_off = 0;
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

($fh, $tmp1) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
print $fh $proc1;
close $fh;

($fh, $tmp2) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
close $fh;

($fh, $tmp3) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
close $fh;

($fh, $tmp4) = mkstemp ("fa_build_w2t_prob_XXXXXXXX");
close $fh;

#
# 1. Normalize the input (case, charmap, etc.)
# 2. Merge counts
#

$command = ""
  ."fa_line_format --wts $normalization < $input | "
  ."fa_merge_stat > $tmp4" ;

`$command` ;


#
# 1. Measure the probabilities of frequent words
# 2. Quantize the probabilities into a fixed range
# 3. Make a TxP tagset and change the input word list
#

$command = ""
  ."fa_count2prob --cutoff=$kn_cut_off < $tmp4 | "
  ."fa_num2int --field=3 --min=0 --max=$max_prob | "
  ."perl $tmp1 $max_prob $tagset $tmp2 > $tmp3 " ;

`$command` ;


#
# 1. Build the guesser over TxP tagset without State -> Ows map extension
#
# # TODO: remove $normalization from the fa_build_word_guesser call
#

`fa_build_word_guesser $input_enc $normalization --keep-state2ows $dict_mode $dir --in=$tmp3 --tagset=$tmp2 --out=$tmp1` ;


#
# 1. Select the annotation data for State -> Ows map extension
# 2. Digitilize/Normalize the input words and tags
# 3. Extend the initial guesser, see fa_extend_cxp --help
# 4. Change [CxP1, ..., CxPn] Ows into [C1, ..., Cn, P1, ..., Pn] Ows
#
# # TODO: remove $normalization from the fa_line2chain_unicode call
#

$command = ""
  ."fa_cutoff --max=$un_cut_off < $tmp4 | "
  ."fa_line2chain_unicode $input_enc $normalization --key-delim --use-keys $dir --tagset=$tagset | "
  ."fa_extend_cxp --in=$tmp1  --max-prob=$max_prob | "
  ."fa_fsm_renum  --out=$output --fsm-type=moore-mdfa --alg=cxps-to-csps --max-prob=$max_prob " ;

`$command` ;


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
    if ($tmp4 && -e $tmp4) {
        unlink ($tmp4);
    }
}
