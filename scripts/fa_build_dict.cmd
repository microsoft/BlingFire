@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_build_dict [OPTIONS]

From the input stream of "W\\tT[\\tT[...]]\\n" lines, this program creates
a dictionary consisting of three structures: MPH W -> Id, Array Id -> InfoId
and Multi Map InfoId -> Info. Where W is a word, T is a tag, \\t is a tab
symbol and Info is a set of tags associated with the word W.

  --in=<input> - input file name, stdin is used if omited

  --out-fsm=<output-file> - writes W -> Id MPH to the <output-file>,
    if omited stdout is used

  --out-k2i=<output-file> - writes Id -> InfoId array to the <output-file>,
    if omited stdout is used

  --out-i2info=<output-file> - writes InfoId -> Info map to the <output-file>,
    if omited stdout is used

  --tagset=<input-file> - reads input tagset from the <input-file>,
    no tagset is used by default

  --input-enc=<enc> - input encoding, "UTF-8" - is used by default

  --type=<type> - selects dictionary algorithm type:
    mph - Mealy automaton based MPH (is used by default)
    moore - Moore automaton based

  --ignore-case - converts input symbols to the lower case,
    uses simple case folding algorithm due to Unicode 4.1.0

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format

  --dir=<direction> - specifies word reading direction:
    l2r - left to right (the dafault value)
    r2l - right to left
    aff - affix first, e.g. last, first, last - 1, first + 1, ...

  --in-tr=<trs> - specifies input transformation type
    <trs> is comma-separated array of the following:
      hyph-redup - hyphenated reduplication
      hyph-redup-rev - reverse hyphenated reduplication
      pref - prefix transformation: represents special prefixes as suffixes
      pref-rev - reversed prefix transformation
      ucf - encodes upper-case-first symbol in a suffix
      ucf-rev - reversed UCF transformation

  --redup-delim=N - reduplication delimiter

  --pref-delim=N - prefix transformation delimiter

  --pref-fsm=<fsm> - keeps dictionary of prefixes to be treated as suffix,
    used only with --in-tr=pref or --out-tr=pref

  --ucf-delim=N - UCF transformation delimiter

  --raw - stores info data as-is, in this mode the input data should not
    contain duplicate keys (this mode is not used by default)

  --tag-prob - Tag Prob mode, all words should be sorted, every input line
    should contain KEY TAG PROB values (not used by default)

  --hyph - Hyph mode, all words should be sorted, every input line
    should contain KEY FREQ OWS values (not used by default)

  --sort-info - applies lexicographical sorting to the InfoId -> Info map

  --float-nums - if specified, allows floating point numbers to be
    inter-mixed with tags
EOM

}


$input = "" ;
$out_fsm = "";
$out_k2i = "";
$out_i2info = "";
$input_enc = "--input-enc=UTF-8" ;
$tagset = "" ;
$ignore_case = "" ;
$type = "--type=mph";
$dir = "--dir=l2r" ;
$in_tr_params = "" ;
$mode = "" ;
$sort_info = "" ;
$float_nums = "" ;


while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--input-enc=./) {

        $input_enc = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $input = $1;

    } elsif ($ARGV [0] =~ /^--out-fsm=(.+)/) {

        $out_fsm = $1;

    } elsif ($ARGV [0] =~ /^--out-k2i=(.+)/) {

        $out_k2i = $1;

    } elsif ($ARGV [0] =~ /^--out-i2info=(.+)/) {

        $out_i2info = $1;

    } elsif ($ARGV [0] =~ /^--tagset=./) {

        $tagset = $ARGV [0];

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ("--float-nums" eq $ARGV [0]) {

        $float_nums = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ("--type=mph" eq $ARGV [0]) {

        $type = $ARGV [0];

    } elsif ("--type=moore" eq $ARGV [0]) {

        $type = $ARGV [0];

    } elsif ("--raw" eq $ARGV [0]) {

        $mode = $ARGV [0];

    } elsif ("--tag-prob" eq $ARGV [0]) {

        $mode = $ARGV [0];

    } elsif ("--hyph" eq $ARGV [0]) {

        $mode = $ARGV [0];

    } elsif ("--sort-info" eq $ARGV [0]) {

        $sort_info = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--dir=./) {

        $dir = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--in-tr=./ ||
             $ARGV [0] =~ /^--redup-delim=./ ||
             $ARGV [0] =~ /^--pref-delim=./ ||
             $ARGV [0] =~ /^--pref-fsm=./ ||
             $ARGV [0] =~ /^--ucf-delim=./) {

        $in_tr_params = ($in_tr_params . " " . $ARGV [0]);

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_build_dict --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


#
# Create temporary files
#

($fh, $tmp1) = mkstemp ("fa_build_dict_XXXXX");
close $fh;
($fh, $tmp2) = mkstemp ("fa_build_dict_XXXXX");
close $fh;


if ("--type=mph" eq $type) {

  #
  # 1. digitize the input
  # 2. make lexicographical sorting
  # 3. build split key and info
  # 4. build min RS automaton to keep all the keys
  # 5. remove state gaps
  # 6. convert min RS automaton into MPH
  #

  $command = "".
    "cat $input | ".
    "fa_line2chain_unicode --use-keys --base=16 --num-size=6 --key-delim $dir $in_tr_params $tagset $input_enc $ignore_case $float_nums | ".
    "sort | uniq | ".
    "fa_dict_split --base=16 --num-size=6 $mode --out-k2i=$out_k2i --out-i2info=$out_i2info | ".
    "fa_chains2mindfa --base=hex | ".
    "fa_fsm_renum --fsm-type=rs-dfa --alg=remove-gaps | ".
    "fa_dfa2mph --type=mealy-dfa --out=$out_fsm";

  `$command` ;

  #
  # 1. sort info lexicographically, if needed
  #

  if ("" ne $sort_info) {

    rename $out_k2i, $tmp1 ;
    rename $out_i2info, $tmp2 ;

    $command = "".
      "fa_fsm_renum --alg=mmap-sort --fsm-type=arr --in=$tmp1 --out=$out_k2i --in-map=$tmp2 --out-map=$out_i2info" ;

    `$command` ;
  }


} elsif ("--type=moore" eq $type) {

  #
  # 1. digitize the input
  # 2. make lexicographical sorting
  # 3. split key and info, leaving info_id at the key side!
  # 4. sort again as the order may change
  # 5. build min RS automaton to keep all the <keys, info_id>
  # 6. convert RS automaton into Moore automaton of key -> info_id
  # 7. remove the gaps from the Moore automaton
  #

  $command = "".
    "cat $input | ".
    "fa_line2chain_unicode --use-keys --key-delim --base=16 --num-size=5 $dir $in_tr_params $tagset $input_enc $ignore_case $float_nums | ".
    "sort | uniq | ".
    "fa_dict_split --no-k2i --base=16 --num-size=5 --info-base=65536 $mode --out-i2info=$out_i2info | ".
    "sort | ".
    "fa_chains2mindfa --base=hex | ".
    "fa_fsm2fsm --in-type=rs-dfa --out-type=moore-dfa --ow-base=65536 --ow-max=2000000 | ".
    "fa_fsm_renum --fsm-type=moore-dfa --alg=remove-gaps > $out_fsm" ;

  `$command` ;

  #
  # 1. sort info lexicographically, if needed
  #

  if ("" ne $sort_info) {

    rename $out_fsm, $tmp1 ;
    rename $out_i2info, $tmp2 ;

    $command = "".
      "fa_fsm_renum --alg=mmap-sort --fsm-type=moore-dfa --in=$tmp1 --out=$out_fsm --in-map=$tmp2 --out-map=$out_i2info" ;

    `$command` ;
  }

}


#
#  *** Remove temporary files ***
#

END {
    if (-e $tmp1) {
        unlink ($tmp1);
    }
    if (-e $tmp2) {
        unlink ($tmp2);
    }
}
