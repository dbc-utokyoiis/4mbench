#!/bin/perl

while(<STDIN>){
    chomp($_);
    if($_ =~ /^#/){ next; }
    ($elid, $lid, $eid, $ts, $sensor, $reading) = split(/\|/, $_);
    $k = "$eid.$sensor";
    $v = $reading;

    $n{$k}++;
    $d{$k}{$v}++;
}

foreach $k1 (sort {$a <=> $b} keys(%n)){
    print "[$k1] $n{$k1} records\n";
    $v = $d{$k1};
    $n = 0; $ne = 0;
    foreach $k2 (sort {$a <=> $b} keys(%$v)){
	printf("  %s\t%10d\t%10.6f ", $k2, $d{$k1}{$k2}, $d{$k1}{$k2} / $n{$k1});
	$n += $d{$k1}{$k2};
	if($k1 =~ /STATUS/){
	    if($k2 != 1){ print "ERROR"; $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /0.PRESSURE/){
	    if($k2 < 110 || $k2 > 140){ print "ERROR"; $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /.PRESSURE/){
	    if($k2 < 10 || $k2 > 20){ print "ERROR"; $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /1.TEMPERATURE/){
	    if($k2 < 178 || $k2 > 182){ print "ERROR"; $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /2.TEMPERATURE/){
	    if($k2 < 40 || $k2 > 55){ print "ERROR"; $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /8.TEMPERATURE/){
	    if($k2 < 5 || $k2 > 35){ print "ERROR"; $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /.TEMPERATURE/){
	    if($k2 < 10 || $k2 > 20){ print "ERROR"; $ne += $d{$k1}{$k2}; }
	}
	print "\n";
    }
    printf("  ### Summary: %d errors out of %d, error ratio: %10.6f\n", $ne, $n, $ne / $n);;
}

# EOF
