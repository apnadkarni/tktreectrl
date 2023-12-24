use strict;
use Getopt::Long;
use ExtUtils::MakeMaker;

WriteMakefile(
    VERSION => '2.4.2.2',
    NAME => "Tcl::Tk::Tkwidget::treectrl",
    PREREQ_PM => {
	'Tcl::Tk' => 1.0,
    },
);

