@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_wtbt2wts [OPTIONS] < wtbt.in.txt > wts.out.txt

Converts WTBT dictionary into WTS dictionary, where TS is a set of tags.

  --ignore-case - lowercases the intput

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format

  --get-bases   - bases and word-forms' tags 

Input:
  ...
  Word\\tTag1\\tBase\\tTag2\\n
  Word\\tTag3\\tBase\\tTag2\\n
  ...

Output:
  ...
  Word\\tTag1\\tTag3\\n
  Base\\tTag2\\n
  ...
EOM

}


$ignore_case = "";
$get_bases = "";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ("--get-bases" eq $ARGV [0]) {

        $get_bases = $ARGV [0];

    } else {

      last;
    }
    shift @ARGV;
}


$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_wtbt2wts" };

#
# WTBT -> WT
#

$wtbt2wt = <<'EOF';

$[ = 1;			# set array base to 1
$\ = "\n";		# set output record separator
$FS = "\t";

$prev_base = "";
$get_bases = $ARGV [1];

while (<STDIN>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split($FS, $_, 9999);

    if (4 == $#Fld) {

        $base = $Fld[3] . "\t" . $Fld[4] ;

        if ($prev_base ne $base) {

            if("" ne $prev_base) {
                print $prev_base;
            }

	    $prev_base = $base;
        }

        if("" eq $get_bases) {
          print $Fld[1] . "\t" . $Fld[2] ;
        } else {
          print $Fld[3] . "\t" . $Fld[2] ;
        }
    }
}

if("" ne $prev_base) {
    print $prev_base;
}

EOF

($fh, $tmp1) = mkstemp ("fa_wtbt2wts_XXXXXXXX");
print $fh $wtbt2wt;
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

($fh, $tmp2) = mkstemp ("fa_wtbt2wts_XXXXXXXX");
print $fh $wt2wts;
close $fh;


#
# build the processing pipe and print out its output
#
$ENV{"LC_ALL"} = "C";
$command = "perl $tmp1 $get_bases | fa_sortbytes -m | uniq | perl $tmp2 |";

if("" ne $ignore_case) {
    $command = "fa_line_format $ignore_case --wtbt | " . $command ;
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
