@perl -Sx %0 %*
@goto :eof
#!perl


sub usage {

print <<EOM;

Usage: fa_w2tp_summ [OPTIONS]

Summs up multiple outputs of the W2TP function.

  --best=N - returns only top N results, returns all by default

EOM

}


$topn = -1;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--best=(.+)/) {

        $topn = 0 + $1;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_w2tp_summ --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}


#
# Summs scores together
#
# Input: pt ru  [ 0 -80 ]
#

$[ = 1;			# set array base to 1

$total_count = 0;

while(<>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    ### # for debugging
    ### print STDERR $_ . "\n"; 
 
    @Fld = split("\t", $_, 9999);

    if ($#Fld == 2) {

	$len1 = (@langs = split(" ", $Fld[1], 9999));
	$len2 = (@probs = split(" ", $Fld[2], 9999));

        if($len1 + 2 != $len2) {
          print STDERR "ERROR: number of classes does not match the number of log probs\n";
        }

        $total_count++;

        for($i = 1; $i <= $len1; ++$i) {
          $lang = $langs[$i];
          $scores{$lang} = 0.0 + $probs[$i + 1] + $scores{$lang};
          $counts{$lang} = 1 + $counts{$lang};
        }
    }
}

if(0 != $total_count) {

  # add pentalties for not-found classes
  foreach $l (keys %scores) {
    $scores{$l} = $scores{$l} + (-160.0 * ($total_count - $counts{$l}));
  }

  $res_count = 0;

  # print the results
  foreach $l (sort {$scores{$b} <=> $scores{$a}} (keys %scores)) {

    $count = $counts{$l};
    $score = $scores{$l};

    print "$l\t$score\t$count\n";

    $res_count++;

    if($res_count == $topn) {
      goto out;
    }
  }

} else {

  print "NONE" ;
}

out:
