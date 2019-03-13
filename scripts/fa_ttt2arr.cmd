@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;


sub usage {

print <<EOM;

Usage: fa_ttt2arr [OPTIONS] < input.utf8 > output.utf8

This program converts trigrams, bigrams or unigrams of tags into a linear 
array. Such that if <[T1, [T2,]] T3, V> in input then V in output at the line 
[(T1 - 1) * MaxTag * MaxTag] + [(T2 - 1) * MaxTag] + (T3 - 1), where [X] 
means optionality of X. Tag values should be 1 based. Each input tuple should
be one per line, elements should be tab separated integer value.

  --tagset=<input-file> - reads input tagset from the <input-file>,
    tagset.txt is used by default

  --max-tag=MaxTag - max tag value, if not specified takes from the tagset

  --default-value=V - the V value used for missing tuples,
    0 is used by default
EOM

}

$ENV{"LC_ALL"} = "C";

$max_tag = 0 ;
$default_value = 0 ;
$tagset = "tagset.txt" ;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--max-tag=(.+)/) {

        $max_tag = 0 + $1;

    } elsif ($ARGV [0] =~ /^--default-value=(.+)/) {

        $default_value = 0 + $1;

    } elsif ($ARGV [0] =~ /^--tagset=(.+)/) {

        $tagset = $1;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_ttt2arr --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


#
# Subprogram:
#
# takes sorted, tab-separated tag values followed by V-values
# prints out an array, indexes are calculated by the formula from the usage
#

$proc1 = <<'EOF';

$max_tag = $ARGV [0];
shift @ARGV;

$default_value = $ARGV [0];
shift @ARGV;

# set array base to 1
$[ = 1;

$prev_idx = -1;

while(<STDIN>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split('\t', $_, 999999);

    $idx = 0;

    if(2 <= $#Fld) {
      $idx += (0 + $Fld[$#Fld - 1] - 1);
    }
    if (3 <= $#Fld) {
      $idx += ($max_tag * (0 + $Fld[$#Fld - 2] - 1));
    }
    if (4 <= $#Fld) {
      $idx += ($max_tag * $max_tag * (0 + $Fld[$#Fld - 3] - 1));
    }
    if (5 <= $#Fld) {
      print STDERR "ERROR: Only unigrams, bigrams and trigrams are supported, in program fa_ttt2arr";
      exit (1);
    }

    $value = 0 + $Fld[$#Fld];

    for ($i = $prev_idx + 1; $i < $idx; ++$i) {
      print $default_value ;
      print "\n";
    }

    print $value;
    print "\n";

    $prev_idx = $idx;
}

EOF

($fh, $tmp1) = mkstemp ("fa_ttt2arr_XXXXXXXX");
print $fh $proc1;
close $fh;


# set array base to 1
$[ = 1;


#
# read in the tagset
#

open TAGSET, "< $tagset" || die "ERROR: cannot open $tagset file.";

while(<TAGSET>) {

    s/[\r\n]+$//;

    m/^(.+)[ ]+([0-9]+)$/;

    if (0 < 0 + $2) {
      $tagset{$1} = 0 + $2;
    }
}

close TAGSET ;

#
# 1. convert tag sequences into number sequences
# 2. sort 
# 3. substitute a set of tuples with array of V
#

$command = "| sort | perl $tmp1 $max_tag $default_value";
open OUTPUT, $command ;

while (<STDIN>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split('\t', $_, 999999);

    for($i = 1; $i < $#Fld; $i++) {

      $t = $Fld[$i];

      if(!(defined $tagset{$t})) {
        print STDERR "ERROR: unknown tag \"$t\"\n" ;
        exit(1);
      }

      printf OUTPUT "%05i\t", $tagset{$t} ;
    }

    printf OUTPUT "%s\n", $Fld[$#Fld] ;
}

close OUTPUT;


#
# remove temporary files
#

END {
    if ($tmp1 && -e $tmp1) {
        unlink ($tmp1);
    }
}
