@perl -Sx %0 %*
@goto :eof
#!perl

sub usage {

print <<EOM;

Usage: fa_fflc2wtbt [OPTIONS] < ffl.txt > wtbt.utf8
EOM

}

$[ = 1;			# set array base to 1
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

if("--help" eq $ARGV [1]) {
  usage ();
  exit (0);
}


open OUTPUT, "| sort | uniq " ;

while (<STDIN>) {

  s/[\r\n]+//g;

  @Fld = split(' ', $_, 9999);

  if (1 < $#Fld) {

    $word = $Fld[1];
    $printed = 0;

    for ($i = 2; $i <= $#Fld; ++$i) {

      if ($Fld[$i] =~ /Infl[(]Noun[)]=/) {

        print OUTPUT substr($Fld[$i], 12, 999999) . "\tNoun(ZZZ)\t" . $word . "\tNoun(ZZZ)";
        $printed = 1;

      } elsif ($Fld[$i] =~ /Base[(]Noun[)]=/) {

        print OUTPUT $word . "\tNoun(ZZZ)\t" . substr($Fld[$i], 12, 999999) . "\tNoun(ZZZ)";
        $printed = 1;
      }
    }

    if (1 != $printed) {
      print OUTPUT $word . "\tNoun(ZZZ)\t" . $word . "\tNoun(ZZZ)";
    }
  }
}

close OUTPUT;
