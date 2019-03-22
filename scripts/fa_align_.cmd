@perl -Sx %0 %*
@goto :eof
#!perl

use encoding 'utf8';

sub usage {

print <<EOM;

Usage: fa_align_ [OPTIONS]

This tool aligns two input sequences of tokens, where each token can be 
anything in between token delimiters, by default that is all characters.
The input should contains two tab separated sequences per line.

  --delim=<regexp> - is used to specify token delimiter

  --out-delim=<string> - is used to specify output token delimiter,
    empty string is used by default

  --rev - reverses two sequences before aligning them, the output
    is reversed back to the normal order

  --merge - prints results as a single sequence
EOM

}

$delim = "";
$out_delim = "";
$merge = "";
$rev = 0;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--delim=(.+)/) {

        $delim = $1;

    } elsif ($ARGV [0] =~ /^--out-delim=(.+)/) {

        $out_delim = $1;

    } elsif ($ARGV [0] eq "--merge") {

        $merge = $ARGV [0] ;

    } elsif ($ARGV [0] eq "--rev") {

        $rev = 1 ;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_align --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


$[ = 1;			# set array base to 1
$FS = ' ';		# set field separator
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

$FS = "\t";

$gap_score = -1;

line: while (<>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split($FS, $_, 9999);

    if ($#Fld == 2) {
	$i = 0;
	$j = 0;

	$len1 = (@str1 = split($delim, $Fld[1], 9999));
	$len2 = (@str2 = split($delim, $Fld[2], 9999));

	if(1 == $rev) {
	    @str1 = reverse(@str1);
	    @str2 = reverse(@str2);
        }

	$D{1 . ' ' . 1} = 0;

	for ($j = 1; $j <= $len1; ++$j) {
	    $D{1 . ' ' . ($j + 1)} = $j * $gap_score;
	}
	for ($i = 1; $i <= $len2; ++$i) {
	    $D{($i + 1) . ' ' . 1} = $i * $gap_score;
	}

	for ($i = 2; $i <= $len2 + 1; ++$i) {
	    for ($j = 2; $j <= $len1 + 1; ++$j) {
		$s1 = $D{($i - 1) . ' ' . ($j - 1)} + &Smlr($str1[$j - 1], $str2[$i - 1]);
		$s2 = $D{$i . ' ' . ($j - 1)} + $gap_score;
		$s3 = $D{($i - 1) . ' ' . $j} + $gap_score;

		if ($s1 >= $s2 && $s1 >= $s3) {
		    $D{$i . ' ' . $j} = $s1;
		}
		elsif ($s2 >= $s1 && $s2 >= $s3) {
		    $D{$i . ' ' . $j} = $s2;
		}
		else {
		    $D{$i . ' ' . $j} = $s3;
		}
	    }
	}

	###  printf "Score matrix:\n";
	###  for(i = 1; i <= len2 + 1; ++i) {
	###    for(j = 1; j <= len1 + 1; ++j) {
	###      printf "%4i ", D[i " " j];
	###    }
	###    printf "\n";
	###  }
	###  printf "\n";

	$str1_align = '';
	$str2_align = '';

	$i = $len2 + 1;
	$j = $len1 + 1;

	while ($i > 1 && $j > 1) {
	    if ($D{$i . ' ' . $j} - &Smlr($str1[$j - 1],

	      $str2[$i - 1]) == $D{($i - 1) . ' ' . ($j - 1)}) {
		$str1_align = $str1[$j - 1] . $out_delim . $str1_align;
		$str2_align = $str2[$i - 1] . $out_delim . $str2_align;
		$i--;
		$j--;
		next;
	    }
	    if ($D{$i . ' ' . $j} - $gap_score == $D{($i - 1) . ' ' . $j}) {
		$str1_align = '_' . $out_delim . $str1_align;
		$str2_align = $str2[$i - 1] . $out_delim . $str2_align;
		$i--;
		next;
	    }
	    if ($D{$i . ' ' . $j} - $gap_score == $D{$i . ' ' . ($j - 1)}) {
		$str1_align = $str1[$j - 1] . $out_delim . $str1_align;
		$str2_align = '_' . $out_delim . $str2_align;
		$j--;
		next;
	    }

	    print 'FATAL ERROR!!!';
	    $ExitValue = (2); last line;
	}
	while ($j > 1) {
	    $str1_align = $str1[$j - 1] . $out_delim . $str1_align;
	    $str2_align = '_' . $out_delim . $str2_align;
	    $j--;
	}
	while ($i > 1) {
	    $str1_align = '_' . $out_delim . $str1_align;
	    $str2_align = $str2[$i - 1] . $out_delim . $str2_align;
	    $i--;
	}

	if(1 == $rev) {
	    $str1_align = join($out_delim, reverse(split($out_delim, $str1_align, 9999)));
	    $str2_align = join($out_delim, reverse(split($out_delim, $str2_align, 9999)));
        }

        if("" eq $merge) {

          print $str1_align . "\t" . $str2_align;

        } else {

          $len1 = (@str1 = split($out_delim, $str1_align, 9999));
          $len2 = (@str2 = split($out_delim, $str2_align, 9999));

	  if($len1 != $len2) {
	    print "ERROR: in line \"$Fld[1]\t$Fld[2]\" use different output delimiter, the current value is: \"$out_delim\"";
	    $ExitValue = (2); last line;
	  }

          printf "%s%s%s", $str1[1], $out_delim, $str2[1] ;

          for ($i = 2; $i <= $len2; ++$i) {
            printf "%s%s%s%s", $out_delim, $str1[$i], $out_delim, $str2[$i] ;
          }

          printf "\n";

        }
    }
}

exit $ExitValue;

sub Smlr {

    local($a, $b) = @_;

    if ($a eq $b) {
	return 1;
    } else {
	# return -3000;
	return 0;
    }
}
