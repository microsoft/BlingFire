@perl -Sx %0 %*
@goto :eof
#!perl

use File::Temp qw/ :mktemp  /;


sub usage {

print <<EOM;

Usage: fa_preproc [OPTIONS] < input.txt > output.txt

This program makes preprocessing for the input rules.

  --dict-root=<path> - specifies dictionary path, if not specified the
    dictionary path is taken from the \$DICTS_ROOT environment variable

  --cpp-out=<filename> - should be specified if there is a code associated
    with each WRE rule, specifies the file name where the right parts' code
    will be stored

  --act-out=<filename> - should be specified if there are single line
    strings associated with each rule delimited with ' --> '


 Performs the following actions with the input stream:

  1. Makes in-place inclusions, syntax:
     _include <filename>, where the <filename> is relative to the dictionary 
     path, file name is specified without quotations

  2. Removes comment and emptylines, syntax:
     /^[ \\t]*#.*\$/ || /^[ \\t]*\$/

  3. Expands _define macro, syntax:
     _define name <anything>\\n
     _define name\\n
       <anything multi line>\\n
     _end\\n

 If --cpp-out=X specified:

  For processing of WRE rules with right parts as C++ functions

  4. Removes right parts of the rules and changes them into empty lines
     in the main stream, syntax:
     {\\n
     <anything>\\n
     }\\n

  5. Uses %ARGS = <params> derictive for function parameters specification,
     for example: %ARGS = void * pData, const int RuleId, const int To

  6. Lines which begin from '%' and between '%{' and '%}' are stored into 
     the output code file as is but without the leading '%', '%{' and '%}'

 If --act-out=X specified:

  For processing of WRE rules with single line actions

  4. Substitutes / --> (.*)\$/ constructions with empty lines, prints out 
     the parallel array of right part strings into X, the space symbol before
     and after --> are delimiters

 Notes:

  1. Defines are not recursive.
  2. All define names must be in ASCII and contain only letters and underscore.
EOM

}


$cpp_out = "";
$act_out = "";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--dict-root=(.+)/) {

        $ENV{'DICTS_ROOT'} = $1;

    } elsif ($ARGV [0] =~ /^--cpp-out=(.+)/) {

        $cpp_out = $1;

    } elsif ($ARGV [0] =~ /^--act-out=(.+)/) {

        $act_out = $1;

    } else {

        last;
    }
    shift @ARGV;
}


#
# makes inclusions
#

$s = <<'EOF';

$[ = 1;			# set array base to 1
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

if ('' eq $ENV{'DICTS_ROOT'}) {
    print STDERR "\nERROR: DICTS_ROOT environment variable is not defined\n";
    exit(1);
}

while (<>) {

    s/[\r\n]+//g;

    @Fld = split(' ', $_, 9999);

    if ('_include' eq $Fld[1]) {

	$filename = $ENV{'DICTS_ROOT'} . '/' . $Fld[2];

        open INPUT, "< $filename" ;

        while (<INPUT>) {
          s/^\xEF\xBB\xBF// ;
          s/[\r\n]+//g ;
          print ;
        }

        close INPUT ;

	next;
    }
    print $_;
}

EOF

($fh, $include) = mkstemp ("fa_preproc_XXXXXXXX");
print $fh $s;
close $fh;


#
# removes comments
#

$s = <<'EOF';

$[ = 1;			# set array base to 1
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

line: while (<>) {

    s/[\r\n]+//g;
    @Fld = split(' ', $_, 9999);

    # skip right parts as-is
    if ('{' eq $Fld[1] && 1 == $#Fld) {
	while ('}' ne $_) {
	    print $_;
	    $_ = &Getline();
	}
	print $_;
	next line;
    }

    # skip additional code as is
    if ('%{' eq $Fld[1] && 1 == $#Fld) {
	while ('%}' ne $_) {
	    print $_;
	    $_ = &Getline();
	}
	print $_;
	next line;
    }

    # remove comments
    if (/^[ \t]*[#]/) {
	next line;
    }

    # remove empty lines
    if (/^[ \t]*$/) {
	next line;
    }

    print $_;
}

sub Getline {
    if (($_ = <>) ne '') {
        s/[\r\n]+//g;
	@Fld = split(' ', $_, 9999);
    }
    $_;
}

EOF

($fh, $comment) = mkstemp ("fa_preproc_XXXXXXXX");
print $fh $s;
close $fh;


#
# expands defines
#

$s = <<'EOF';

$[ = 1;			# set array base to 1
$FS = ' ';		# set field separator
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

line: while (<>) {

    s/[\r\n]+//g;
    @Fld = split($FS, $_, 9999);

    # single-line define
    if ('_define' eq $Fld[1] && 2 < $#Fld) {

	$val = '';
	for ($i = 3; $i < $#Fld; ++$i) {
	    $val = $val . '' . $Fld[$i] . ' ';
	}
	$val = $val . '' . $Fld[$#Fld];

	&CheckName($Fld[2]);
	$defines{$Fld[2]} = $val;

	next line;
    }
    # multi-lines define
    if ('_define' eq $Fld[1] && 2 == $#Fld) {

	$key = $Fld[2];
        $val = '';
	$_ = &Getline();

	while ('_end' ne $_) {
	    $val = $val . '' . $_ . "\n";
	    $_ = &Getline();
	}
        chomp($val);

	&CheckName($key);
	$defines{$key} = $val;

	next line;
    }

    # skip right parts as-is
    if ('{' eq $Fld[1] && 1 == $#Fld) {
	while ('}' ne $_) {
	    print $_;
	    $_ = &Getline();
	}
	print $_;
	next line;
    }
    # skip additional code as is
    if ('%{' eq $Fld[1] && 1 == $#Fld) {
	while ('%}' ne $_) {
	    print $_;
	    $_ = &Getline();
	}
	print $_;
	next line;
    }
    # skip %lines as is
    if ('%' eq substr($Fld[1], 1, 1)) {
	print $_;
	next line;
    }

    # make substitutions

    # remember old FS/OFS values here
    $old_fs = $FS;
    $old_ofs = $,;

    # make resplitting
    $FS = '';
    $, = '';
    $prev_res = $_;
    $res = "";
    $SC = 0;

    while(true) {

        @Fld = split($FS, $prev_res, 99999);

        $name_begin = -1;

        for ($i = 1; $i <= $#Fld; ++$i) {

            $c = '' . $Fld[$i];

            if (1 == &IsNameSymbol($c)) {

	        if (-1 == $name_begin) {
                    $name_begin = $i;
                }

            } else {

                if (-1 != $name_begin) {

                    $name = substr($prev_res, $name_begin, $i - $name_begin);

                    if (defined $defines{$name}) {
                        $name = $defines{$name};
                    }

                    $res = $res . sprintf('%s', $name);
                }

                $name_begin = -1;
                $res = $res . sprintf('%s', $c);
            }
        }

        if (-1 != $name_begin) {

            $name = substr($prev_res, $name_begin, 99999);

            if (defined $defines{$name}) {
                $name = $defines{$name};
            }
            $res = $res . sprintf('%s', $name);
        }

        if(100 <= ++$SC) {
            printf STDERR "ERROR: Recursive or too deep define found. The resulting string is:\n\n\"%s\"\n\n", $prev_res;
            exit(1);
        } elsif($prev_res eq $res) {
            last;
        } else {
            $prev_res = $res;
            $res = "";
        }
    }

    printf "%s\n", $res;

    # restore old FS/OFS values here
    $FS = $old_fs;
    $, = $old_ofs;
}

sub IsNameSymbol {
    local($c) = @_;
    if ($c =~ /^[A-Za-z_0-9]$/) {
	return 1;
    } else {
	return 0;
    }
}

sub CheckName {
    local($name) = @_;
    if ($name !~ /^[A-Za-z_][A-Za-z_0-9]*$/) {
	printf STDERR "ERROR: Bad define name \"%s\",\nsee fa_preproc --help for the details.\n", $name;
	exit(1);
    }
}

sub Getline {
    if ($getline_ok = (($_ = <>) ne '')) {
        s/[\r\n]+//g;
	@Fld = split($FS, $_, 9999);
    }
    $_;
}

EOF

($fh, $define) = mkstemp ("fa_preproc_XXXXXXXX");
print $fh $s;
close $fh;


#
# makes cpp output and delimits rules one from another with empty line
#

$s = <<'EOF';

$[ = 1;			# set array base to 1
$, = ' ';		# set output field separator
$\ = "\n";		# set output record separator

$cpp_args = 'void * pData, const int RuleId, const int From, const int To';
$cpp_fcount = 0;
$cpp_out = shift(@ARGV);
$cpp_class = "";
$cpp_base_class = "";
$cpp_object = "";
$fn_name_pref = "";

open CPPOUT, "> $cpp_out" ;

line: while (<>) {

    s/[\r\n]+//g;
    @Fld = split(' ', $_, 9999);

    # get C++ function arguments from the input file
    # %ARGS = void * pData, const int RuleId, const int To
    if ($Fld[1] eq '%ARGS' && $Fld[2] eq '=') {

	$Fld[1] = '';
	$Fld[2] = '';
	$cpp_args = join($,,@Fld);

	next line;
    }

    # %OBJECT = Name
    if ($Fld[1] eq '%OBJECT' && $Fld[2] eq '=') {
	$cpp_object = $Fld[3];
	next line;
    }

    # %BASE = Name
    if ($Fld[1] eq '%BASE' && $Fld[2] eq '=') {
	$cpp_base_class = $Fld[3];
	next line;
    }

    # %CLASS = Name
    if ($Fld[1] eq '%CLASS' && $Fld[2] eq '=') {

	$cpp_class = $Fld[3];
        $fn_name_pref = $cpp_class . "::";

        print CPPOUT "class " . $cpp_class ;
        if("" ne $cpp_base_class) {
            print CPPOUT " : public " . $cpp_base_class ;
        }
        print CPPOUT " { " ;

        print CPPOUT <<PRINT_END;

protected:

    void * GetAct (const int ActNum) const
    {
        if (0 <= ActNum && (sizeof (Rule2Fn) / sizeof (Rule2Fn [0])) > ActNum) {
            return Rule2Fn [ActNum] ;
        } else {
            return NULL;
        }
    }

private:

PRINT_END

	next line;
    }

    # skip additional code as is
    if ('%{' eq $Fld[1] && 1 == $#Fld) {

	while ('%}' ne $_) {

	    $_ = &Getline();

	    if ('%}' ne $_) {
                print CPPOUT join($,,@Fld) ;
	    }
	}
	next line;
    }

    # print the line into the cpp out as is, but without %
    if ('%' eq substr($Fld[1], 1, 1)) {
	print CPPOUT substr($_, 2, 999999) ;
	next line ;
    }

    # print the right part of the rule
    if ('{' eq $Fld[1] && 1 == $#Fld) {

	# print next C++ function
        print CPPOUT 'static void ' . 'fn' . $cpp_fcount . ' (' . $cpp_args . ')' ;

	while ($_ !~ /^[}][ \t]*$/) {
            print CPPOUT join($,,@Fld) ;
	    $_ = &Getline();
	}
        print CPPOUT "}\n" ;

	$cpp_fcount++;
	print '';
	next line;
    }

    print join($,,@Fld);
}

# finish up class, if any
if("" ne $cpp_class) {
    print CPPOUT "private:" ;
    print CPPOUT "    static void * Rule2Fn [$cpp_fcount] ;" ;
    print CPPOUT "};" ;
}

# print RuleNum -> FnPtr map
print CPPOUT "\n\n" ;
print CPPOUT "void * " . $fn_name_pref . "Rule2Fn [$cpp_fcount] = {" ;
for($i = 0 ; $i < $cpp_fcount - 1; $i++) {
  print CPPOUT '    ' . $fn_name_pref . 'fn' . $i . ',' ;
}
print CPPOUT '    ' . $fn_name_pref . 'fn' . ($cpp_fcount - 1);
print CPPOUT '};';


# print global object if needed
if("" ne $cpp_object) {
  print CPPOUT <<PRINT_END;


$cpp_class g_$cpp_object ;
PRINT_END

  if("" ne $cpp_base_class) {
    print CPPOUT "const FAActionsA * g_p$cpp_object = & g_$cpp_object ;" ;
  }
}

# helper
sub Getline {
    if (($_ = <>) ne '') {
        s/[\r\n]+//g;
	@Fld = split(' ', $_, 9999);
    }
    $_;
}

close CPPOUT ;

EOF

($fh, $cpp) = mkstemp ("fa_preproc_XXXXXXXX");
print $fh $s;
close $fh;



#
# makes parallel array of " --> " actions and 
# makes output rules empty line delimited
#

$s = <<'EOF';

$actions = shift(@ARGV);

open ACTIONS, "> $actions" ;

while (<>) {

    s/[\r\n]+//g;

    if ($_ =~ /^(.*) --> (.*)$/) {

        print "$1\n\n" ;
        print ACTIONS "$2\n" ;

    } else {

        print "$_\n" ;
    }
}

close ACTIONS ;

EOF

($fh, $actions) = mkstemp ("fa_preproc_XXXXXXXX");
print $fh $s;
close $fh;


#
# Build the command line
#

$cmdline = "| perl $include | perl $comment | perl $define " ;

if($cpp_out ne "") {
    $cmdline = $cmdline . "| perl $cpp $cpp_out " ;
}

if($act_out ne "") {
    $cmdline = $cmdline . "| perl $actions $act_out " ;
}


#
# Make processing
#

open OUTPUT, $cmdline ;
local $SIG{PIPE} = sub { die "ERROR: Broken pipe at fa_preproc" };

if (OUTPUT) {

    while(<>) {
        print OUTPUT $_ ;
    }
    close OUTPUT;

} else {

    print STDERR "ERROR: Internal error : fa_preproc";
    exit (1);
}


#
# delete temporary files
#

END {
    if ($include && -e $include) {
        unlink ($include);
    }
    if ($comment && -e $comment) {
        unlink ($comment);
    }
    if ($define && -e $define) {
        unlink ($define);
    }
    if ($cpp && -e $cpp) {
        unlink ($cpp);
    }
    if ($actions && -e $actions) {
        unlink ($actions);
    }
}
