@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;
Usage: fa_split_wtbt [OPTIONS] < wtbt.all.txt > wtbt.train.txt 

This program splits input WTBT into training and testing sets. It cuts out
upto N different words or bases.

  --out-test=<filename> - specifies file name for the output data,
    testing.txt - is used by default

  --word-count=N - specifies the amount of words to cut into the testing set

  --base-count=N - specifies the amount of bases to cut into the testing set

  Note: Only one of two --*-count=N parameters should be specified, 100 bases
        are cut off by default.
EOM

}

$\ = "\n";

#
# process command line parameters
#

$test_size = 100;
$cut_base = 1;
$out_test = "testing.txt";

while (0 < 1 + $#ARGV) {

  if("--help" eq $ARGV [0]) {

      usage ();
      exit (0);

  } elsif ($ARGV [0] =~ /^--out-test=(.+)/) {

      $out_test = $1;

  } elsif ($ARGV [0] =~ /^--word-count=(.+)/) {

      $test_size = 0 + $1;
      $cut_base = 0;

  } elsif ($ARGV [0] =~ /^--base-count=(.+)/) {

      $test_size = 0 + $1;
      $cut_base = 1;

  } else {

      last;
  }
  shift @ARGV;
}

srand();


($fh, $tmp1) = mkstemp ("fa_split_wtbt_XXXXXXXX");
close $fh;

#
# copy input into temporary file
#

`cat > $tmp1` ;

#
# count input size
#

$lines = 0;

open INPUT, "< $tmp1" ;

while (<INPUT>) {
  $lines++;
}

close INPUT ;


#
# select random positions for the test set
#

while((0 + keys(%t)) < $test_size) {

  $pos = int(rand($lines)) ;

  if(!(defined $t{$pos})) {
    $t{$pos} = 1 ;
  }
}


#
# collect a set of base-forms or a set of word-forms from these positions
#

open INPUT, "< $tmp1" ;

$pos = 0;

while (<INPUT>) {

  $pos++;

  if(defined $t{$pos}) {

    chomp ;

    @Fld = split("\t", $_, 9999) ;

    if (0 == $cut_base) {
      $w = $Fld[0] ;
    } else {
      $w = $Fld[2] ;
    }

    $dict{$w} = 1 ;
  }
}

close INPUT ;



#
# split the input
#

open INPUT, "< $tmp1" ;
open TEST, "> $out_test" ;

$pos = 0;

while (<INPUT>) {

    s/[\r\n]+$//;

    @Fld = split("\t", $_, 9999) ;

    if (0 == $cut_base) {

      # The case when base-form matches one of selected words is a special one
      # because base is always a word too. Simply including this entry results
      # into including everything that is associated with that base thus we'll
      # have to calculate a transistive closure to make a proper inclusion over
      # Wi --> Bi, Bi \in Dict, Bi == Wj, Wj --> Bj.
      #
      # Instead, we may simply discard these cases.

      if(defined $dict{$Fld[0]}) {

        print TEST $_ ;

      } elsif(defined $dict{$Fld[2]}) {

        print STDERR "WARNING: discarding \"$_\" for proper test." ;

      } else {

        print $_ ;
      }

    } else {

      # as word is not a baseform, unless explicitly specified

      if(defined $dict{$Fld[2]}) {

        print TEST $_ ;

      } else {

        print $_ ;
      }

    }
}

close INPUT ;
close TEST ;


#
# delete the temporary file
#

END {
  if ($tmp1 && -e $tmp1) {
    unlink ($tmp1);
  }
}
