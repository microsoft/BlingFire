@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;

sub usage {

print <<EOM;

Usage: fa_suffle < in.utf8 > out.utf8

Makes random permutations of the lines of the input file/stream.

EOM

}


while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } else {

      last;
    }
    shift @ARGV;
}


$SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_shuffle" };



#
# Input: line\n
# Output: R\tline\n
#

$add_num = <<'EOF';

$MAX_RAND = 999999 ;

srand();

while (<STDIN>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    print sprintf("%06i", int(rand($MAX_RAND))) . "\t" . $_ . "\n";
}

EOF

($fh, $tmp1) = mkstemp ("fa_shuffle_XXXXXXXX");
print $fh $add_num;
close $fh;


#
# Input:  R\tline\n
# Output: line\n
#

$remove_num = <<'EOF';

while (<STDIN>) {

    s/[\r\n]+$//;

    print substr($_, 1 + index($_, "\t")) . "\n" ;
}

EOF

($fh, $tmp2) = mkstemp ("fa_shuffle_XXXXXXXX");
print $fh $remove_num;
close $fh;


#
# make the shuffle
#

$command = "perl $tmp1 | sort | perl $tmp2 |" ;

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
