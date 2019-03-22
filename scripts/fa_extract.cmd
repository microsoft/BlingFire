@perl -Sx %0 %*
@goto :eof
#!perl


sub usage {

print <<EOM;

Extracts constituents (with sub constituents) which name match input 
regular expression.

Usage: fa_extract [OPTIONS] [FILES]

  --in=<input-file> - input file, if not specified reads from the stdin

  --out=<output-file> - output file, if not specified writes to stdout

  --name=<regexp> - specifies the name of extracting constituents,
    '.*' is used by default

  --no-subtrees - prints constituents without sub constituents,
    just a flat tagged text

  --context=N - takes N words from the left and N from the right as a context
    for extracted constituent, 0 - is used by default

EOM

}

#
# process command line parameters
#

$regexp = '.*[(][(]' ;
$nosubs = 0 ;
$context = 0 ;
$input = "";
$output = "";


while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

      usage ();
      exit (0);

    } elsif ($ARGV [0] =~ /^--name=(.+)/) {

      $regexp = $1 . '[(][(]' ;

    } elsif ($ARGV [0] =~ /^--context=(.+)/) {

      $context = 0 + $1 ;

    } elsif ("--no-subtrees" eq $ARGV [0]) {

      $nosubs = 1 ;

    } elsif ($ARGV [0] =~ /--in=(.+)/) {

      $input = $1 ;

    } elsif ($ARGV [0] =~ /--out=(.+)/) {

      $output = $1 ;

    } elsif ($ARGV [0] =~ /^--/) {

      next;

    } else {

      last;
    }

    shift @ARGV;
}

# set array base to 1
$[ = 1;


$command = "cat $input | " ;
open INPUT, $command ;

if("" ne $output) {
  open OUTPUT, ">$output" ;
}


while (<INPUT>) {

  if (!/$regexp/) {
    next;
  }

  s/[\r\n]+//g;

  @Fld = split(' ', $_, 999999);

  $in = 0 ;
  $from = -1 ;
  $to = -1 ;

  for ($i = 1; $i <= $#Fld; ++$i) {

    $s = $Fld[$i] ;

    if (0 == $in && $s =~ /$regexp/) {

      $in = 1 ;
      $from = $i;

    } elsif (0 < $in && $s =~ /.*[(][(]/) {

      $in++ ;

    } elsif (1 < $in && $s eq "))") {

      $in-- ;

    } elsif (1 == $in && $s eq "))") {

      $in = 0 ;
      $to = $i;

      $out = "" ;
      $left = "" ; 
      $right = "" ;

      # get left context

      $j = $from;
      $c = 0;

      while($c != $context) {

        $j--;

        if($j <= 0) {
          $s = '^' ;
        } else {
          $s = $Fld[$j] ;
        }

        if($s =~ /^((.*[(][(])|([)][)]))$/) {
          if(0 == $nosubs) {
            $left = $s . " " . $left;
            $c++;
          }
        } else {
          $left = $s . " " . $left;
          $c++;
        }
      }

      # get right context

      $j = $to;
      $c = 0;

      while($c != $context) {

        $j++;

        if($j > $#Fld) {
          $s = '$' ;
        } else {
          $s = $Fld[$j] ;
        }

        if($s =~ /^((.*[(][(])|([)][)]))$/) {
          if(0 == $nosubs) {
            $right = $right . $s . " " ;
            $c++;
          }
        } else {
          $right = $right . $s . " " ;
          $c++;
        }
      }

      # get constituent

      for($j = $from; $j <= $to; ++$j) {

        if($j <= 0) {
          $s = '^' ;
        } elsif ($j > $#Fld) {
          $s = '$' ;
        } else {
          $s = $Fld[$j] ;
        }

        if($s =~ /^((.*[(][(])|([)][)]))$/) {
          if(0 == $nosubs) {
            $out = $out . $s . " " ;
          }
        } else {
          $out = $out . $s . " " ;
        }
      }

      if("" ne $output) {
        if(0 == $context) {
          print OUTPUT "$out\n" ;
        } else {
          print OUTPUT "$left\t$out\t$right\n" ;
        }
      } else {
        if(0 == $context) {
          print "$out\n" ;
        } else {
          print "$left\t$out\t$right\n" ;
        }
      }

    }
  }
}

close INPUT ;

if("" ne $output) {
  close OUTPUT ;
}
