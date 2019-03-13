@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_wtbt2wbs [OPTIONS] < wtbt.in.txt > wbs.out.txt

Converts WTBT dictionary into WBS dictionary, where BS is a set of base-forms.

  --ignore-case - lowercases the intput

  --charmap=<mmap-dump> - applies a custom character normalization procedure
    according to the <mmap-dump>, the dump should be in "fixed" format

  --no-bases - does not use Base as an input word

Input:
  ...
  Word1\\tTag1\\tBase1\\tTag2\\n
  Word2\\tTag3\\tBase1\\tTag2\\n
  Word3\\tTag1\\tBase2\\tTag2\\n
  Word1\\tTag3\\tBase2\\tTag2\\n
  ...

Output:
  ...
  Base1\\tBase1\\n
  Word1\\tBase1\\tBase2\\n
  Word2\\tBase1\\n
  Base2\\tBase2\\n
  Word3\\tBase2\\n
  ...
EOM

}

$ignore_case = "";
$no_bases = "";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ("--no-bases" eq $ARGV [0]) {

        $no_bases = $ARGV [0];

    } else {

      last;
    }
    shift @ARGV;
}


$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_wtbt2bws" };

#
# WTBT -> WB
#

$wtbt2wb = <<'EOF';

$[ = 1;			# set array base to 1
$\ = "\n";		# set output record separator
$FS = "\t";

$no_bases = "";
$prev_base = "";

if("--no-bases" eq $ARGV [1]) {
  $no_bases = $ARGV [1] ;
  shift @ARGV;
}

while (<STDIN>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split($FS, $_, 9999);

    if ("" eq $no_bases && $prev_base ne $Fld[3]) {

        if("" ne $prev_base) {
            print $prev_base . "\t" . $prev_base;
        }

        $prev_base = $Fld[3];
    }

    if (4 == $#Fld) {
        print $Fld[1] . "\t" . $Fld[3] ;
    }
}

if ("" eq $no_bases && "" ne $prev_base) {
    print $prev_base . "\t" . $prev_base;
}

EOF

($fh, $tmp1) = mkstemp ("fa_wtbt2wbs_XXXXXXXX");
print $fh $wtbt2wb;
close $fh;


#
# WB -> WBS
#

$wb2wbs = <<'EOF';

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

($fh, $tmp2) = mkstemp ("fa_wtbt2wbs_XXXXXXXX");
print $fh $wb2wbs;
close $fh;


#
# build the processing pipe and print out its output
#
$ENV{"LC_ALL"} = "C";
$command = "perl $tmp1 $no_bases | fa_sortbytes -m | uniq | perl $tmp2 |";

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
