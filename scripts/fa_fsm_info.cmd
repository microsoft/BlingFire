@perl -Sx %0 %*
@goto :eof
#!perl

sub usage {

print <<EOM;

Usage: fa_fsm_info [OPTIONS] < fsm.txt > info.txt

This program returns various information about an input automaton. It 
automatically identifies whether automaton is of Rabin-Scott or Moore type.

  --st-hist=<filename> - prints state histogram,
    does not print by default

  --iw-hist=<filename> - prints input weight histogram,
    does not print by default

  --ow-hist=<filename> - prints output weight histogram,
    does not print by default

Note:
  1. state histogram are the lines of the following format:
    ...
    <out_power> <in_power> <state> [i] [f]
    ...
  2. input/output weight histogram are the lines of the following format:
    ...
    <freq> <iw/ow>
    ...
EOM

}

# process command line parameters
$st_hists = "" ;
$iw_hists = "" ;
$ow_hists = "" ;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        # dummy
        $opened{""}++;
        exit (0);

    } elsif ($ARGV [0] =~ /^--st-hist=(.+)/) {

        $st_hists = $1;

    } elsif ($ARGV [0] =~ /^--iw-hist=(.+)/) {

        $iw_hists = $1;

    } elsif ($ARGV [0] =~ /^--ow-hist=(.+)/) {

        $ow_hists = $1;

    } else {

        last;
    }
    shift @ARGV;
}


$[ = 1;			# set array base to 1
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

$tr_count = 0;
$max_state = '';
$min_state = '';
$max_initial = '';
$min_initial = '';
$max_final = '';
$min_final = '';
$max_iw = '';
$min_iw = '';
$max_ow = '';
$min_ow = '';
$has_ows = 0;

# collect the information

while (<>) {

    s/[\r\n]+//g;

    @Fld = split(' ', $_, 9999);

    if (3 == $#Fld) {

	$tr_count++;

	### update the information about states

	if ('' ne $st_hists) {

	    $states{0 + $Fld[1]} = 1;
	    $states{0 + $Fld[2]} = 1;

            if (defined $out_power{0 + $Fld[1]}) {
	        $out_power{0 + $Fld[1]} = 1 + $out_power{0 + $Fld[1]};
            } else {
                $out_power{0 + $Fld[1]} = 1;
            }
            if (defined $in_power{0 + $Fld[2]}) {
	        $in_power{0 + $Fld[2]} = 1 + $in_power{0 + $Fld[2]};
            } else {
                $in_power{0 + $Fld[2]} = 1;
            }
	}
	$curr_max = 0 + $Fld[1];
	$curr_min = 0 + $Fld[2];
	if ($curr_max < 0 + $Fld[2]) {
	    $curr_max = 0 + $Fld[2];
	    $curr_min = 0 + $Fld[1];
	}
	if ('' eq $max_state || $max_state < $curr_max) {
	    $max_state = $curr_max;
	}
	if ('' eq $min_state || $min_state > $curr_min) {
	    $min_state = $curr_min;
        }

	### update the information about input weights
	if ('' ne $iw_hists) {
            if (defined $iws{0 + $Fld[3]}) {
	        $iws{0 + $Fld[3]} = 1 + $iws{0 + $Fld[3]} ;
            } else {
                $iws{0 + $Fld[3]} = 1 ;
            }
	}
	if ('' eq $max_iw || $max_iw < 0 + $Fld[3]) {
	    $max_iw = 0 + $Fld[3];
	}
	if ('' eq $min_iw || $min_iw > 0 + $Fld[3]) {
	    $min_iw = 0 + $Fld[3];
	}
    }

    ### update the information about output weights

    if (4 <= $#Fld) {

        if (-1 != 0 + $Fld[4]) {

            if ('' ne $ow_hists) {
                if (defined $ows{0 + $Fld[4]}) {
                    $ows{0 + $Fld[4]} = 1 + $ows{0 + $Fld[4]};
                } else {
                    $ows{0 + $Fld[4]} = 1;
                }
            }
            if ('' eq $max_ow || $max_ow < 0 + $Fld[4]) {
                $max_ow = 0 + $Fld[4];
            }
            if ('' eq $min_ow || $min_ow > 0 + $Fld[4]) {
                $min_ow = 0 + $Fld[4];
            }
            $has_ows = 1;
        }
    }

    if ('initial:' eq $Fld[1]) {

	if ('' ne $st_hists) {
	    $ini_states{0 + $Fld[2]} = 1;
	}
	if ('' eq $max_initial || $max_initial < 0 + $Fld[2]) {
	    $max_initial = 0 + $Fld[2];
	}
	if ('' eq $min_initial || $min_initial > 0 + $Fld[2]) {
	    $min_initial = 0 + $Fld[2];
	}
    }

    if ('final:' eq $Fld[1]) {

	if ('' ne $st_hists) {
	    $fin_states{0 + $Fld[2]} = 1;
	}
	if ('' eq $max_final || $max_final < 0 + $Fld[2]) {
	    $max_final = 0 + $Fld[2];
	}
	if ('' eq $min_final || $min_final > 0 + $Fld[2]) {
	    $min_final = 0 + $Fld[2];
	}
    }
}

#
# print the report
#

print 'Trs Count = ' . $tr_count;
print 'Min State = ' . $min_state;
print 'Max State = ' . $max_state;
print 'Min Initial = ' . $min_initial;
print 'Max Initial = ' . $max_initial;
print 'Min Final = ' . $min_final;
print 'Max Final = ' . $max_final;

if ('' ne $st_hists) {
    for ($state = $min_state; $state <= $max_state; ++$state) {

	if (defined $states{$state}) {

            if (defined $out_power{$state}) {
                $curr_out_power = $out_power{$state};
            } else {
                $curr_out_power = 0;
            }

            if (defined $in_power{$state}) {
                $curr_in_power = $in_power{$state};
            } else {
                $curr_in_power = 0;
            }

	    $info = sprintf('%6i %6i %6i', $curr_out_power, $curr_in_power, $state);

	    if (defined $ini_states{$state}) {
		$info = $info . ' i';
	    }
	    if (defined $fin_states{$state}) {
		$info = $info . ' f';
	    }
	    &Pick('|', ('sort -r > ' . $st_hists . '')) &&
		(print $fh $info);
	}
    }
}

print 'Min Iw = ' . $min_iw;
print 'Max Iw = ' . $max_iw;

if ('' ne $iw_hists) {
    for ($iw = $min_iw; $iw <= $max_iw; ++$iw) {

	if (defined $iws{$iw}) {
	    &Pick('|', ('sort -r > ' . $iw_hists . '')) &&
		(printf $fh "%8i %5i\n", $iws{$iw}, $iw);
	}
    }
}

if (1 == $has_ows) {

    print 'Min Ow = ' . $min_ow;
    print 'Max Ow = ' . $max_ow;

    if ('' ne $ow_hists) {
	for ($ow = $min_ow; $ow <= $max_ow; ++$ow) {

	    if (defined $ows{$ow}) {
		&Pick('|', ('sort -r > ' . $ow_hists . '')) &&
		    (printf $fh "%8i %5i\n", $ows{$ow}, $ow);
	    }
	}
    }
}

sub Pick {
    local($mode,$name) = @_;
    $fh = $name;
    open($name,$mode.$name) unless $opened{$name}++;
}
