#!/usr/bin/perl

use strict;
use warnings;

# Takes the source code and the output of perlcritic and make a
# per-function report on the state of the source.  Very
# rough-and-ready but it helps in it's own little way.

my $line = 1;

my %calls;
my %subs;

my @builtin =
  qw(basename Dumper qw Configure dclone join defined delete print shift close accept confess A new thaw add
  remove count flush handles printf split dirname);

my %builtin;

foreach (@builtin) {
    $builtin{$_}++;
}

my $current_fn;

my %fns;
my @fns;
my @calls;
my %refs;

my $PH;

open $PH, '<', 'padb';

while (<$PH>) {
    if ( m{\A[^#]*?(\w+)\(}x and not defined $builtin{$1} ) {
        push @{ $calls{$1} }, $line;
        $calls[$line] = $1;

    } elsif (m{\Asub\s+(\w+)}) {
        $current_fn = $1;

        $fns{$current_fn}{start} = $line;
        push @fns, $current_fn;
    } elsif ( m{\A\}} and defined $current_fn ) {

        $fns{$current_fn}{end} = $line;
        $current_fn = undef;
    } elsif (m{\\\&(\w+)}) {
        $refs{$1} = 1;
    }

    $line++;
}

close $PH;

my $PPH;
open $PPH, '<', 'pc';

my @pc_names = qw(brutal cruel harsh stern gentle);

my @errors;
while (<$PPH>) {
    if (m{\A(\d+)\:[ ]\((\d)\)[ ](.*)\Z}x) {
        push @{ $errors[$1] }, "$pc_names[$2-1]($2): $3";
    } else {
        printf "?? $_\n";
    }
}

foreach my $fn (@fns) {
    foreach my $line ( $fns{$fn}{start} .. $fns{$fn}{end} ) {

        if ( defined $errors[$line] ) {
            @{ $fns{$fn}{errors}{ $line - $fns{$fn}{start} } } =
              @{ $errors[$line] };
        }
        next unless defined( $calls[$line] );
        $fns{ $calls[$line] }{called_by}{$fn}++;
        $fns{$fn}{calls}{ $calls[$line] }++;
    }
}

foreach my $fn (@fns) {
    printf("Function: $fn $fns{$fn}{start}\n");

    if ( defined $refs{$fn} ) {
        printf("\tIs dereferenced\n");
    }
    foreach my $cf ( sort keys %{ $fns{$fn}{called_by} } ) {
        printf("\tIs called by:\t$cf ($fns{$fn}{called_by}{$cf} times)\n");
    }
    foreach my $cf ( sort keys %{ $fns{$fn}{calls} } ) {
        printf("\tCalls:\t\t$cf ($fns{$fn}{calls}{$cf} times)\n");
    }
    foreach my $el ( sort { $a <=> $b } keys %{ $fns{$fn}{errors} } ) {
        foreach my $error ( @{ $fns{$fn}{errors}{$el} } ) {

            #printf("\tError:\t\t$fns{$fn}{errors}{$el} ($el)\n");
            printf("\tError:\t\t$error ($el)\n");
        }
    }
    printf("\n");
}

exit 0;
