@perl -Sx %0 %*
@goto :eof
#!perl

sub usage {

print <<EOM;
Based on PRM LDB run-time configuration file, builds a space separated list of
tasks for auto-test.

Usage: fa_conf2test_prm < ldb.conf

[w2b] --> test_w2b test_w2b_unk
[b2w] --> test_b2w test_b2w_unk
[w2t] --> test_w2t test_w2t_unk
[b2t] --> test_b2t test_b2t_unk
[w2s] --> test_w2s
[tag-dict] --> test_tag_dict
[w2h] --> test_w2h test_w2h_unk

EOM

}

$test_targets = "" ;

while (<>) {

  s/[\r\n]+$// ;

  if ("[w2b]" eq $_ ) {
    $test_targets .= "test_w2b test_w2b_unk ";
  } elsif ("[b2w]" eq $_ ) {
    $test_targets .= "test_b2w test_b2w_unk ";
  } elsif ("[w2t]" eq $_ ) {
    $test_targets .= "test_w2t test_w2t_unk ";
  } elsif ("[b2t]" eq $_ ) {
    $test_targets .= "test_b2t test_b2t_unk ";
  } elsif ("[w2s]" eq $_ ) {
    $test_targets .= "test_w2s ";
  } elsif ("[tag-dict]" eq $_ ) {
    $test_targets .= "test_tag_dict ";
  } elsif ("[w2h]" eq $_ ) {
    $test_targets .= "test_w2h test_w2h_unk ";
  } elsif ("[w2h-alt]" eq $_ ) {
    $test_targets .= "test_w2h_alt test_w2h_alt_unk ";
  }
}

print join (' ', sort (split (' ', $test_targets))) ;
