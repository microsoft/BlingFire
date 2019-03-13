@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_wts2wts [OPTIONS] < wts.in.txt > wts.out.txt

Converts WTS dictionary into WTS dictionary without duplicates, where TS 
is a set of symbolic tags.

  --ignore-case - lowercases the intput

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format

Input:
  ...
  Word\\tTag1\\t\\tTag2\\n
  Word\\tTag2\\t\\tTag3\\n
  ...

Output:
  ...
  Word\\tTag1\\tTag2\\tTag3\\n
  ...
EOM

}


$ignore_case = "";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $ignore_case .= (" " . $ARGV [0]);

    } else {

      last;
    }
    shift @ARGV;
}


$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_wts2wts" };

#
# WTS -> WT
#

$wts2wt = <<'EOF';

$[ = 1;			# set array base to 1
$\ = "\n";		# set output record separator
$FS = "\t";

while (<STDIN>) {

    s/[\r\n]+$//;

    @Fld = split($FS, $_, 9999);

    for ($i = 2; $i <= $#Fld; ++$i) {
        print $Fld[1] . "\t" . $Fld[$i] ;
    }
}

EOF

($fh, $tmp1) = mkstemp ("fa_wts2wts_XXXXXXXX");
print $fh $wts2wt;
close $fh;


#
# WT -> WTS
#

$wt2wts = <<'EOF';

$[ = 1;			# set array base to 1
$\ = "\n";		# set output record separator
$FS = "\t";

$prev_word = "";
$prev_tags = "";

while (<STDIN>) {

    chomp;

    @Fld = split($FS, $_, 9999);

    if (2 == $#Fld) {

        $word = $Fld[1];
        $tag = $Fld[2];

	if ($prev_word ne $word) {

            if("" ne $prev_tags) {
	        print $prev_word . "\t" . $prev_tags;
            }

	    $prev_word = $word;
	    $prev_tags = $tag;

	} else {

            $prev_tags .= ("\t" . $tag) ;
        }
    }
}

if("" ne $prev_tags) {
    print $prev_word . "\t" . $prev_tags;
}

EOF

($fh, $tmp2) = mkstemp ("fa_wts2wts_XXXXXXXX");
print $fh $wt2wts;
close $fh;


#
# build the processing pipe and print out its output
#
$ENV{"LC_ALL"} = "C";
$command = "perl $tmp1 | fa_sortbytes -m | uniq | perl $tmp2 |";

if("" ne $ignore_case) {
    $command = "fa_line_format $ignore_case --wts | " . $command ;
}

open INPUT, $command ;

while(<INPUT>) {
    print $_ ;
}
close INPUT ;


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
}

