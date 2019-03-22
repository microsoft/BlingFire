@perl -Sx %0 %*
@goto :eof
#!perl

sub usage {

print <<EOM;

Usage: fa_build_wre [OPTIONS] [ < wre-rules.txt ] [ > output.txt ]

This program builds WRE rules.

  --in=<input-rules> - specifies the file name of input WRE rules,
    stdin is used if omited

  --tagset=<input-file> - reads input tagset from the <input-file>,
    is not used by default

  --dict-root=<path> - specifies dictionary path, if not specified the
    dictionary path is taken from the \$DICTS_ROOT environment variable

  --tagset2=<input-file> - if this parameter is specified then all WRE @TAG
    references are resolved by the tag-dictionary and corresponding digitizer
    will be constructed, not used by default

  --ldb=<ldb-dump> - specifies PRM LDB file, it is needed for tag-dictionary
    digitizer, not used by default

  --out=<output> - writes compiled WRE into <output> file
    stdout is used by default

  --build-dump - builds a single memory-dump output file

  --out3=<filename> - specifies the file name where C++ code for the right
    parts will be stored, does not print C++ output by default

  --input-enc=<enc> - input encoding, "UTF-8" - is used by default

  --type=<type> - is one of the following:
    rs - Rabin-Scott automaton (text -> Yes/No), is used by default
    moore - Moore automaton (text -> RuleNum)
    mealy - Mealy automaton (text -> TrBr)

  --mode=debug|release - runs debug or release mode of executables,
    release - is used by default

  --no-preproc - no preprocessor will be called (no _define, _inlude or
    comments allowed, empty line is used as a rule delimiter)

  --verbose - prints additional information during the compilation

 To create an external digitizer:

  --out-ext=<filename> - stores Token <-> Num, Token -> Type CNF and Type ->
    Ows map, needed for external digitizer; does not print this by default

 To use an external digitizer:

  --in-ext=<filename> - reads Token <-> Num, Token -> Type CNF and Type ->
    Ows map, needed for external digitizer; does not read this by default

EOM

}


#
# *** Process command line parameters ***
#

$in_rules = "" ;
$tagset = "" ;
$tagset2 = "" ;
$ldb = "" ;
$fsm_type = "--type=rs" ;
$out = "";
$build_dump = "" ;
$cpp_file = "/dev/null" ;
$mode = "" ;
$input_enc = "--input-enc=UTF-8" ;
$verbose = "" ;
$optimize_weights = "" ;
$no_preproc = "" ;
$no_trivial = "";
$out_ext = "";
$in_ext = "";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--input-enc=./) {

        $input_enc = $ARGV [0];

    } elsif ("--mode=debug" eq $ARGV [0]) {

        $mode = "_debug";

    } elsif ("--mode=release" eq $ARGV [0]) {

        $mode = "";

    } elsif ($ARGV [0] =~ /^--in=(.+)/) {

        $in_rules = $1;

    } elsif ($ARGV [0] =~ /^--tagset=./) {

        $tagset = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--tagset2=./) {

        $tagset2 = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--ldb=./) {

        $ldb = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--dict-root=(.+)/) {

        $ENV{'DICTS_ROOT'} = $1;

    } elsif ($ARGV [0] =~ /^--out=./) {

        $out = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--build-dump/) {

        $build_dump = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--out3=(.+)/) {

        $cpp_file = $1;

    } elsif ("--type=rs" eq $ARGV [0]) {

        $fsm_type = $ARGV [0];

    } elsif ("--type=moore" eq $ARGV [0]) {

        $fsm_type = $ARGV [0];

    } elsif ("--type=mealy" eq $ARGV [0]) {

        $fsm_type = $ARGV [0];

    } elsif ("--verbose" eq $ARGV [0]) {

        $verbose = $ARGV [0];

    } elsif ("--no-preproc" eq $ARGV [0]) {

        $no_preproc = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--out-ext=./) {

        $out_ext = $ARGV [0];

    } elsif ($ARGV [0] =~ /^--in-ext=./) {

        $in_ext = $ARGV [0];

    } elsif ("--no-trivial" eq $ARGV [0]) {

        $no_trivial = $ARGV [0];

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "fa_build_wre\tERROR: Unknown parameter $$ARGV[0], see fa_build_wre --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_build_wre" };


#
# 1. optional preprocessing
# 2. compilation
#

if ($no_preproc eq "--no-preproc") {
    $preproc = "" ;
} else {
    $preproc = "fa_preproc --cpp-out=$cpp_file | " ;
}


$input_params = "$fsm_type $input_enc $tagset $tagset2 $ldb --dict-root=$ENV{'DICTS_ROOT'} $in_ext" ;
$output_params = "$out_ext $out $build_dump"  ;

$command = "".
  "cat $in_rules | ".
  $preproc .
  "fa_wrec$mode $input_params $output_params " ;

`$command` ;

if ($verbose eq "--verbose") {
    print STDERR "\nfa_build_wre\tCompilation is done.\n" ;
}

