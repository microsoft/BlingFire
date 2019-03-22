@perl -Sx %0 %*
@goto :eof
#!perl

sub usage {

print <<EOM;

Usage: fa_segs2comps [OPTIONS] < segments.dict.utf8 > compounds.dict.utf8

Reads dictionary of segments from stdin and creates compounds using words
from the limited contexct of the dictionary, one per input segment. Output
is printed to stdout.

  --max-seg-count=N - each compound is created from 2 upto N segments
    randomly selected from the dictionary context, 3 is used by default

  --rand-seed=N - uses N as a seed for random number generator, 
    use this option if you need the same output for the same intput

Input:
  ...
  Word1\\n
  Word2\\n
  ...

Output:
  ...
  Word1Word2...WordN\\tWord1\\tWord2\\t...\\tWordN\\n
  Word1Word2...WordM\\tWord1\\tWord2\\t...\\tWordM\\n
  ...
EOM

}

#
# process command line parameters
#
$N = 3;
$seed = -1;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--max-seg-count=(.+)/) {

        $N = $1;

    } elsif ($ARGV [0] =~ /^--rand-seed=(.+)/) {

        $seed = $1;

    } else {

        last;
    }
    shift @ARGV;
}


$[ = 1;

if($seed == -1) {
  srand();
} else {
  srand($seed);
}

$max_dict_size = 10000;
$dict_size = 0;


while (<>) {

    chomp;

    @Fld = split(' ', $_, 9999);

    if (0 < $#Fld) {

	if ($dict_size < $max_dict_size) {

	    $dict{$dict_size} = $_;
	    $dict_size++;

	} else {

	    $i = int(rand(1) * $max_dict_size);
	    $dict{$i} = $_;

	    $count = 2 + int(rand(1) * ($N - 1));
	    $comp = '';

	    for ($j = 0; $j < $count; ++$j) {

		$i = int(rand(1) * $max_dict_size);
		$arr{$j} = $i;
		$comp = $comp . '' . $dict{$i};
	    }

	    printf '%s', $comp;

	    for ($j = 0; $j < $count; ++$j) {
		printf "\t%s", $dict{$arr{$j}};
	    }

	    printf "\n";
	}
    }
}
