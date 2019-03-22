@perl -Sx %0 %*
@goto :eof
#!perl


sub usage {

print <<EOM;

Converts double-braket tree representation into XML format.

Usage: fa_tree2xml [OPTIONS] [FILES]

  --keep-nodes=<regexp>

EOM

}

#
# process command line parameters
#

$keep_nodes_re = ".*";
$input = "";
$output = "";

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0]) {

      usage ();
      exit (0);

    } elsif ($ARGV [0] =~ /^--keep-nodes=(.+)/) {

       $keep_nodes_re = $1;

    } elsif ($ARGV [0] =~ /^--/) {

      next;

    } else {

      last;
    }

    shift @ARGV;
}

# set array base to 1
$[ = 1;

while (<>) {

  s/[\r\n]+//g;

  @Fld = split(' ', $_, 999999);

  $in = 0 ;
  $from = -1 ;
  $to = -1 ;
  @tags = [];

  print "<xml> ";

  for ($i = 1; $i <= $#Fld; ++$i) {

    $s = $Fld[$i] ;

    if ($s =~ /(.*)[(][(]/) {

      $in++ ;
      $tag = $1;
      $tags[$in] = $tag;

      if($tag =~ /$keep_nodes_re/) {
          print "<" . $tag . "> ";
      }

    } elsif ($s eq "))") {

      $tag = $tags[$in];
      $in-- ;

      if($tag =~ /$keep_nodes_re/) {
          print "</" . $tag . "> ";
      }

    } else {

      print $s . " " ;
    }
  }

  print "</xml>\n";

}
