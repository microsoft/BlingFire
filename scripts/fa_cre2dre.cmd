@perl -Sx %0 %*
@goto :eof
#!perl

use Getopt::Long;
use IO::File;
use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_cre2dre [OPTIONS] < character-regexp.txt > digit-regexp.txt

Converts character-based regular expression into digit-based 
regular expression. The output encoding is UCS2. Assumes there is no comments
in regexps.

  --input-enc=<enc> - input encoding, "UTF-8" - is used by default

EOM

}

$show_usage = 0;
$enc = "UTF-8";

&GetOptions ("help" => \$show_usage, 
             "input-enc:s" => \$enc);

if (1 == $show_usage) {
  usage ();
  exit (0);
}

if (!$enc) {
  $enc = "UTF-8";
}


#
# !!! This entire algorithm must be improved in future !!!
#


($fh, $tmp) = mkstemp ("fa_cre2dre_XXXXX");
close $fh;


$[ = 1;			# set array base to 1
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

$d2c{'00040'} = '(';
$d2c{'00041'} = ')';
$d2c{'00124'} = '|';
$d2c{'00063'} = '?';
$d2c{'00042'} = '*';
$d2c{'00043'} = '+';
$d2c{'00046'} = '.';


$cmdline = "| fa_line2chain_unicode --base=10 --input-enc=$enc > $tmp";
open OUTPUT, $cmdline ;

if (OUTPUT) {

    while(<>) {
        print OUTPUT $_ ;
    }
    close OUTPUT;

    open(DIGITS, $tmp) || die ("ERROR: Can't open file $tmp .");

    while (<DIGITS>) {

        s/[\r\n]+//g;

        @Fld = split(' ', $_, 9999);

        for ($i = 1; $i <= $#Fld; ++$i) {
	    if (defined $d2c{$Fld[$i]}) {
	        $Fld[$i] = $d2c{$Fld[$i]};
	    }
        }
        print join($,,@Fld);
    }

    close DIGITS;

} else {

    print STDERR "ERROR: Could not create \"$cmdline\" pipe.";
    exit (1);
}

unlink($tmp);
