use ExtUtils::MakeMaker;
WriteMakefile(
    VERSION => '2.4.2.5',
    NAME => "Tcl::Tk::Tkwidget::treectrl",
    PREREQ_FATAL => 1,
    PREREQ_PM => {
       'Tcl::Tk' => 1.0,
    },
);
