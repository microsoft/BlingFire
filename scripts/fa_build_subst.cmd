@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_build_subst [OPTIONS] [< subst.txt] [> output.txt]

This program builds substitution rules. Builds Moore Multi automaton,
extracting brackets maps and action map.

  --in=<input-rules> - specifies the input rules file name,
    stdin is used if omited

  --tagset=<input-file> - reads input tagset from the <input-file>,
    if omited does not use tagset

  --out1=<fsa> - Moore Multi automaton file name,
    subst.fsa.txt is used by default

  --out2=<pos-nfa> - reversed position NFA,
    pos.nfa.txt is used by default

  --out3=<trbr-maps> - maps for extracting brackets,
    trbr.txt is used by default

  --out4=<act-map> - map of actions,
    acts.map.txt is used by default

  --input-enc=<enc> - input encoding, \"UTF-8\" - is used by default

  --rev - compiles rules in the right to left order

  --verbose - prints additional information during the compilation
EOM

}


#
# *** Process command line parameters ***
#

$in_rules = "" ;
$aut_rules = "subst.fsa.txt" ;
$aut_pos = "pos.nfa.txt" ;
$maps_trbr = "trbr.txt" ;
$map_act = "acts.map.txt" ;
$input_enc = "--input-enc=UTF-8" ;
$tagset = "";
$rev = "" ;
$verbose = "" ;
$tmp_nfa = "" ;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--input-enc=./) {

        $input_enc = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $in_rules = $1;

    } elsif ($ARGV [0] =~ /^--tagset=./) {

        $tagset = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--out1=(.+)/) {

        $aut_rules = $1;

    } elsif ($ARGV [0] =~ /^--out2=(.+)/) {

        $aut_pos = $1;

    } elsif ($ARGV [0] =~ /^--out3=(.+)/) {

        $maps_trbr = $1;

    } elsif ($ARGV [0] =~ /^--out4=(.+)/) {

        $map_act = $1;

    } elsif ("--rev" eq $ARGV [0]) {

        $rev = "--rev";

    } elsif ("--verbose" eq $ARGV [0]) {

        $verbose = "--verbose";

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_build_subst --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


#
#  *** Create temporary files ***
#

($fh, $tmp_nfa) = mkstemp ("fa_build_subst_XXXXXXXX");
close $fh;


#
# *** Build the rules ***
#

$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_build_subst" };


# 1. Make preprocessing
# 2. Builds regular expression and extract map of actions
# 3. Simplifies regular expression
# 4. Build Nfa, Position and TrBr maps
# 5. Expand '.' localy


# 6. Build Moore Multi Dfa

$command1 = "".
  "fa_preproc $in_rules | ".
  "fa_subst2re $input_enc $tagset $rev --out-actions=$map_act | ".
  "fa_re2re_simplify $rev --label=digit | ".
  "fa_re2nfa $rev --label=digit --out-trbr=$maps_trbr --out-pos-nfa=$aut_pos > $tmp_nfa";

#  "fa_re2nfa $rev --label=digit --out-trbr=$maps_trbr --out-pos-nfa=$aut_pos | ".
#  "fa_nfa2nfa_any --iw-base=2 > $tmp_nfa";

# execute the command
`$command1` ;

if ($verbose eq "--verbose") {
    print STDERR "\nRules NFA, position NFA and maps have been built.\n" ;
}


$command2 = "".
  "fa_nfa2dfa --spec-any=0 --fsm=pos-rs-nfa --pos-nfa=$aut_pos < $tmp_nfa | ".
  "fa_fsm_renum --fsm-type=moore-mdfa >$aut_rules " ;

# execute the command
`$command2` ;


if ($verbose eq "--verbose") {
    print STDERR "\nRules DFA has been built.\n" ;
}


#
#  *** Remove temporary files ***
#

END {
    if ($tmp_nfa && -e $tmp_nfa) {
        unlink ($tmp_nfa);
    }
}
