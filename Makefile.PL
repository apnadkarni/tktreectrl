sub _die ($) {
    # CPAN smokers report FAIL if Makefile.PL dies, it should exit with status 0
    my $err = shift;
    warn $err;
    exit 0;
}

use strict;
use Getopt::Long;
use ExtUtils::MakeMaker;

eval {
    WriteMakefile(
        VERSION => '2.4.2.2',
        NAME => "Tcl::Tk::Tkwidget::treectrl",
        PREREQ_FATAL => 1,
        PREREQ_PM => {
           'Tcl::Tk' => 1.0,
        },
    );
};
_die $@ if $@;
