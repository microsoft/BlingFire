@perl -Sx %0 %*
@goto :eof
#!perl

use encoding 'utf8';

while (<>) {

  s/[\n\r]//g;

  # the first command makes words space-separated
  # next commands process special symbols

  s/([ \t\r]+)/ /g ;
  s/(^|[ ])([-+*\/\\|&()\[\]!?.,;:=][-+*\/\\|&()\[\]!?.,;:=]?[-+*\/\\|&()\[\]!?.,;:=]?)(\w)/$1$2 $3/g ;
  s/(\w)([-+*\/\\|&()\[\]!?.,;:=][-+*\/\\|&()\[\]!?.,;:=]?[-+*\/\\|&()\[\]!?.,;:=]?)([ ]|$)/$1 $2 /g ;
  s/(\w)([-+*\/\\|&()\[\]!?.,;:=][-+*\/\\|&()\[\]!?.,;:=]?[-+*\/\\|&()\[\]!?.,;:=]?)(\w)/$1 $2 $3/g ;

  # remove final space
  s/ $//g ;

  # add an NP tag
  s/( )/\/NP /g ;
  s/([^ ])$/$1\/NP/g ;

  print $_ . "\n";
}
