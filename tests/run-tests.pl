#! /usr/bin/perl

use File::Slurp qw(slurp);

my $interp = $ENV{"MINIME"} || "../minime 2>/dev/null";
my $suites = $ENV{"SUITES"} || "simple";


## expected: errors don't start with a space
## actual: errors don't start with =>

## match substrings due to #<primitive-procedure 0x4....>
sub expectation_ok {
    my ($actual, $expected) = @_;

    if ($actual =~ /^=> (.+)$/) {
	    $actual = $1;

	    ## expect value
	    return undef unless ($expected =~ s/^ //);

	    return ($expected eq substr($actual, 0, length($expected)));
    }

    return ($expected eq substr($actual, 0, length($expected)));

}

sub run_tests
{
    my $suite = shift;

    open(my $fh, "$interp < testcases.$suite |") or die;
    my @actual = slurp($fh);

    my $total = 0, $fail = 0, $line = 0;

    open(my $fh, '<', "testcases.$suite") or die;
    while (<$fh>) {
	$line++;
	next if /^\s*$/;	# skip empty
	next if /^;/;		# skip comment lines

	chomp;

	my ($sexp, $expected) = split /;/;
	$sexp     =~ s/\s+$//;	# trim right whitespace
	$expected =~ s/\s+$//;


	my $actual = shift @actual;
	chomp $actual;

	unless (expectation_ok($actual, $expected)) {
	    print " -- $sexp :$line, expected '$expected', actual '$actual'\n";
	    $fail++;
	}

	$total++;
    }

    print $fail ?
          "FAIL $fail of $total tests in $suite\n" :
	  "PASS $total of $total tests in $suite\n";
}


## run selected test suites
run_tests($_) foreach split(/ +/, $suites);


