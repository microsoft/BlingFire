@perl -Sx %0 %*
@goto :eof
#!perl


sub usage {

print <<EOM;

Usage: fa_combine.pl <map1> <map2> >superposition.txt 2>undefined.txt

This program calculates a superposition of two maps.

Notes: 

  1. <map1> is read into memory
  2. maps should contain lines: <Key>[ \t]<Data>\n

EOM

}

if("--help" eq $ARGV [0]) {

  usage ();
  exit (0);
}


#
# Read in the map
#

$file1 = $ARGV [0];
shift @ARGV;

open MAP, "< $file1" ;

while(<MAP>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    # Key\tData
    m/^([^ \t]+)[ \t](.+)$/;

    if("" ne $1 && "" ne $2) {

      if(!(defined $map{$1})) {

        $map{$1} = $2;

      } else {

        $map{$1} = $map{$1} . " " . $2;
      }
    }
}

close MAP ;



# read the data

$file2 = $ARGV [0];
shift @ARGV;


open DATA, "< $file2" ;

while(<DATA>) {

    s/[\r\n]+$//;
    s/^\xEF\xBB\xBF//;

    if(/^[ ]*[#]/ || /^$/) {

      print "$_\n";

    } else {

      # Key\tData
      m/^([^ \t]+)[ \t](.+)$/;

      if(defined $map{$2}) {
        print "$1\t$map{$2}\n" ;
      } else {
        print STDERR "WARNING: no mapping found for\t$1\t$2\n" ;
      }
    }
}

close DATA ;

