#!/usr/bin/perl
#
# configure.win.pl
# MegaMol Core
#
# Copyright (C) 2008-2010 by VISUS (Universitaet Stuttgart).
# Alle Rechte vorbehalten.
#
use Cwd qw{abs_path};
use strict;
use warnings 'all';
my $incpath = abs_path($0);
$incpath =~ s/\/[^\/]+$//;
push @INC, "$incpath/configperl";
require configperl;

my $fullauto = 0;
if ((grep {$_ eq "fullauto"} @ARGV) || (defined $ENV{'CONFIGPERL_FULLAUTO'})) {
    $fullauto = 1;
}

my ($a, $b, $c);
my @pps = ();
my @fps = ();
my @cfps = ();
my @sps = ();

$a = PathParameter->new();
    $a->id("outbin");
    $a->description("Path to the global \"bin\" output directory");
    $a->placeholder("%outbin%");
    $a->autoDetect(0);
    $a->value("../bin");
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    push @pps, $a;

$a = PathParameter->new();
    $a->id("vislib");
    $a->description("Path to the vislib directory");
    $a->placeholder("%vislib%");
    $a->markerFile("vislib.sln\$");
    $a->relativeLocation("./");
    $a->autoDetect(1);
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("expat");
    $a->description("Path to the expat directory");
    $a->placeholder("%expatPath%");
    $a->markerFile("expat.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("libpng");
    $a->description("Path to the libpng directory");
    $a->placeholder("%pngPath%");
    $a->markerFile("png.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("zlib");
    $a->description("Path to the zlib directory");
    $a->placeholder("%zlibPath%");
    $a->markerFile("zlib.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    $a->directorySeparator("\\");
    $a->enforceTrailingDirectorySeparator(1);
    push @pps, $a;
$a = FlagParameter->new();
    $a->id("mpi");
    $a->description("Enable MPI-based cluster communication");
    $a->placeholder("withmpi");
    $a->value(0);
    push @fps, $a;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->id("mpiincpath");
    $a->description("Path to the MPI include path");
    $a->placeholder("%mpiincpath%");
    $a->markerFile("mpi.h\$");
    $a->relativeLocation(".\\");
    $a->enforceTrailingDirectorySeparator(1);
    $a->dependencyFlagID("mpi");
    $a->dependencyDisabledValue("");
    push @pps, $a;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->id("mpilib32path");
    $a->description("Path to the 32 bit MPI library");
    $a->placeholder("%mpilib32path%");
    $a->markerFile("msmpi.lib\$");
    $a->dependencyFlagID("mpi");
    $a->dependencyDisabledValue("");
    push @pps, $a;
$a = PathParameter->new();
    $a->directorySeparator("\\");
    $a->id("mpilib64path");
    $a->description("Path to the 64 bit MPI library");
    $a->placeholder("%mpilib64path%");
    $a->markerFile("msmpi.lib\$");
    $a->dependencyFlagID("mpi");
    $a->dependencyDisabledValue("");
    push @pps, $a;

$c = ConfigFilePair->new();
    $c->inFile("ExtLibs.props.input");
    $c->outFile("ExtLibs.props");
    push @cfps, $c;

VISUS::configperl::Configure("MegaMol(TM) Core Configuration for Windows", ".megamol.core.win.cache", \@pps, \@fps, \@cfps, \@sps, $fullauto);

