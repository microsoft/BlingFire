@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_wtbt2bws [OPTIONS] < wtbt.in.utf8 > bws.out.utf8

Converts WTBT dictionary into BWS dictionary, where WS is a set of word-forms.

  --ignore-case - lowercases the intput

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format

Input:
  ...
  Word1\\tTag1\\tBase\\tTag2\\n
  Word2\\tTag3\\tBase\\tTag2\\n
  ...

Output:
  ...
  Base\\tWord1\\tWord2\\n
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


$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_wtbt2bws" };

#
# WTBT -> BW
#

$wtbt2bw = <<'EOF';

$[ = 1;			# set array base to 1
$\ = "\n";		# set output record separator
$FS = "\t";

$prev_base = "";

while (<STDIN>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split($FS, $_, 9999);

    if (4 == $#Fld) {
        print $Fld[3] . "\t" . $Fld[1] ;
    }
}

EOF

($fh, $tmp1) = mkstemp ("fa_wtbt2bws_XXXXXXXX");
print $fh $wtbt2bw;
close $fh;


#
# BW -> BWS
#

$bw2bws = <<'EOF';

$[ = 1;			# set array base to 1
$\ = "\n";		# set output record separator
$FS = "\t";

$prev_word = "";
$prev_info = "";

while (<STDIN>) {

    chomp;

    @Fld = split($FS, $_, 9999);

    if (2 == $#Fld) {

        $word = $Fld[1];
        $info = $Fld[2];

	if ($prev_word ne $word) {

            if("" ne $$prev_info) {
	        print $prev_word . "\t" . $$prev_info;
            }

	    $prev_word = $word;
	    $$prev_info = $info;

	} else {

            $$prev_info .= ("\t" . $info) ;
        }
    }
}

if("" ne $$prev_info) {
    print $prev_word . "\t" . $$prev_info;
}

EOF

($fh, $tmp2) = mkstemp ("fa_wtbt2bws_XXXXXXXX");
print $fh $bw2bws;
close $fh;


#
# build the processing pipe and print out its output
#
$ENV{"LC_ALL"} = "C";
$command = "perl $tmp1 | fa_sortbytes -m | uniq | perl $tmp2 |";

if("" ne $ignore_case) {
    $command = "fa_line_format $ignore_case | " . $command ;
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
