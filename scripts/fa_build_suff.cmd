@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_build_suff [OPTIONS]

This program builds suffix rules. Builds Moore Multi automaton and action map.

  --in=<input-rules> - specifies the input rules file name,
    stdin is used by default

  --tagset=<input-file> - reads input tagset from the <input-file>,
    does not use tagset by default

  --out1=<fsa> - Moore Multi automaton file name,
    suff.fsa.txt is used by default

  --out2=<act-map> - map of actions,
    acts.map.txt is used by default

  --input-enc=<enc> - input encoding,
    \"UTF-8\" - is used by default

  --generalize - reduces the length of left suffixes as long as it does not
    contradict original rules

  --min-depth=N - sets up minimum suffix length starting from which
    the transformation will be guaranteed for a previosly unseen word,
    (can be used only with --generalize) the default value is 3

  --keep-state2ows - won't make any extension of State -> Ows map,
    --min-depth=N parameter will be ignored

  --ows-merge=<type> - specifies how Ows are merged, if extending state2ows
    or - union of Ows, is used by default
    and - intersection of Ows

  --ows-bound=N - sets up % of Ows to be used for State2Ow extension, from
    more to less frequent; all are taken by default

  --line-step=N - the amount of entries processed at once,
    by default processes the whole input

EOM

}

#
# *** Process command line parameters ***
#

$in_rules = "" ;
$aut_rules = "suff.fsa.txt" ;
$map_act = "acts.map.txt" ;
$input_enc = "--input-enc=UTF-8" ;
$min_depth = "--min-depth=3" ;
$keep_state2ows = "" ;
$ows_merge = "";
$ows_bound = "";
$tagset = "";
$generalize = "" ;
$imp_delim = "" ;
$tmp1 = "" ;
$tmp2 = "" ;
$MAX_LINES_AT_ONCE = 0 ;


while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--input-enc=./) {

        $input_enc = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--min-depth=./) {

        $min_depth = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--ows-merge=./) {

        $ows_merge = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--ows-bound=./) {

        $ows_bound = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--keep-state2ows/) {

        $keep_state2ows = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $in_rules = $1;

    } elsif ($ARGV [0] =~ /^--tagset=./) {

        $tagset = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--out1=(.+)/) {

        $aut_rules = $1;

    } elsif ($ARGV [0] =~ /^--out2=(.+)/) {

        $map_act = $1;

    } elsif ("--generalize" eq $ARGV [0]) {

        $generalize = "--generalize";
        $imp_delim = "--imp-delim";

    } elsif ($ARGV [0] =~ /^--line-step=(.+)/) {

        $MAX_LINES_AT_ONCE = 0 + $1;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_build_suff --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}

#
#  *** Create temporary files ***
#

($fh, $tmp1) = mkstemp ("fa_build_suff_XXXXXXXX");
close $fh;
($fh, $tmp2) = mkstemp ("fa_build_suff_XXXXXXXX");
close $fh;
($fh, $tmp3) = mkstemp ("fa_build_suff_XXXXXXXX");
close $fh;
($fh, $tmp4) = mkstemp ("fa_build_suff_XXXXXXXX");
close $fh;

#
# Iw classification
#

$ow_base = "65536" ;
$ow_max = "2000000" ;

#
# *** Build the rules ***
#

$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_build_suff" };
$ENV{"LC_ALL"} = "C";


if(0 == $MAX_LINES_AT_ONCE) {

  # 1. Build list of chains and action map
  # 2. sort, uniq
  # 3. Build Min RS DFA
  # 4. Convert RS DFA into Multi-Moore DFA
  $command = "".
    "cat $in_rules | ".
    "fa_suff2chains --out-ow2f=$tmp2 --num-size=7 $input_enc $imp_delim $tagset --key-base=$ow_base --out-actions=$map_act | ".
    "sort | uniq | ".
    "fa_chains2mindfa | ".
    "fa_fsm2fsm --in-type=rs-dfa --out-type=moore-mdfa --ow-base=$ow_base --ow-max=$ow_max --out=$aut_rules " ;

  `$command` ;

} else {

  $line_num = 0 ;

  # 1. cat from file or standard input
  # 2. convert input suffix rules into chains and build a global action map
  $command1 = "".
    "cat $in_rules | ".
    "fa_suff2chains --out-ow2f=$tmp2 --num-size=7 $input_enc $imp_delim $tagset --key-base=$ow_base --out-actions=$map_act | ";

  # 1. lexicographically sort chains and build an RS DFA
  $command2 = "| sort | uniq | fa_chains2mindfa > $tmp1" ;

  # 1. merge common RS DFA and a portion RS DFA inot one RS NFA
  # 2. make determinization and minimization
  $command3 = "".
    "cat $tmp3 $tmp1 | ".
    "fa_nfalist2nfa --alg=dfa-union | ".
    "fa_nfa2mindfa --alg=br > $tmp4" ;

  open INPUT, $command1 ;

  while(<INPUT>) {

    if (0 == $line_num) {
      open OUTPUT, $command2 ;
    }

    $line_num++;
    print OUTPUT $_ ;

    if ($MAX_LINES_AT_ONCE == $line_num) {

      # close is waiting for $command2 pipe to be finished
      close OUTPUT ;

      if(-z $tmp3) {
        rename $tmp1, $tmp3 ;
      } else {
        # merge a big dictionary (tmp3) and a small portion (tmp1) together
        `$command3` ;
        rename $tmp4, $tmp3 ;
      }

      $line_num = 0;
    }
  }

  # close is waiting for $command2 pipe to be finished
  close OUTPUT;

  if (0 != $line_num) {

    if (-z $tmp3) {
      rename $tmp1, $tmp3 ;
    } else {
      # merge a big dictionary (tmp3) and a small portion (tmp1) together
      `$command3` ;
      rename $tmp4, $tmp3 ;
    }
  }

  close INPUT ;

  # 1. Convert Common RS DFA into a Moore DFA
  $command = "".
    "fa_fsm2fsm --in-type=rs-dfa --out-type=moore-mdfa --ow-base=$ow_base --ow-max=$ow_max --out=$aut_rules < $tmp3 " ;

  `$command` ;
}

#
# *** Generalize the dictionary ***
#

if ($generalize eq "--generalize") {

    rename $aut_rules, $tmp1 ;

    # 1. Make context generalization
    # 2. Remove gaps in state space

    $command = "".
      "fa_dict2classifier --in=$tmp1 --in-ow2f=$tmp2 --extend-finals $keep_state2ows $min_depth $ows_merge $ows_bound | ".
      "fa_fsm_renum --fsm-type=moore-mdfa --alg=remove-gaps > $aut_rules " ;

    `$command` ;
}


#
# *** Make action map sorted lexicographically ***
#

rename $aut_rules, $tmp1 ;
rename $map_act,   $tmp2 ;

$command = "fa_fsm_renum --alg=mmap-sort --fsm-type=moore-mdfa --in-map=$tmp2 --out-map=$map_act < $tmp1 > $aut_rules" ;
`$command` ;


#
#  *** Remove temporary files ***
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
