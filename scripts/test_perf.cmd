@perl -Sx %0 %*
@goto :eof
#!perl

sub usage {

print <<EOM;

Usage: test_perf [OPTIONS] --cmd <command line>

This program measures performance of a <command line>. The program of the 
<command line> have to support two parameters: --no-output and --no-process.

  --trials=N - the number of times <program> will be executed,
    10 is used by default

  --verbose - generates verbose report, not just processing time

  --hires - uses high resolution timer, requires Time::HiRes perl module

EOM
}

$trials = 10;
$high_res = 0;
$verbose = 0;

while (0 < 1 + $#ARGV) {

    if("--help" eq $ARGV [0] or "-help" eq $ARGV [0] or "/?" eq $ARGV [0]) {
        usage ();
        exit (0);
    } elsif ($ARGV [0] =~ /^--trials=(.+)/) {
        $trials = $1;
    } elsif ("--hires" eq $ARGV [0]) {
        $high_res = 1;
    } elsif ("--verbose" eq $ARGV [0]) {
        $verbose = 1;
    } elsif ("--cmd" eq $ARGV [0]) {
        shift @ARGV;
        last;
    }
    shift @ARGV;
}

if($high_res) {
  require "Time/HiRes.pm";
  import Time::HiRes ;
}

sub mytimer {
    if($high_res) {
        return Time::HiRes::time();
    } else {
        return time();
    }
}


# see whether command line is specified
$command = join(" ", @ARGV);
if ("" eq $command) {
    printf "ERROR: Command line is not specified!\n";
    usage ();
    exit 1;
}

# see whether times function works
$works = mytimer();
sleep 2;
$works = mytimer() - $works;
if (!$works) {
    printf STDERR "ERROR: timer does not work!\n";
    exit 1;
}

# getting IO time
$io_time = -1.0;
for ($i = 0; $i < $trials; $i++) {

    $a = mytimer() ;
    `$command --no-output --no-process` ;
    $b = mytimer() - $a ;

    if ($b < $io_time or -1.0 == $io_time) {
        $io_time = $b ;
    }
}

# getting processing and IO time
$exec_time = -1.0;
for ($i = 0; $i < $trials; $i++) {

    $a = mytimer() ;
    `$command --no-output` ;
    $b = mytimer() - $a ;

    if ($b < $exec_time or -1.0 == $exec_time) {
        $exec_time = $b ;
    }
}

# print results
if ($verbose) {
    printf "Proc  %.2f s\nIO    %.2f s\nTotal %.2f s\n", ($exec_time - $io_time), $io_time, $exec_time;
}  else {
    printf "%.2f\n", $exec_time - $io_time;
}
