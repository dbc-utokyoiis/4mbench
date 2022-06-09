#!/bin/perl

$n = 0; $ndup = 0;

while(<STDIN>){
    chomp($_);
    if($_ =~ /^#/){ next; }
    ($elid, $lid, $eid, $ts, $sensor, $reading) = split(/\|/, $_);
    $k = "$lid.$eid.$ts.$sensor";
    $s{$k}++;
    $n++;
    if($s{$k} > 1){
	$ndup++;
	print "DUP ($s{$k}) --> $_\n";
    }
}

print "$n records, $ndup duplicated.\n";

# EOF
