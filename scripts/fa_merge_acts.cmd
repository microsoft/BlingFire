@perl -Sx %0 %*
@goto :eof
#!perl


sub usage {

print <<EOM;

Usage: fa_merge_acts FILE [FILE [...]] > output.cpp

This program creates a common source file which merges several stages 
of actions.

  --name=<NAME> - uses NAME for common container objects

EOM

}

#
# *** Process command line parameters ***
#

$name = "Acts";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

        usage ();
        exit (0);

    } elsif ($ARGV [0] =~ /^--name=(.+)/) {

        $name = $1;

    } elsif ($ARGV [0] =~ /^-.*/) {

        print STDERR "ERROR: Unknown parameter $$ARGV[0], see fa_merge_acts --help";
        exit (1);

    } else {

        last;
    }
    shift @ARGV;
}

#
# *** make 
#

print <<PRINT_END;

#include "FAActionsArray_static.h"
PRINT_END


foreach $stage(@ARGV) {
  print "#include \"$stage.cxx\"\n";
}
print "\nclass FAActionsA;\n\n\n";


foreach $stage(@ARGV) {
  print "$stage g_$stage;\n";
}
print ("\n\nconst FAActionsA * g_Array$name [] = {\n");


foreach $stage(@ARGV) {
  print "    & g_$stage,\n";
}
print "};\n\n";


print <<PRINT_END;
FAActionsArray_static g_$name\Stages (
        & g_Array$name [0], 
        sizeof (g_Array$name) / sizeof (g_Array$name [0])
    );

const FAActionsArrayA * g_p$name\Stages = & g_$name\Stages;
PRINT_END
