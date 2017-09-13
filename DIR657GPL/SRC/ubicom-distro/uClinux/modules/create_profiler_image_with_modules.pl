#!/usr/bin/perl

# process ath_dev.ko 0x43280000
# process wlan.ko    0x432c0000
# process ath_rate_atheros.ko 0x43270000
# process ath_hal.ko 0x43600000
use strict;

my %modules;

my $DISCARD_EXIT = 1;
my $input ='';
my $linux_ocm_sections ='';
my $linux_ram_sections ='';
my $ocm_sections ='';
my $ram_sections ='';
my $discard_sections ='';

my @linux_symbols;
my %symbol_hash;

my @include = ( '.' );
my @args;
my $proc_modules;
my $vmlinux;
my $outelf;

sub debugf {
#    printf @_;
}
sub debug {
#    print @_;
}


sub objdump_start($)
{
    my $elf = shift;
    my @dmp = `ubicom32-elf-objdump -h $elf`;
    ($? == 0) || return 0;
    return \@dmp;
}

# sub objdump_next($)
# {
#     my $dmp = shift;
#     my $line = shift @$dmp;
#     chomp $line;
#     while(@$dmp) {
# 	if ( $line =~ /(\d+)\s+(\S*)\s+([a-f\d]{8})\s*([a-f\d]{8})\s*([a-f\d]{8})\s*([\da-f]{8})\s*(\d\*\*\d)/ ) {
# 	    debug("Match '$1' '$2' '$3' '$4' '$5' '$6' '$7'\n");
# 	    my $attrib = shift @$dmp;
# 	    chomp $attrib;
# 	    $attrib =~ s/^\s*(.*)\s*$/$1/;
# 	    return \($2, hex("0x$3"), hex("0x$4", $attrib), 
# 	}
#     }
#     return 0;
# }
	
#
# load information about a kernel loadable module
#
sub load_module($$) {
    my $elf = shift;
    my $OCM = shift;

    for my $path (@include) {
	if ( -e "$path/$elf") {
	    $elf = "$path/$elf";
	    debug("Found $elf");
	    last;
	}
    }

    ( -e $elf ) || return 0;

    my @dmp = `ubicom32-elf-objdump -h $elf`;
    ($? == 0) || return 0;
    # core and init are terms used by modules.c
    my %layout = { 'ocm_text' => '',
		   'filename' => $elf,
		   'input' => "INPUT($elf)\n",
		   'core_text' => '',
		   'core_const_data' => '',
		   'core_data' => '',
		   'core_size' => 0,
		   'ocm_size' => 0,
		   'other' => '' };

    line : while (@dmp) {
	my $line = shift @dmp;
	chomp $line;

	# parse the objdump header
	#                   1     .text.ath_hk      000001bc  00000000 00000000  00000034    2**2
	if ( $line =~ /(\d+)\s+(\S*)\s+([a-f\d]{8})\s*([a-f\d]{8})\s*([a-f\d]{8})\s*([\da-f]{8})\s*(\d\*\*\d)/ ) {

	    debug("Match '$1' '$2' '$3' '$4' '$5' '$6' '$7'\n");
	    my $section_name = $2;
	    my $section_size = hex "0x$3";
	    my $section_start = hex "0x$4";

	    my $attrib = shift @dmp;
	    chomp $attrib;
	    $attrib =~ s/^\s*(.*)\s*$/$1/;
	    my $str = "\t\t$elf($section_name) /* $section_size bytes, $attrib */\n";

	    if ( $attrib !~ /ALLOC/ || $section_name eq "__versions" || $section_name eq ".modinfo" ) {
		$layout{'other'} .= $str;
		next line;
	    }

	    if ( $section_name =~ /^\.init/) {
		$layout{'other'} .= $str;
	    }
	    elsif ( $DISCARD_EXIT && $section_name =~ /^\.exit/) {
		printf("\tWarning: discarding exit $section_name\n");
		$layout{'other'} .= $str;
	    }
	    elsif ( $attrib =~ /CODE/) {
		#
		# look like code, better determine where it would be loaded
		#
		if ( $OCM && $section_name =~ /ocm_text/) {
		    $layout{'ocm_text'} .= $str;
		    $layout{'ocm_size'} += $section_size;
		} else {
		    $layout{'core_text'} .= $str;
		    $layout{'core_size'} += $section_size;
		}
	    } elsif ( $attrib =~ /READONLY/) {
		    $layout{'core_const_data'} .= $str;
		    $layout{'core_size'} += $section_size;

	    } else {
		    $layout{'core_data'} .= $str;
		    $layout{'core_size'} += $section_size;
	    }

	} else {
#	    print "NO MATCH '$line'\n";
	}
    }
    return \%layout;
}

my $LINK_LINUX = 0;

#
# link interesting parts of vmlinux
#
sub add_vmlinux($) {
    my $elf = shift;
    print "objdump $elf\n";

    system("ubicom32-elf-strip -s -o $elf.stripped $elf");
    $elf .= ".stripped";
    $input .= "INPUT($elf)\n";

    my @dmp = `ubicom32-elf-objdump -h $elf`;

    line : while (@dmp) {
	my $line = shift @dmp;
	chomp $line;
	# parse the objdump header
	#                   1     .text.ath_hk      000001bc  00000000 00000000  00000034    2**2
	if ( $line =~ /(\d+)\s+(\S*)\s+([a-f\d]{8})\s*([a-f\d]{8})\s*([a-f\d]{8})\s*([\da-f]{8})\s*(\d\*\*\d)/ ) {

	    debug("Match '$1' '$2' '$3' '$4' '$5' '$6' '$7'\n");
	    my $section_name = $2;
	    my $section_size = hex "0x$3";
	    my $section_start = hex "0x$4";

	    my $attrib = shift @dmp;
	    chomp $attrib;
	    $attrib =~ s/^\s*(.*)\s*$/$1/;
	    my $str = sprintf("\t\t$elf($section_name) /* linux - %d(0x%x) bytes, $attrib */\n", $section_size, $section_size);

	    if ( $attrib !~ /ALLOC/ && $attrib !~ /LOAD/) {
		$discard_sections .= $str;
	    } elsif ($section_name =~ /\.(init|bss|eh_frame)/ ) {
		$discard_sections .= $str;
	    } else {
		my $noload = $attrib !~ /CODE/ ? "(NOLOAD)" : "";
		$str = sprintf("\n\t$section_name 0x%x $noload : AT(0x%x) /* END %x */  ALIGN(0) SUBALIGN(0) {\n$str\t}\n", $section_start, $section_start, $section_start + $section_size);
		if ($section_name =~ /\.ocm/) {
		    $linux_ocm_sections .= $str;
		} else {
		    $linux_ram_sections .= $str;
		}
	    }
	}
    }
}

#
# add symbols from a linked elf file
#
sub add_symbols($$) {
    my $elf = shift;
    my $prefix = shift;
    print "Reading Symbols for $elf prefix '$prefix'\n";
    my @syms = `ubicom32-elf-nm $elf | sort`;
    $? == 0 || die "Couldn't run nm on $elf";

    for my $sym (@syms) {

	# parse the objdump header
	#                   1     .text.ath_hk      000001bc  00000000 00000000  00000034    2**2
	if ($sym =~ /^([a-f\d]{8})\s+(\w)\s+(\w+)/ ) {
#	     debug("$elf Symbol '$1' '$2' '$3' ");
	     my $symbol = $3;
	     my $section_type = $2;
	     my $symbol_addresss = hex "0x$1";
	     my $n = $symbol_hash{$symbol} + 0;
	     $symbol_hash{$symbol} = $n + 1;

	     if ($n) {
		 $n = "_v$n";
	     } else {
		 $n = '';
	     }
	     if ($symbol =~ /^__crc_.*/) {
		 next;
	     }

	     if ($symbol_addresss > 0x3ff00000 && $symbol_addresss < 0x44000000) {
		 push @linux_symbols, sprintf("%-25s = 0x%x; /* $section_type */ \n", "$prefix$symbol$n", $symbol_addresss);
	     }
	} else {
#	    print "NO MATCH '$line'\n";
	}
    }
    return 0;
}

#
# read modules
#
sub read_options()
{
    while ($_ = shift @ARGV) {

	if ($_ =~ /^(\-.)(.*)/ ) {

	    if ($1 eq '-I' ) {

		if ($2 ne '') {
		    push @include, $2;
		} else {
		    push @include, shift @ARGV;
		}

		print "Option: -I include " . $include[1] . "\n";

	    } else  {
		printf "unknown option $_\n";
	    }
	}
	push @args, $_;
    }

    $proc_modules = $args[0];
    $vmlinux = $args[1];
    $outelf =  @args[2];

}

#
# read modules
#
sub read_modules() {

    print "Open '$proc_modules'\n";
    open(MODULES, "<$proc_modules") || die "Cannot open '$proc_modules'";

    if ($LINK_LINUX)
    {
	add_vmlinux($vmlinux);
    }

    add_symbols($vmlinux, '');

    while(<MODULES>) {
	chomp;
	if ( /^(\w+) (\d+) .*Live 0x([a-f0-9]+)(.*)/ ) {
	    my $module = $1;
	    my $size =$2;
	    my $ram = hex($3);
	    my $ocm = 0;
	    if ($4 =~ /OCM\(.*0x([a-f0-9]+)\)/ ) {
		$ocm = hex($1);
	    }
	    printf "Found %-16s of %5dcc @ %x ocm %x\n",  "'$module'", $size, $ram, $ocm;
	    my $filename = "$module.ko";
	    my $layout = load_module($filename, $ocm != 0);
	    if (!$layout) {
		$filename =~ s/_/-/;
		$layout = load_module($filename, $ocm != 0);
	    }
	    if (!$layout) {
		print "\tWARNING: Couldn't find $filename or $module.ko, skipping\n";
	    } else {
		$$layout{'ram'} = $ram;
		$$layout{'ocm'} = $ocm;
		$modules{'$module'} = $layout;

		$input .= $$layout{'input'};
		my $ocm_section = sprintf("\t/* %s OCM size %d(0x%x) bytes */\n", $filename, $$layout{'ocm_size'}, , $$layout{'ocm_size'});
		if ($$layout{'ocm_size'}) {
		    $ocm_section .= sprintf("\t.ocm.$filename 0x%x : AT(0x%x) ALIGN(0) SUBALIGN(0){\n", $ocm, $ocm);
		    $ocm_section .= $$layout{'ocm_text'};
		    $ocm_section .= "\t}\n";
		}

		my $ram_section = sprintf("\t/* %s RAM size %d(0x%x) bytes */\n", $filename, $$layout{'core_size'}, $$layout{'core_size'});
		$ram_section .= sprintf("\t.ram.$filename 0x%x : AT(0x%x) ALIGN(0) SUBALIGN(0) {\n", $ram, $ram);
		$ram_section .= $$layout{'core_text'};
		$ram_section .= $$layout{'core_const_data'};
		$ram_section .= $$layout{'core_data'};
		$ram_section .= "\t}\n";

		debug "SECTIONS\n$ocm_section$ram_section\n";
		$ocm_sections .= $ocm_section;
		$ram_sections .= $ram_section;
		$discard_sections .= $$layout{'other'};
		$input .= $$layout{'input'};

	    }
	}
    }


    if (!$LINK_LINUX) {
	$linux_ram_sections = ".text 0x40000000 : AT(0x40000000) ALIGN(0) SUBALIGN(0) { . += 4; }";
    }
}

sub relink()
{
    # create the lds file
    my $LDS =<<EOF;
OUTPUT_ARCH(ubicom32)
    /*MEMORY {
	ram : ORIGIN = 0x40000000, LENGTH = (0x44000000)
	ocm_text : ORIGIN = (0x3ffc000), LENGTH = 0x3c000
	ocm_data : ORIGIN = (0x3ffc0000 +0x0003C000 -0x19000), LENGTH = 0x19000
    }*/
    __ocm_begin = 0x3ffc0000;
__ocm_limit = __ocm_begin + 0x0003C000;
__sdram_begin = 0x40000000;
__sdram_limit = __sdram_begin + 0x4000000;
__filemedia_begin_addr = 0x60000000;
__filemedia_end_addr = __filemedia_begin_addr + 0x00800000;
$input

@linux_symbols

SECTIONS {
$linux_ocm_sections
$ocm_sections
$linux_ram_sections
$ram_sections
	DISCARD (NOLOAD) : {
$discard_sections
	}
}

EOF
;

    my $lds = "$outelf.lds";
    open(LDS, ">$lds") || die "Couldn't open $outelf";
    print LDS $LDS;
    close LDS;

    print "Linking with $lds to $outelf.all\n";
    system("ubicom32-elf-ld --script  $lds --allow-multiple-definition -o $outelf.all") == 0 || die "ERROR: Couldn't link?\n";
    print "Discarding unwanted from $lds\n";
    system("ubicom32-elf-objcopy -R DISCARD $outelf.all $outelf") == 0 || die;
    print(" Final Sections in Elf\n");
    system("ubicom32-elf-objdump -h $outelf | egrep '^ *[0-9].*' ") == 0 || die;
}

if ($#ARGV < 2) {
    print "Usage: $0 proc_modules vmlinux profiler_elf -I path\n";
    print "    procmodules is a plain text file containing output of 'cat /proc/modules' on target system	\n";
    print "    vmlinux is name of the kernel image 								\n";
    print "    profiler_elf is the output file									\n";
    print "    path is where to look for .ko files								\n";
    exit;
}

read_options();
read_modules();
relink();

exit 0;
