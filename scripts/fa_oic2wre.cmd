@perl -Sx %0 %*
@goto :eof
#!perl


sub usage {

print <<EOM;

Usage: fa_oic2wre [OPTIONS] < input.utf8 > output.utf8

This program converts OIC rules, into the "shallow parser syntax" WRE rules.
The input and the output should be in UTF-8.

   --out-tagset=<oiutput-file> - saves tagset into the output file,
    does not save tagset by default
EOM
}


$out_tagset = "" ;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--out-tagset=(.+)/) {

        $out_tagset = $1;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_oic2wre --help";
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
$action_ = '';
$error_type_ = '';
$dialect_ = '';

line: while (<STDIN>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    @Fld = split($FS, $_, -1);

    # repeat all empty lines
    if (/^[ \t]*$/) {
	printf "\n";
	next line;
    }

    # repeat comments
    if (/^[\/][\/]/) {
	print '# ' . $_;
	next line;
    }

    # print Ronly rules as comments
    if (/^[^\t]+[\t]Ronly/) {
	print '### was not commented ### ' . $_;
	next line;
    }

    if (1 < $#Fld) {
	$action_ = $Fld[1];
	$dialect_ = '';
	$error_type_ = '';
	$rule_ = '';

	for ($i = 2; $i <= $#Fld; ++$i) {
	    $S = &convert($Fld[$i]);
	    if ('' ne $S) {
		$rule_ = $rule_ . ' ' . $S;
	    }
	}

	$dialect_ = &dialect2wre($dialect_);

	if ('' eq $error_type_) {
	    printf '%s <%s > --> %s', $dialect_, $rule_, $action_;
	    $tagset{$action_} = 1;
	}
	else {
	    printf '%s %s --> %s.%s', $dialect_, $rule_, $action_,

	      $error_type_;
	    $tagset{$action_ . '.' . $error_type_} = 1;
	}

	### uncomment to see original rules
	### printf "\t\t\t%s", $0;

	printf "\n";
    }
}

if("" ne $out_tagset) {
    open OUTPUT, ">$out_tagset" ;
    $i = 1;
    foreach $tag (sort keys %tagset) { 
        printf OUTPUT "%s %i\n", $tag, $i++;
    }
    close OUTPUT;
}

sub convert {
    local($S) = @_;
    if ($S eq '<*>') {
	return '.';
    }
    elsif ($S =~ /^[[].+[]]$/) {
	$error_type_ = $S;
	return '< . >';
    }
    elsif ($S =~ /^R[+\-] D/) {
	$dialect_ = $S;
	return '';
    }
    elsif ($S eq '') {
	return '';
    }
    else {
	return "\"" . $S . "\"";
    }
}

sub dialect2wre {
    local($S) = @_;
    $c = (@a = split(/[ ]/, $S, -1));

    $rval = '';
    $dialects_ = '';

    for ($i = 1; $i <= $c; ++$i) {
	if ($a[$i] eq 'R+') {
	    $rval = 'POST';
	}
	elsif ($a[$i] eq 'R-') {
	    $rval = 'PRE';
	}
	elsif ($a[$i] eq 'Dall') {
	    if ('' ne $dialects_) {
		$dialects_ = $dialects_ . '|';
	    }
	    $dialects_ = $dialects_ . 'DALL';
	}
	elsif ($a[$i] =~ /^D[0-9]+$/) {
	    if ('' ne $dialects_) {
		$dialects_ = $dialects_ . '|';
	    }
	    $dialects_ = $dialects_ . $a[$i];
	}
    }

    if ('' eq $rval) {
	$rval = 'POST';
    }
    if ('' eq $dialects_) {
	$dialects_ = 'DALL';
    }

    # ignore reform

    $dialects_;
}
