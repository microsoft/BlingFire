@perl -Sx %0 %*
@goto :eof
#!perl

use utf8;

sub usage {

print <<EOM;

Usage: fa_filter [OPTIONS]

  --mask=<re1> -- substitutes text matched by <re1> with empty string

  --match=<re2> -- if <re2> if found within the line modulo mask <re1>

  --ignore-case -- ignores case

  --invert-match -- select non-matching lines

  --print-matched-only -- prints an input subsstring which matches <re2> after
    masking with <re1>

EOM

}


$re1 = "";
$re2 = "";
$ignore_case = "";
$invert = 0;
$print_matched = 0;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--mask=(.+)/) {

        $re1 = $1;

    } elsif ($ARGV [0] =~ /^--match=(.+)/) {

        $re2 = $1;

    } elsif ($ARGV [0] eq "--invert-match") {

        $invert = 1;

    } elsif ("--ignore-case" eq $ARGV [0]) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] =~ /^--charmap=./) {

        $ignore_case .= (" " . $ARGV [0]);

    } elsif ($ARGV [0] eq "--print-matched-only") {

        $print_matched = 1 ;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_filter --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


$command = "cat | " ;
if("" ne $ignore_case) {
  $command = "fa_line_format $ignore_case | " ;
}
open INPUT, $command ;

while (<INPUT>) {

  s/[\r\n]+//g;
  s/^\xEF\xBB\xBF//;

  $in = $_;

  s/$re1//g;

  if(1 == $invert) {
    if (!/$re2/) {
      print "$in\n";
    }
  } else {
    if (/$re2/) {
      if(0 == $print_matched) {
        print "$in\n";
      } else {
        # $& the matched part of an input after masking
        $custom_re = join("(?:$re1)?", split("", $&));
        if($in =~ /$custom_re/) {
          # $& the matched a part of an input including masked text
          print "$&\n";
        } else {
          die "ERROR: Fatal error, check if your perl supports UTF-8\n" ;
        }
      }
    }
  }
}


close INPUT;
