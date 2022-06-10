#!/bin/perl

while(<STDIN>){
    chomp($_);
    if($_ =~ /^#/){ next; }
    ($mlid, $lid, $mtype, $olid1, $oldid2, $serial, $r1, $r2, $r3, $r4, $r5, $r6, $comment) = split(/\|/, $_);
    
    $k = "$mtype.WEIGHT"; $v = $r1;
    $n{$k}++; $d{$k}{$v}++;
    $k = "$mtype.DIMENSIONX"; $v = $r2;
    $n{$k}++; $d{$k}{$v}++;
    $k = "$mtype.DIMENSIONY"; $v = $r3;
    $n{$k}++; $d{$k}{$v}++;
    $k = "$mtype.DIMENSIONZ"; $v = $r4;
    $n{$k}++; $d{$k}{$v}++;
    $k = "$mtype.SHAPESCORE"; $v = $r5;
    $n{$k}++; $d{$k}{$v}++;
    $k = "$mtype.TEMPERATURE"; $v = $r6;
    $n{$k}++; $d{$k}{$v}++;
}

foreach $k1 (sort {$a cmp $b} keys(%n)){
    # print "[$k1] $n{$k1} records\n";
    print "[$k1]\t";
    $v = $d{$k1};
    $n = 0; $ne = 0;
    foreach $k2 (sort {$a <=> $b} keys(%$v)){
	# printf("  %s\t%10d\t%10.6f ", $k2, $d{$k1}{$k2}, $d{$k1}{$k2} / $n{$k1});
	$n += $d{$k1}{$k2};
	if($k1 =~ /MT0[01].WEIGHT/){
	    if($k2 < 98.0 || $k2 > 102.0){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[23].WEIGHT/){
	    if($k2 < 89.0 || $k2 > 91.0){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[45].WEIGHT/){
	    if($k2 < 600.0-7.0 || $k2 > 600.0+7.0){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[67].WEIGHT/){
	    if($k2 < 14400.0-160.0 || $k2 > 14400.0+160.0){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[89].WEIGHT/){
	    if($k2 < 692000.0-7680.0 || $k2 > 692000.0+7680.0){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[01].DIMENSIONX/){
	    if($k2 < 5.0-0.5 || $k2 > 5.0+0.5){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[23].DIMENSIONX/){
	    if($k2 < 4.5-0.4 || $k2 > 4.5+0.4){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[4567].DIMENSIONX/){
	    if($k2 < 10.0-0.1 || $k2 > 10.0+0.1){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[89].DIMENSIONX/){
	    if($k2 < 42.0-0.4 || $k2 > 42.0+0.4){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[01].DIMENSIONY/){
	    if($k2 < 5.0-0.5 || $k2 > 5.0+0.5){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[23].DIMENSIONY/){
	    if($k2 < 4.5-0.4 || $k2 > 4.5+0.4){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[45].DIMENSIONY/){
	    if($k2 < 5.0-0.1 || $k2 > 5.0+0.1){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[67].DIMENSIONY/){
	    if($k2 < 31.0-0.6 || $k2 > 31.0+0.6){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[89].DIMENSIONY/){
	    if($k2 < 95.0-1.8 || $k2 > 95.0+1.8){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[01].DIMENSIONZ/){
	    if($k2 < 1.0-0.1 || $k2 > 1.0+0.1){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[23].DIMENSIONZ/){
	    if($k2 < 0.9-0.09 || $k2 > 0.9+0.09){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[45].DIMENSIONZ/){
	    if($k2 < 5.0-0.1 || $k2 > 5.0+0.1){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[67].DIMENSIONZ/){
	    if($k2 < 21.0-0.4 || $k2 > 21.0+0.4){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[89].DIMENSIONZ/){
	    if($k2 < 85.0-1.6 || $k2 > 85.0+1.6){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /SHAPESCORE/){
	    if($k2 < 0.98){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT0[01].TEMPERATURE/){
	    if($k2 < 2.0 || $k2 > 5.0){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /MT02.TEMPERATURE/){
	    if($k2 < 178.0 || $k2 > 182.0){ $ne += $d{$k1}{$k2}; }
	}elsif($k1 =~ /.TEMPERATURE/){
	    if($k2 < 5.0 || $k2 > 35.0){ $ne += $d{$k1}{$k2}; }
	}
	# print "\n";
    }
    # printf("  ### Summary: %d errors out of %d, error ratio: %10.6f\n", $ne, $n, $ne / $n);
    printf("error ratio: %10.6f (%d errors out of %d)\n", $ne / $n, $ne, $n);
}

# EOF
