@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;


sub usage {

print <<EOM;

Usage: fa_num2int [OPTIONS]

This program transforms floats or integers of the given column into integer
values in the given range. The transformation is done by the linear scale
from Min to Max. Columns should be tab separated.

  --field=N - 1-based value of the field to be remapped,
    1 is used by default

  --min=N - minimum output value, 0 is used by default

  --max=N - maximum output value, 255 is used by default

  --log-scale - calculates natural logarithm before rescaling

  --out-minmax=<file-name> - prints out minimum and maximum input or log(input)
    values to the output file, does not print by default

EOM

}

#
# *** Process command line parameters ***
#

$field = 1 ;
$out_min = 0 ;
$out_max = 255 ;
$log_scale = 0 ;
$out_minmax = "";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--min=(.+)/) {

        $out_min = 0 + $1;

    } elsif ($ARGV [0] =~ /^--max=(.+)/) {

        $out_max = 0 + $1;

    } elsif ($ARGV [0] =~ /^--field=(.+)/) {

        $field = 0 + $1;

    } elsif ($ARGV [0] eq "--log-scale") {

        $log_scale = 1;

    } elsif ($ARGV [0] =~ /^--out-minmax=(.+)/) {

        $out_minmax = $1;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_num2int --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


#
#  *** Create temporary files ***
#

($fh, $tmp) = mkstemp ("fa_num2int_XXXXXXXX");
close $fh;

#
#  *** Calculate input maximum and minimum ***
#

$in_min =  1000000000.0;
$in_max = -1000000000.0;

$[ = 1;
$\ = "\n";
$FS = "\t";

open TMP, ">$tmp" ;

while(<STDIN>) {

  s/[\r\n]+//g;

  print TMP $_;

  @Fld = split($FS, $_, 9999);

  $f = $Fld[$field];
  if (1 == $log_scale) {
    $f = log(0.000000001 + $f) ;
  }

  if (0.0 + $f > $in_max) {
    $in_max = 0.0 + $f ;
  }

  if (0.0 + $f < $in_min) {
    $in_min = 0.0 + $f ;
  }
}

close TMP;

# store input min/max values used for coding

if("" ne $out_minmax) {
  open OUTPUT, ">$out_minmax" ;
  print OUTPUT "$in_min\n$in_max";
  close OUTPUT;
}


#
#  *** Remap the data ***
#

$k = 0;

if($in_max > $in_min) {
  $k = (0.0 + $out_max - $out_min) / (0.0 + $in_max - $in_min);
}

$b = (0.0 + $out_min) - ($k * $in_min);


open INPUT, "< $tmp" ;

while(<INPUT>) {

  s/[\r\n]+//g;

  @Fld = split($FS, $_, 9999);

  $f = $Fld[$field];

  # calculate logarithm, if needed
  if (1 == $log_scale) {
    $f = log(0.000000001 + $f) ;
  }

  # see if $b is defined
  if($in_max > $in_min) {
    $v = int($k * (0.0 + $f) + $b + 0.5);
  } else {
    $v = int($out_min);
  }

  # make sure $v is within set limits
  if($v < $out_min) {
    $v = $out_min;
  } elsif ($v > $out_max) {
    $v = $out_max;
  }

  $Fld[$field] = $v ;

  print join("\t", @Fld) ;
}

close INPUT ;

#
#  *** Remove temporary files ***
#

END {
  if ($tmp && -e $tmp) {
    unlink ($tmp);
  }
}
