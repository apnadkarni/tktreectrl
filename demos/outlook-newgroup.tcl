# Copyright (c) 2002-2011 Tim Baker

#
# Demo: Outlook Express newsgroup messages
#
namespace eval DemoOutlookNewsgroup {}
proc DemoOutlookNewsgroup::Init {T} {

    variable Priv

    InitPics outlook-*

    set height [font metrics [$T cget -font] -linespace]
    if {$height < 18} {
	set height 18
    }

    #
    # Configure the treectrl widget
    #

    $T configure -itemheight $height -selectmode browse \
	-showroot no -showrootbutton no -showbuttons yes -showlines no \
	-xscrollincrement 20 -xscrollsmoothing yes

    switch -- [$T theme platform] {
	visualstyles {
	    $T theme setwindowtheme "Explorer"
	}
    }

    #
    # Create columns
    #

    $T column create -image outlook-clip -tags clip
    $T column create -image outlook-arrow -tags arrow
    $T column create -image outlook-watch -tags watch
    $T column create -text Subject -width 250 -tags subject
    $T column create -text From -width 150 -tags from
    $T column create -text Sent -width 150 -tags sent
    $T column create -text Size -width 60 -justify right -tags size

#    $T column configure all -gridrightcolor #ebf4fe

    # Would be nice if I could specify a column -tag too
    # *blink* The amazing code Genie makes it so!!!
    $T configure -treecolumn subject

    # State for a read message
    $T item state define read

    # State for a message with unread descendants
    $T item state define unread

    # States for "open" rectangles.  This is an ugly hack to get the
    # active outline to span multiple columns.
    $T item state define openWE
    $T item state define openE
    $T item state define openW

    #
    # Create elements
    #

    $T element create elemImg image -image {
	outlook-read-2Sel {selected read unread !open}
	outlook-read-2 {read unread !open}
	outlook-readSel {selected read}
	outlook-read {read}
	outlook-unreadSel {selected}
	outlook-unread {}
    }

    $T element create elemTxt text -fill [list $::SystemHighlightText {selected focus}] \
	-font [list DemoFontBold {read unread !open} DemoFontBold {!read}] -lines 1

    $T element create sel rect \
	-fill [list $::SystemHighlight {selected focus} gray {selected !focus}] \
	-open {we openWE e openE w openW} -showfocus yes

    #
    # Create styles using the elements
    #

    # Image + text
    set S [$T style create s1]
    $T style elements $S {sel elemImg elemTxt}
    $T style layout $S elemImg -expand ns
    $T style layout $S elemTxt -padx {2 6} -squeeze x -expand ns
    $T style layout $S sel -union [list elemTxt] -iexpand nes -ipadx {2 0}

    # Text
    set S [$T style create s2]
    $T style elements $S {sel elemTxt}
    $T style layout $S elemTxt -padx 6 -squeeze x -expand ns
    $T style layout $S sel -detach yes -iexpand xy

    # Set default item styles
    $T column configure subject -itemstyle s1
    $T column configure from -itemstyle s2
    $T column configure sent -itemstyle s2
    $T column configure size -itemstyle s2

    #
    # Create items and assign styles
    #

    set msgCnt 100

    set thread 0
    set Priv(count,0) 0
    set items [$T item id root]
    for {set i 1} {$i < $msgCnt} {incr i} {
	set itemi [$T item create]
	while 1 {
	    set j [expr {int(rand() * $i)}]
	    set itemj [lindex $items $j]
	    if {$j == 0} break
	    if {[$T depth $itemj] == 5} continue
	    if {$Priv(count,$Priv(thread,$itemj)) == 15} continue
	    break
	}
	$T item lastchild $itemj $itemi

	set Priv(read,$itemi) [expr rand() * 2 > 1]
	if {$j == 0} {
	    set Priv(thread,$itemi) [incr thread]
	    set Priv(seconds,$itemi) [expr {[clock seconds] - int(rand() * 500000)}]
	    set Priv(seconds2,$itemi) $Priv(seconds,$itemi)
	    set Priv(count,$thread) 1
	} else {
	    set Priv(thread,$itemi) $Priv(thread,$itemj)
	    set Priv(seconds,$itemi) [expr {$Priv(seconds2,$itemj) + int(rand() * 10000)}]
	    set Priv(seconds2,$itemi) $Priv(seconds,$itemi)
	    set Priv(seconds2,$itemj) $Priv(seconds,$itemi)
	    incr Priv(count,$Priv(thread,$itemj))
	}
	lappend items $itemi
    }

    for {set i 1} {$i < $msgCnt} {incr i} {
	set itemi [lindex $items $i]
	set subject "This is thread number $Priv(thread,$itemi)"
	set from somebody@somewhere.net
	set sent [clock format $Priv(seconds,$itemi) -format "%d/%m/%y %I:%M %p"]
	set size [expr {1 + int(rand() * 10)}]KB

	# This message has been read
	if {$Priv(read,$itemi)} {
	    $T item state set $itemi read
	}

	# This message has unread descendants
	if {[AnyUnreadDescendants $T $itemi]} {
	    $T item state set $itemi unread
	}

	if {[$T item numchildren $itemi]} {
	    $T item configure $itemi -button yes

	    # Collapse some messages
	    if {rand() * 2 > 1} {
		$T item collapse $itemi
	    }
	}

#		$T item style set $i 3 s1 4 s2.we 5 s2.we 6 s2.w
	$T item text $itemi subject $subject from $from sent $sent size $size

	$T item state forcolumn $itemi subject openE
	$T item state forcolumn $itemi from openWE
	$T item state forcolumn $itemi sent openWE
	$T item state forcolumn $itemi size openW
    }

    # Do something when the selection changes
    $T notify bind $T <Selection> {
	DemoOutlookNewsgroup::Selection %T
    }

    # Fix the display when a column is dragged
    $T notify bind $T <ColumnDrag-receive> {
	%T column move %C %b
	DemoOutlookNewsgroup::FixItemStyles %T
    }

    # Fix the display when a column's visibility changes
    $T notify bind $T <DemoColumnVisibility> {
	DemoOutlookNewsgroup::FixItemStyles %T
    }

    return
}

proc DemoOutlookNewsgroup::Selection {T} {
    variable Priv
    # One item is selected
    if {[$T selection count] == 1} {
	if {[info exists Priv(afterId)]} {
	    after cancel $Priv(afterId)
	}
	set Priv(afterId,item) [$T selection get 0]
	set Priv(afterId) [after 500 DemoOutlookNewsgroup::MessageReadDelayed]
    }
    return
}

proc DemoOutlookNewsgroup::MessageReadDelayed {} {

    variable Priv

    set T [DemoList]

    unset Priv(afterId)
    set I $Priv(afterId,item)
    if {![$T selection includes $I]} return

    # This message is not read
    if {!$Priv(read,$I)} {

	# Read the message
	$T item state set $I read
	set Priv(read,$I) 1

	# Check ancestors (except root)
	foreach I2 [lrange [$T item ancestors $I] 0 end-1] {

	    # This ancestor has no more unread descendants
	    if {![AnyUnreadDescendants $T $I2]} {
		$T item state set $I2 !unread
	    }
	}
    }
    return
}

# Alternate implementation that does not rely on run-time states
proc DemoOutlookNewsgroup::Init_2 {T} {

    global Message

    InitPics outlook-*

    set height [font metrics [$T cget -font] -linespace]
    if {$height < 18} {
	set height 18
    }

    #
    # Configure the treectrl widget
    #

    $T configure -itemheight $height -selectmode browse \
	-showroot no -showrootbutton no -showbuttons yes -showlines no

    #
    # Create columns
    #

    $T column create -image outlook-clip -tags clip
    $T column create -image outlook-arrow -tags arrow
    $T column create -image outlook-watch -tags watch
    $T column create -text Subject -width 250 -tags subject
    $T column create -text From -width 150 -tags from
    $T column create -text Sent -width 150 -tags sent
    $T column create -text Size -width 60 -justify right -tags size

    $T configure -treecolumn 3

    #
    # Create elements
    #

    $T element create image.unread image -image outlook-unread
    $T element create image.read image -image outlook-read
    $T element create image.read2 image -image outlook-read-2
    $T element create text.read text -fill [list $::SystemHighlightText {selected focus}] \
	-lines 1
    $T element create text.unread text -fill [list $::SystemHighlightText {selected focus}] \
	-font [list DemoFontBold] -lines 1
    $T element create sel.e rect -fill [list $::SystemHighlight {selected focus} gray {selected !focus}] -open e -showfocus yes
    $T element create sel.w rect -fill [list $::SystemHighlight {selected focus} gray {selected !focus}] -open w -showfocus yes
    $T element create sel.we rect -fill [list $::SystemHighlight {selected focus} gray {selected !focus}] -open we -showfocus yes

    #
    # Create styles using the elements
    #

    # Image + text
    set S [$T style create unread]
    $T style elements $S {sel.e image.unread text.unread}
    $T style layout $S image.unread -expand ns
    $T style layout $S text.unread -padx {2 6} -squeeze x -expand ns
    $T style layout $S sel.e -union [list text.unread] -iexpand nes -ipadx {2 0}

    # Image + text
    set S [$T style create read]
    $T style elements $S {sel.e image.read text.read}
    $T style layout $S image.read -expand ns
    $T style layout $S text.read -padx {2 6} -squeeze x -expand ns
    $T style layout $S sel.e -union [list text.read] -iexpand nes -ipadx {2 0}

    # Image + text
    set S [$T style create read2]
    $T style elements $S {sel.e image.read2 text.unread}
    $T style layout $S image.read2 -expand ns
    $T style layout $S text.unread -padx {2 6} -squeeze x -expand ns
    $T style layout $S sel.e -union [list text.unread] -iexpand nes -ipadx {2 0}

    # Text
    set S [$T style create unread.we]
    $T style elements $S {sel.we text.unread}
    $T style layout $S text.unread -padx 6 -squeeze x -expand ns
    $T style layout $S sel.we -detach yes -iexpand xy

    # Text
    set S [$T style create read.we]
    $T style elements $S {sel.we text.read}
    $T style layout $S text.read -padx 6 -squeeze x -expand ns
    $T style layout $S sel.we -detach yes -iexpand xy

    # Text
    set S [$T style create unread.w]
    $T style elements $S {sel.w text.unread}
    $T style layout $S text.unread -padx 6 -squeeze x -expand ns
    $T style layout $S sel.w -detach yes -iexpand xy

    # Text
    set S [$T style create read.w]
    $T style elements $S {sel.w text.read}
    $T style layout $S text.read -padx 6 -squeeze x -expand ns
    $T style layout $S sel.w -detach yes -iexpand xy

    #
    # Create items and assign styles
    #

    set msgCnt 100

    set thread 0
    set Priv(count,0) 0
    for {set i 1} {$i < $msgCnt} {incr i} {
	$T item create
	while 1 {
	    set j [expr {int(rand() * $i)}]
	    if {$j == 0} break
	    if {[$T depth $j] == 5} continue
	    if {$Priv(count,$Priv(thread,$j)) == 15} continue
	    break
	}
	$T item lastchild $j $i

	set Priv(read,$i) [expr rand() * 2 > 1]
	if {$j == 0} {
	    set Priv(thread,$i) [incr thread]
	    set Priv(seconds,$i) [expr {[clock seconds] - int(rand() * 500000)}]
	    set Priv(seconds2,$i) $Priv(seconds,$i)
	    set Priv(count,$thread) 1
	} else {
	    set Priv(thread,$i) $Priv(thread,$j)
	    set Priv(seconds,$i) [expr {$Priv(seconds2,$j) + int(rand() * 10000)}]
	    set Priv(seconds2,$i) $Priv(seconds,$i)
	    set Priv(seconds2,$j) $Priv(seconds,$i)
	    incr Priv(count,$Priv(thread,$j))
	}
    }

    for {set i 1} {$i < $msgCnt} {incr i} {
	set subject "This is thread number $Priv(thread,$i)"
	set from somebody@somewhere.net
	set sent [clock format $Priv(seconds,$i) -format "%d/%m/%y %I:%M %p"]
	set size [expr {1 + int(rand() * 10)}]KB
	if {$Priv(read,$i)} {
	    set style read
	    set style2 read
	} else {
	    set style unread
	    set style2 unread
	}
	$T item style set $i 3 $style 4 $style2.we 5 $style2.we 6 $style2.w
	$T item text $i 3 $subject 4 $from 5 $sent 6 $size
	if {[$T item numchildren $i]} {
	    $T item configure $i -button yes
	}
    }

    $T notify bind $T <Selection> {
	if {[%T selection count] == 1} {
	    set I [%T selection get 0]
	    if {!$Priv(read,$I)} {
		if {[%T item isopen $I] || ![AnyUnreadDescendants %T $I]} {
		    # unread ->read
		    %T item style map $I subject read {text.unread text.read}
		    %T item style map $I from read.we {text.unread text.read}
		    %T item style map $I sent read.we {text.unread text.read}
		    %T item style map $I size read.w {text.unread text.read}
		} else {
		    # unread -> read2
		    %T item style map $I subject read2 {text.unread text.unread}
		}
		set Priv(read,$I) 1
		DisplayStylesInItem $I
	    }
	}
    }

    $T notify bind $T <Expand-after> {
	if {$Priv(read,%I) && [AnyUnreadDescendants %T %I]} {
	    # read2 -> read
	    %T item style map %I subject read {text.unread text.read}
	    # unread -> read
	    %T item style map %I from read.we {text.unread text.read}
	    %T item style map %I sent read.we {text.unread text.read}
	    %T item style map %I size read.w {text.unread text.read}
	}
    }

    $T notify bind $T <Collapse-after> {
	if {$Priv(read,%I) && [AnyUnreadDescendants %T %I]} {
	    # read -> read2
	    %T item style map %I subject read2 {text.read text.unread}
	    # read -> unread
	    %T item style map %I from unread.we {text.read text.unread}
	    %T item style map %I sent unread.we {text.read text.unread}
	    %T item style map %I size unread.w {text.read text.unread}
	}
    }

    for {set i 1} {$i < $msgCnt} {incr i} {
	if {rand() * 2 > 1} {
	    if {[$T item numchildren $i]} {
		$T item collapse $i
	    }
	}
    }

    return
}

proc DemoOutlookNewsgroup::AnyUnreadDescendants {T I} {

    variable Priv

    foreach item [$T item descendants $I] {
	if {!$Priv(read,$item)} {
	    return 1
	}
    }
    return 0
}

proc DemoOutlookNewsgroup::FixItemStyles {T} {

    set columns1 [$T column id "visible tag clip||arrow||watch !tail"]
    set columns2 [$T column id "visible tag !(clip||arrow||watch) !tail"]

    foreach C [$T column id "visible !tail"] {

	# The clip/arrow/watch columns only get a style when they are
	# between the first and last text-containing columns.
	if {[lsearch -exact $columns1 $C] != -1} {
	    if {[$T column compare $C > [lindex $columns2 0]] &&
		[$T column compare $C < [lindex $columns2 end]]} {
		$T item style set all $C s2
		$T item state forcolumn all $C {!openW !openE openWE}
	    } else {
		$T item style set all $C ""
	    }
	    continue
	}

	# The text-containing columns need their styles set such that the
	# active outline of the selected item extends from left to right.
	# Also, the left-most text-containing column is the tree column
	# and displays the icon.
	if {$C eq [lindex $columns2 0]} {
	    $T configure -treecolumn $C
	    set S s1
	    set state openE
	} elseif {$C eq [lindex $columns2 end]} {
	    set S s2
	    set state openW
	} else {
	    set S s2
	    set state openWE
	}
	$T item state forcolumn all $C [list !openWE !openE !openW $state]

	# Change the style, but keep the text so we don't have to reset it.
	$T item style map all $C $S {elemTxt elemTxt}
    }
    return
}
