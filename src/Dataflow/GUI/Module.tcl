#  The contents of this file are subject to the University of Utah Public
#  License (the "License"); you may not use this file except in compliance
#  with the License.
#  
#  Software distributed under the License is distributed on an "AS IS"
#  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
#  License for the specific language governing rights and limitations under
#  the License.
#  
#  The Original Source Code is SCIRun, released March 12, 2001.
#  
#  The Original Source Code was developed by the University of Utah.
#  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
#  University of Utah. All Rights Reserved.
#

global CurrentlySelectedModules
set CurrentlySelectedModules ""

global startX
set startX 0

global startY
set startY 0

global undoList
set undoList ""

global redoList
set redoList ""

itcl_class Module {
   
    method modname {} {
	return [string range $this [expr [string last :: $this] + 2] end]
    }
			
    constructor {config} {
	global $this-gui-x $this-gui-y
        set $this-gui-x -1
        set $this-gui-y -1

	set msgLogStream "[SciTclStream msgLogStream#auto]"
	# these live in parallel temporarily
	global $this-notes Notes
	if ![info exists $this-notes] { set $this-notes "" }
	trace variable $this-notes w "syncNotes [modname]"
	
	# messages should be accumulating
	if {[info exists $this-msgStream]} {
	    $msgLogStream registerVar $this-msgStream
	}
    }
    
    destructor {
	set w .mLogWnd[modname]
	if [winfo exists $w] {
	    destroy $w
	}
	$msgLogStream destructor
	eval unset [info vars $this-*]
	destroy $this
    }
    
    public msgLogStream
    public name
    protected make_progress_graph 1
    protected make_time 1
    protected graph_width 50
    protected old_width 0
    protected indicator_width 15
    protected initial_width 0
    # flag set when the module is compiling
    protected compiling_p 0
    # flag set when the module has all incoming ports blocked
    public state "NeedData" {$this update_state}
    public msg_state "Reset" {$this update_msg_state}
    public progress 0 {$this update_progress}
    public time "00.00" {$this update_time}
    public isSubnetModule 0
    public subnetNumber 0

    method get_msg_state {} {
	return $msg_state
    }

    method compiling_p {} { return $compiling_p }
    method set_compiling_p { val } { 
	set compiling_p $val	
	setColorAndTitle
        if {[winfo exists .standalone]} {
	    app indicate_dynamic_compile [modname] [expr $val?"start":"stop"]
	}
    }

    method name {} {
	return $name
    }
    
    method set_state {st t} {
	set state $st
	set time $t
	update_state
	update_time
	update idletasks
    }

    method set_msg_state {st} {
	set msg_state $st
	update_msg_state
	update idletasks
    }

    method set_progress {p t} {
	set progress $p
	set time $t
	update_progress
	update_time
	update idletasks
    }

    method set_title {name} {
	# index points to the second "_" in the name, after package & category
	set index [string first "_" $name [expr [string first "_" $name 0]+1]]
	return [string range $name [expr $index+1] end]
    }

    method initialize_ui { {my_display "local"} } {
        $this ui
	if {[winfo exists .ui[modname]]!= 0} {
	    set w .ui[modname]

	    if { [winfo ismapped $w] == 1} {
		raise $w
	    } else {
		wm deiconify $w
	    }

	    # Mac Hac to keep GUI windows from "growing..."  Hopefully
	    # a TCL fix will come out for this soon and we can remove
	    # the following line: (after 1 "wm geometry $w {}") Note:
	    # this forces the GUIs to resize themselves to their
	    # "best" size.  However, if the user has resized the GUI
	    # for some reason, then this will resize it back to the
	    # "original" size.  Perhaps not the best behavior. :-(
	    after 1 "wm geometry $w {}"

	    wm title $w [set_title [modname]]
	}
    }

    # Brings the UI window to near the mouse.
    method fetch_ui {} {
	set w .ui[modname]
	# Mac window manager top window bar height is 22 pixels (at
	# least on my machine.)  Because the 'winfo y' command does
	# not take this into account (at least on the Mac, need to
	# check PC), we need to do so explicitly. -Dav
	set wm_border_height 22
	if {[winfo exists $w] != 0} {
	    global $this-gui-x $this-gui-y
	    set $this-gui-x [winfo x $w]
	    set $this-gui-y [expr [winfo y $w] - $wm_border_height]
	    moveToCursor $w
	    # Raise the window
	    initialize_ui
	}
    }

    # Returns the UI window to where the user had originally placed it.
    method return_ui {} {
	set w .ui[modname]
	if {[winfo exists $w] != 0} {
            global $this-gui-x $this-gui-y
	    wm geometry $w +[set $this-gui-x]+[set $this-gui-y]
	    # Raise the window
	    initialize_ui
	}
    }

    method have_ui {} {
	return [llength [$this info method ui]]
    }

    #  Make the modules icon on a particular canvas
    method make_icon {modx mody { ignore_placement 0 } } {
	global $this-done_bld_icon Disabled Subnet Color ToolTipText
	set $this-done_bld_icon 0
	set Disabled([modname]) 0
	set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	set minicanvas $Subnet(Subnet$Subnet([modname])_minicanvas)
	
	set modframe $canvas.module[modname]
	frame $modframe -relief raised -borderwidth 3 
	
	bind $modframe <1> "moduleStartDrag [modname] %X %Y 0"
	bind $modframe <B1-Motion> "moduleDrag [modname] %X %Y"
	bind $modframe <ButtonRelease-1> "moduleEndDrag [modname] %X %Y"
	bind $modframe <Control-Button-1> "moduleStartDrag [modname] %X %Y 1"
	bind $modframe <3> "moduleMenu %X %Y [modname]"
	
	frame $modframe.ff
	set p $modframe.ff
	pack $p -side top -expand yes -fill both -padx 5 -pady 6

	global ui_font modname_font time_font
	if {[have_ui]} {
	    button $p.ui -text "UI" -borderwidth 2 -font $ui_font \
		-anchor center -command "$this initialize_ui"
	    pack $p.ui -side left -ipadx 5 -ipady 2
	    Tooltip $p.ui $ToolTipText(ModuleUI)
	}

	# Make the title
	label $p.title -text "$name" -font $modname_font -anchor w
	pack $p.title -side [expr $make_progress_graph?"top":"left"] \
	    -padx 2 -anchor w
	bind $p.title <Map> "$this setDone"

	# Make the time label
	if {$make_time} {
	    label $p.time -text "00.00" -font $time_font
	    pack $p.time -side left -padx 2
	    Tooltip $p.time $ToolTipText(ModuleTime)
	}

	# Make the progress graph
	if {$make_progress_graph} {
	    frame $p.inset -relief sunken -height 4 \
		-borderwidth 2 -width $graph_width
	    pack $p.inset -side left -fill y -padx 2 -pady 2
	    frame $p.inset.graph -relief raised \
		-width 0 -borderwidth 2 -background green
	    Tooltip $p.inset $ToolTipText(ModuleProgress)
	}


	# Make the message indicator
	frame $p.msg -relief sunken -height 15 -borderwidth 1 \
	    -width [expr $indicator_width+2]
	pack $p.msg -side [expr $make_progress_graph?"left":"right"] \
	    -padx 2 -pady 2
	frame $p.msg.indicator -relief raised -width 0 -height 0 \
	    -borderwidth 2 -background blue
	bind $p.msg.indicator <Button> "$this displayLog"
	Tooltip $p.msg.indicator $ToolTipText(ModuleMessageBtn)

	update_msg_state
	update_progress
	update_time

	# compute the placement of the module icon
	if { !$ignore_placement } {
	    set pos [findModulePosition $Subnet([modname]) $modx $mody]
	} else {
	    set pos [list $modx $mody]
	}
	
	set pos [eval clampModuleToCanvas $pos]
	
	# Stick it in the canvas
	$canvas create window [lindex $pos 0] [lindex $pos 1] -anchor nw \
	    -window $modframe -tags "module [modname]"

	set pos [scalePath $pos]
	$minicanvas create rectangle [lindex $pos 0] [lindex $pos 1] \
	    [expr [lindex $pos 0]+4] [expr [lindex $pos 1]+2] \
	    -outline "" -fill $Color(Basecolor) -tags "module [modname]"

	# Create, draw, and bind all input and output ports
	drawPorts [modname]
	
	# create the Module Menu
	menu $p.menu -tearoff false -disabledforeground white

	Tooltip $p $ToolTipText(Module)

	bindtags $p [linsert [bindtags $p] 1 $modframe]
	bindtags $p.title [linsert [bindtags $p.title] 1 $modframe]
	if {$make_time} {
	    bindtags $p.time [linsert [bindtags $p.time] 1 $modframe]
	}
	if {$make_progress_graph} {
	    bindtags $p.inset [linsert [bindtags $p.inset] 1 $modframe]
	}

	# If we are NOT currently running a script... ie, not loading the net
	# from a file
	if ![string length [info script]] {
	    unselectAll
	    global CurrentlySelectedModules
	    set CurrentlySelectedModules "[modname]"
	}
	
	fadeinIcon [modname]
    }
    # end make_icon
    
    method setColorAndTitle { { color "" } args} {
	global Subnet Color Disabled
	set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	set m $canvas.module[modname]
	if ![string length $color] {
	    set color $Color(Basecolor)
	    if { [$this is_selected] } { set color $Color(Selected) }
	    setIfExists disabled Disabled([modname]) 0
	    if { $disabled } { set color [blend $color $Color(Disabled)] }
	}
	if { $compiling_p } { set color $Color(Compiling) }
	if { ![llength $args] && [isaSubnetIcon [modname]] } {
	    set args $Subnet(Subnet$Subnet([modname]_num)_Name)
	}
	$m configure -background $color
	$m.ff configure -background $color
	if {[$this have_ui]} { $m.ff.ui configure -background $color }
	if {$make_time} { $m.ff.time configure -background $color }
	if {$isSubnetModule} { $m.ff.subnet configure -background $color }
	if {![llength $args]} { set args $name }
	$m.ff.title configure -text "$args" -justify left -background $color
	if {$isSubnetModule} { 
	    $m.ff.type configure -text $Subnet(Subnet$Subnet([modname]_num)_Instance) \
		-justify left -background $color
	}
	    
    }
       
    method addSelected {} {
	if {![$this is_selected]} { 
	    global CurrentlySelectedModules
	    lappend CurrentlySelectedModules [modname]
	    setColorAndTitle
	}
    }    

    method removeSelected {} {
	if {[$this is_selected]} {
	    #Remove me from the Currently Selected Module List
	    global CurrentlySelectedModules
	    listFindAndRemove CurrentlySelectedModules [modname]
	    setColorAndTitle
	}
    }
    
    method toggleSelected {} {
	if [is_selected] removeSelected else addSelected
    }
    
    method update_progress {} {
	set width [expr int($progress*($graph_width-4))]
	if {!$make_progress_graph || $width == $old_width } return
	global Subnet
	set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	set graph $canvas.module[modname].ff.inset.graph
	if {$width == 0} { 
	    place forget $graph
	} else {
	    $graph configure -width $width
	    if {$old_width == 0} { place $graph -relheight 1 -anchor nw }
	}
	set old_width $width
    }
	
    method update_time {} {
	if {!$make_time} return

	if {$state == "JustStarted"} {
	    set tstr " ?.??"
	} else {
	    if {$time < 60} {
		set secs [expr int($time)]
		set frac [expr int(100*($time-$secs))]
		set tstr [format "%2d.%02d" $secs $frac]
	    } elseif {$time < 3600} {
		set mins [expr int($time/60)]
		set secs [expr int($time-$mins*60)]
		set tstr [format "%2d:%02d" $mins $secs]
	    } else {
		set hrs [expr int($time/3600)]
		set mins [expr int($time-$hrs*3600)]
		set tstr [format "%d::%02d" $hrs $mins]
	    }
	}
	global Subnet
	set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	$canvas.module[modname].ff.time configure -text $tstr
    }

    method update_state {} {
	if {!$make_progress_graph} return
	if {$state == "JustStarted 1123"} {
	    set progress 0.5
	    set color red
	} elseif {$state == "Executing"} {
	    set progress 0
	    set color red
	} elseif {$state == "NeedData"} {
	    set progress 1
	    set color yellow
	} elseif {$state == "Completed"} {
	    set progress 1
	    set color green
	} else {
	    set width 0
	    set color grey75
	    set progress 0
	}

	if {[winfo exists .standalone]} {
	    app update_progress [modname] $state
	}
	
	global Subnet
	set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	$canvas.module[modname].ff.inset.graph configure -background $color
	update_progress
    }

    method update_msg_state {} { 
	if {$msg_state == "Error"} {
	    set p 1
	    set color red
	} elseif {$msg_state == "Warning"} {
	    set p 1
	    set color yellow
	} elseif {$msg_state == "Remark"} {
	    set p 1
	    set color blue
	}  elseif {$msg_state == "Reset"} {
	    set p 1
	    set color grey75
	} else {
	    set p 0
	    set color grey75
	}
	global Subnet
	set number $Subnet([modname])
	set canvas $Subnet(Subnet${number}_canvas)
	set indicator $canvas.module[modname].ff.msg.indicator
	place forget $indicator
	$indicator configure -width $indicator_width -background $color
	place $indicator -relheight 1 -anchor nw 

	if $number {
	    SubnetIcon$number update_msg_state
	}

	if {[winfo exists .standalone]} {
	    app indicate_error [modname] $msg_state
	}
	
    }

    method get_x {} {
	global Subnet
	set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	return [lindex [$canvas coords [modname]] 0]
    }

    method get_y {} {
	global Subnet
	set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	return [lindex [$canvas coords [modname]] 1]
    }

    method is_selected {} {
	global CurrentlySelectedModules
	return [expr ([lsearch $CurrentlySelectedModules [modname]]!=-1)?1:0]
    }

    method displayLog {} {
	if [$this is_subnet] return
	set w .mLogWnd[modname]
	
	if {[winfo exists $w]} {
	    if { [winfo ismapped $w] == 1} {
		raise $w
	    } else {
		wm deiconify $w
	    }
	    return
	}
	
	# create the window (immediately withdraw it to avoid flicker)
	toplevel $w; wm withdraw $w
	update

	append t "Log for " [modname]
	set t "$t -- pid=[$this-c getpid]"
	wm title $w $t
	
	frame $w.log
	# Create the txt in the "disabled" state so that a user cannot type into the text field.
	# Also, must give it a width and a height so that it will be allowed to automatically
	# change the width and height (go figure?).
	text $w.log.txt -relief sunken -bd 2 -yscrollcommand "$w.log.sb set" -state disabled \
	    -height 2 -width 10
	scrollbar $w.log.sb -relief sunken -command "$w.log.txt yview"
	pack $w.log.txt -side left -padx 5 -pady 2 -expand 1 -fill both
	pack $w.log.sb -side left -padx 0 -pady 2 -fill y

	frame $w.fbuttons 
	# TODO: unregister only for streams with the supplied output
	button $w.fbuttons.clear -text "Clear" -command "$this clearStreamOutput"
	button $w.fbuttons.ok    -text "Close" -command "wm withdraw $w"
	
	Tooltip $w.fbuttons.ok "Close this window.  The log is not effected."
	TooltipMultiline $w.fbuttons.clear \
            "If the log indicates anything but an error, the Clear button\n" \
            "will clear the message text from the log and reset the warning\n" \
            "indicator to No Message.  Error messages cannot be cleared -- you\n" \
	    "must fix the problem and re-execute the module."

	pack $w.fbuttons.clear $w.fbuttons.ok -side left -expand yes -fill both \
	    -padx 5 -pady 5 -ipadx 3 -ipady 3
	pack $w.log      -side top -padx 5 -pady 2 -fill both -expand 1
	pack $w.fbuttons -side top -padx 5 -pady 2 -fill x

	wm minsize $w 450 150

	# Move window to cursor after it has been created.
	moveToCursor $w "leave_up"

	$msgLogStream registerOutput $w.log.txt
    }

    method clearStreamOutput { } {
	# Clear the text widget 
	$msgLogStream clearTextWidget

	# Clear the module indicator color if
        # not in an error state
	if {$msg_state != "Error"} {
	    set_msg_state "Reset"
	}
    }
    
    method destroyStreamOutput {w} {
	# TODO: unregister only for streams with the supplied output
	$msgLogStream unregisterOutput
	destroy $w
    }


    method resize_icon {} {
	set iports [portCount "[modname] 0 i"]
	set oports [portCount "[modname] 0 o"]
	set nports [expr $oports>$iports?$oports:$iports]
	if {[set $this-done_bld_icon]} {
	    global Subnet port_spacing
	    set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	    set width [expr 8+$nports*$port_spacing] 
	    set width [expr ($width < $initial_width)?$initial_width:$width]
	    $canvas itemconfigure [modname] -width $width
	    #$canvas create window [lindex $pos 0] [lindex $pos 1] -anchor
	    #-window $m -tags "module [modname]"

	}
    }

    method setDone {} {
	#module actually mapped to the canvas
	if ![set $this-done_bld_icon] {
	    set $this-done_bld_icon 1	
	    global Subnet
	    set canvas $Subnet(Subnet$Subnet([modname])_canvas)
	    set initial_width [winfo width $canvas.module[modname]]
	    resize_icon
	    drawNotes [modname]
	    drawConnections [portConnections "[modname] all o"]
	}
    }

    method execute {} {
	$this-c needexecute
    }
    
    method is_subnet {} {
	return $isSubnetModule
    }
	
}   

proc fadeinIcon { modid { seconds 0.333 } { center 0 }} {
    if [llength [info script]] {
	$modid setColorAndTitle
	return
    }
    global Color Subnet FadeInID
    if $center {
	set canvas $Subnet(Subnet$Subnet($modid)_canvas)
	set bbox [$canvas bbox $modid]
	if { [lindex $bbox 0] < [$canvas canvasx 0] ||
	     [lindex $bbox 1] < [$canvas canvasy 0] ||
	     [lindex $bbox 2] > [$canvas canvasy [winfo width $canvas]] ||
	     [lindex $bbox 3] > [$canvas canvasy [winfo height $canvas]] } {
	    set modW [expr [lindex $bbox 2] - [lindex $bbox 0]]
	    set modH [expr [lindex $bbox 3] - [lindex $bbox 1]]
	    set canScroll [$canvas cget -scrollregion]
	    set x [expr [lindex $bbox 0] - ([winfo width  $canvas] - $modW)/2]
	    set y [expr [lindex $bbox 1] - ([winfo height $canvas] - $modH)/2]
	    set x [expr $x/([lindex $canScroll 2] - [lindex $canScroll 0])]
	    set y [expr $y/([lindex $canScroll 3] - [lindex $canScroll 1])]
	    $canvas xview moveto $x
	    $canvas yview moveto $y
	}
    }

    set frequency 24
    set period [expr double(1000.0/$frequency)]
    set t $period
    set stopAt [expr double($seconds*1000.0)]
    set dA [expr double(1.0/($seconds*$frequency))]
    set alpha $dA
	    
    if [info exists FadeInID($modid)] {
	foreach id $FadeInID($modid) {
	    after cancel $id
	}
    }
    set FadeInID($modid) ""

    $modid setColorAndTitle $Color(IconFadeStart)
    while { $t < $stopAt } {
	set color [blend $Color(Selected) $Color(IconFadeStart) $alpha]
	lappend FadeInID($modid) [after [expr int($t)] "$modid setColorAndTitle $color"]
	set alpha [expr double($alpha+$dA)]
	set t [expr double($t+$period)]
    }
    lappend FadeInID($modid) [after [expr int($t)] "$modid setColorAndTitle"]
}
	
proc moduleMenu {x y modid} {
    global Subnet mouseX mouseY
    set mouseX $x
    set mouseY $y
    set canvas $Subnet(Subnet$Subnet($modid)_canvas)
    set menu_id "$canvas.module$modid.ff.menu"
    regenModuleMenu $modid $menu_id
    tk_popup $menu_id $x $y    
}

proc regenModuleMenu { modid menu_id } {
    # Wipe the menu clean...
    set num_entries [$menu_id index end]
    if { $num_entries == "none" } { 
	set num_entries 0
    }
    for {set c 0} {$c <= $num_entries} {incr c } {
	$menu_id delete 0
    }

    global Subnet Disabled CurrentlySelectedModules
    $menu_id add command -label "$modid" -state disabled
    $menu_id add separator

    # 'Execute Menu Option
    $menu_id add command -label "Execute" -command "$modid execute"
    
    # 'Help' Menu Option
    if { ![$modid is_subnet] } {
	$menu_id add command -label "Help" -command "moduleHelp $modid"
    }
    
    # 'Notes' Menu Option
    $menu_id add command -label "Notes" \
	-command "notesWindow $modid notesDoneModule"

    # 'Destroy Selected' Menu Option
    if { [$modid is_selected] } { 
	$menu_id add command -label "Destroy Selected" \
	    -command "moduleDestroySelected"
    }

    # 'Destroy' module Menu Option
    $menu_id add command -label "Destroy" -command "moduleDestroy $modid"
    
    # 'Show Log' module Menu Option
    if {![$modid is_subnet]} {
	$menu_id add command -label "Show Log" -command "$modid displayLog"
    }
    
    # 'Make Sub-Network' Menu Option
    set mods [expr [$modid is_selected]?"$CurrentlySelectedModules":"$modid"]
    $menu_id add command -label "Make Sub-Network" \
	-command "createSubnetFromModules $mods"
    
    # 'Expand Sub-Network' Menu Option
    if { [$modid is_subnet] } {
	$menu_id add command -label "Expand Sub-Network" \
	    -command "expandSubnet $modid"
    }

    # 'Enable/Disable' Menu Option
    if {[llength $Subnet(${modid}_connections)]} {
	setIfExists disabled Disabled($modid) 0
	if $disabled {
	    $menu_id add command -label "Enable" \
		-command "disableModule $modid 0"
	} else {
	    $menu_id add command -label "Disable" \
		-command "disableModule $modid 1"
	}
    }

    # 'Fetch/Return UI' Menu Option
    if { [envBool SCIRUN_GUI_UseGuiFetch] } {
        $menu_id add separator
	$menu_id add command -label "Fetch UI"  -command "$modid fetch_ui"
	$menu_id add command -label "Return UI" -command "$modid return_ui"
    }
}


proc notesDoneModule { id } {
    global $id-notes Notes
    if { [info exists Notes($id)] } {
	set $id-notes $Notes($id)
    }
}

proc notesWindow { id {done ""} } {
    global Notes Color Subnet
    set w .notes$id
    if { [winfo exists $w] } { destroy $w }
    setIfExists cache Notes($id) ""
    toplevel $w
    wm title $w $id
    text $w.input -relief sunken -bd 2 -height 20
    bind $w.input <KeyRelease> \
	"set Notes($id) \[$w.input get 1.0 \"end - 1 chars\"\]"
    frame $w.b
    button $w.b.done -text "Done" \
	-command "okNotesWindow $id \"$done\""
    button $w.b.clear -text "Clear" -command "$w.input delete 1.0 end; set Notes($id) {}"
    button $w.b.cancel -text "Cancel" -command \
	"set Notes($id) \"$cache\"; destroy $w"

    setIfExists rgb Color($id) white
    button $w.b.reset -fg black -text "Reset Color" -command \
	"unset Notes($id-Color); $w.b.color configure -bg $rgb"

    setIfExists rgb Notes($id-Color) $rgb
    button $w.b.color -fg black -bg $rgb -text "Text Color" -command \
	"colorNotes $id"

    frame $w.d -relief groove -borderwidth 2

    setIfExists Notes($id-Position) Notes($id-Position) def

    set radiobuttons { {"Default" def} {"None" none} {"Tooltip" tooltip} {"Top" n} }
    if [info exists Subnet($id)] {
	lappend radiobuttons {"Left" w}
	lappend radiobuttons {"Right" e}
	lappend radiobuttons {"Bottom" s}
    }
	
    make_labeled_radio $w.d.pos "Display:" "" left Notes($id-Position) $radiobuttons

    pack $w.input -fill x -side top -padx 5 -pady 3
    pack $w.d -fill x -side top -padx 5 -pady 0
    pack $w.d.pos
    pack $w.b -fill y -side bottom -pady 3
    pack $w.b.done $w.b.clear $w.b.cancel $w.b.reset \
	$w.b.color -side right -padx 5 -pady 5 -ipadx 3 -ipady 3
	    
    if [info exists Notes($id)] {$w.input insert 1.0 $Notes($id)}
}

proc colorNotes { id } {
    global Notes
    set w .notes$id
    networkHasChanged
    $w.b.color configure -bg [set Notes($id-Color) \
       [tk_chooseColor -initialcolor [$w.b.color cget -bg]]]
}

proc okNotesWindow {id {done  ""}} {
    destroy .notes$id
    if { $done != ""} { eval $done $id }
}

proc disableModule { module state } {
    global Disabled CurrentlySelectedModules Subnet
    set mods [expr [$module is_selected]?"$CurrentlySelectedModules":"$module"]
    foreach modid $mods { ;# Iterate through the modules
	foreach conn $Subnet(${modid}_connections) { ;# all module connections
	    setIfExists disabled Disabled([makeConnID $conn]) 0
	    if { $disabled != $state } {
		disableConnection $conn
	    }
	}
    }
}

proc checkForDisabledModules { args } {
    global Disabled Subnet
    set args [lsort -unique $args]
    foreach modid $args {
	if [isaSubnetEditor $modid] continue;
	# assume module is disabled
	set Disabled($modid) 1
	foreach conn $Subnet(${modid}_connections) {
	    # if connection is enabled, then enable module
	    setIfExists disabled Disabled([makeConnID $conn]) 0
	    if { !$disabled } {
		set Disabled($modid) 0
		# module is enabled, continue onto next module
		break;
	    }
	}
	if {![llength $Subnet(${modid}_connections)]} {set Disabled($modid) 0}
	$modid setColorAndTitle
    }
}


proc canvasExists { canvas arg } {
    return [expr [llength [$canvas find withtag $arg]]?1:0]
}


proc shadow { pos } {
    return [list [expr 1+[lindex $pos 0]] [expr 1+[lindex $pos 1]]]
}

proc scalePath { path } {
    set opath ""
    global SCALEX SCALEY
    foreach pt $path {
	lappend opath [expr round($pt/(([llength $opath]%2)?$SCALEY:$SCALEX))]
    }
    return $opath
}
    

proc getModuleNotesOptions { module } {
    global Subnet Notes
    set bbox [$Subnet(Subnet$Subnet($module)_canvas) bbox $module]
    set off 2
    set xCenter [expr ([lindex $bbox 0]+[lindex $bbox 2])/2]
    set yCenter [expr ([lindex $bbox 1]+[lindex $bbox 3])/2]
    set left    [expr [lindex $bbox 0] - $off]
    set right   [expr [lindex $bbox 2] + $off]
    setIfExists pos Notes($module-Position) def
    switch $pos {
	n { return [list $xCenter [lindex $bbox 1] -anchor s -justify center] }
	s { return [list $xCenter [lindex $bbox 3] -anchor n -justify center] }
	w { return [list $left $yCenter -anchor e -justify right] }
	# east is default
	default {  return [list $right $yCenter	-anchor w -justify left]
	}

    }
}




global ignoreModuleMove 
set ignoreModuleMove 1

proc moduleStartDrag {modid x y toggleOnly} {
    global ignoreModuleMove CurrentlySelectedModules redrawConnectionList
    set ignoreModuleMove 0
    if $toggleOnly {
	$modid toggleSelected
	set ignoreModuleMove 1
	return
    }

    global Subnet startX startY lastX lastY
    set canvas $Subnet(Subnet$Subnet($modid)_canvas)

    #raise the module icon
    raise $canvas.module$modid

    #set module movement coordinates
    set lastX $x
    set lastY $y
    set startX $x
    set startY $y
       
    #if clicked module isnt selected, unselect all and select this
    if { ![$modid is_selected] } { 
	unselectAll
	$modid addSelected
    }

    #build a connection list of all selected modules to draw conns when moving
    set redrawConnectionList ""
    foreach csm $CurrentlySelectedModules {
	eval lappend redrawConnectionList $Subnet(${csm}_connections)
    }
    set redrawConnectionList [lsort -unique $redrawConnectionList]
    
    #create a gray bounding box around moving modules
    if {[llength $CurrentlySelectedModules] > 1} {
	set bbox [compute_bbox $canvas]
	$canvas create rectangle  $bbox -outline black -tags tempbox
    }
}

proc moduleDrag {modid x y} {
    global ignoreModuleMove CurrentlySelectedModules redrawConnectionList
    if $ignoreModuleMove return
    global Subnet grouplastX grouplastY lastX lastY
    set canvas $Subnet(Subnet$Subnet($modid)_canvas)
    set bbox [compute_bbox $canvas]
    # When the user tries to drag a group of modules off the canvas,
    # Offset the lastX and or lastY variable, so that they can only drag
    #groups to the border of the canvas
    set min_possibleX [expr [lindex $bbox 0] + $x - $lastX]
    set min_possibleY [expr [lindex $bbox 1] + $y - $lastY]    
    if {$min_possibleX <= 0} { set lastX [expr [lindex $bbox 0] + $x] } 
    if {$min_possibleY <= 0} { set lastY [expr [lindex $bbox 1] + $y] }
    set max_possibleX [expr [lindex $bbox 2] + $x - $lastX]
    set max_possibleY [expr [lindex $bbox 3] + $y - $lastY]
    if {$max_possibleX >= 4500} { set lastX [expr [lindex $bbox 2]+$x-4500] }
    if {$max_possibleY >= 4500} { set lastY [expr [lindex $bbox 3]+$y-4500] }

    # Move each module individually
    foreach csm $CurrentlySelectedModules {
	do_moduleDrag $csm $x $y
    }	
    set lastX $grouplastX
    set lastY $grouplastY
    # redraw connections between moved modules
    drawConnections $redrawConnectionList
    # move the bounding selection rectangle
    $canvas coords tempbox [compute_bbox $canvas]
}    

proc do_moduleDrag {modid x y} {
    networkHasChanged
    global Subnet lastX lastY grouplastX grouplastY SCALEX SCALEY
    set canvas $Subnet(Subnet$Subnet($modid)_canvas)
    set minicanvas $Subnet(Subnet$Subnet($modid)_minicanvas)

    set grouplastX $x
    set grouplastY $y
    set bbox [$canvas bbox $modid]
    
    # Canvas Window width and height
    set width  [winfo width  $canvas]
    set height [winfo height $canvas]

    # Total Canvas Scroll Region width and height
    set canScroll [$canvas cget -scrollregion]
    set canWidth  [expr double([lindex $canScroll 2] - [lindex $canScroll 0])]
    set canHeight [expr double([lindex $canScroll 3] - [lindex $canScroll 1])]
        
    # Cursor movement delta from last position
    set dx [expr $x - $lastX]
    set dy [expr $y - $lastY]

    # if user attempts to drag module off left edge of canvas
    set modx [lindex $bbox 0]
    set left [$canvas canvasx 0] 
    if { [expr $modx+$dx] <= $left } {
	if { $left > 0 } {
	    $canvas xview moveto [expr ($modx+$dx)/$canWidth]
	}
	if { [expr $modx+$dx] <= 0 } {
	    $canvas move $modid [expr -$modx] 0
	    $minicanvas move $modid [expr (-$modx)/$SCALEX] 0
	    set dx 0
	}
    }
    
    #if user attempts to drag module off right edge of canvas
    set modx [lindex $bbox 2]
    set right [$canvas canvasx $width] 
    if { [expr $modx+$dx] >= $right } {
	if { $right < $canWidth } {
	    $canvas xview moveto [expr ($modx+$dx-$width)/$canWidth]
	}
	if { [expr $modx+$dx] >= $canWidth } {
	    $canvas move $modid [expr $canWidth-$modx] 0
	    $minicanvas move $modid [expr ($canWidth-$modx)/$SCALEX] 0
	    set dx 0
	} 
    }
    
    #if user attempts to drag module off top edge of canvas
    set mody [lindex $bbox 1]
    set top [$canvas canvasy 0]
    if { [expr $mody+$dy] <= $top } {
	if { $top > 0 } {
	    $canvas yview moveto [expr ($mody+$dy)/$canHeight]
	}    
	if { [expr $mody+$dy] <= 0 } {
	    $canvas move $modid 0 [expr -$mody]
	    $minicanvas move $modid 0 [expr (-$mody)/$SCALEY]
	    set dy 0
	}
    }
 
    #if user attempts to drag module off bottom edge of canvas
    set mody [lindex $bbox 3]
    set bottom [$canvas canvasy $height]
    if { [expr $mody+$dy] >= $bottom } {
	if { $bottom < $canHeight } {
	    $canvas yview moveto [expr ($mody+$dy-$height)/$canHeight]
	}	
	if { [expr $mody+$dy] >= $canHeight } {
	    $canvas move $modid 0 [expr $canHeight-$mody]
	    $minicanvas move $modid 0 [expr ($canHeight-$mody)/$SCALEY]
	    set dy 0
	}
    }

    # X and Y coordinates of canvas origin
    set Xbounds [winfo rootx $canvas]
    set Ybounds [winfo rooty $canvas]
    set currx [expr $x-$Xbounds]

    #cursor-boundary check and warp for x-axis
    if { [expr $x-$Xbounds] > $width } {
	cursor warp $canvas $width [expr $y-$Ybounds]
	set currx $width
	set scrollwidth [.bot.neteditFrame.vscroll cget -width]
	set grouplastX [expr $Xbounds + $width - 5 - $scrollwidth]
    }
    if { [expr $x-$Xbounds] < 0 } {
	cursor warp $canvas 0 [expr $y-$Ybounds]
	set currx 0
	set grouplastX $Xbounds
    }
    
    #cursor-boundary check and warp for y-axis
    if { [expr $y-$Ybounds] > $height } {
	cursor warp $canvas $currx $height
	set scrollwidth [.bot.neteditFrame.hscroll cget -width]
	set grouplastY [expr $Ybounds + $height - 5 - $scrollwidth]
    }
    if { [expr $y-$Ybounds] < 0 } {
	cursor warp $canvas $currx 0
	set grouplastY $Ybounds
    }
    
    # if there is no movement to perform, then return
    if {!$dx && !$dy} { return }
    
    # Perform the actual move of the module window
    $canvas move $modid $dx $dy
    $minicanvas move $modid [expr $dx / $SCALEX ] [expr $dy / $SCALEY ]
    
    drawNotes $modid
}


proc drawNotes { args } {
    global Subnet Color Notes Font ToolTipText modname_font
    set Font(Notes) $modname_font

    foreach id $args {
	setIfExists position Notes($id-Position) def	
	setIfExists isModuleNotes  0
	setIfExists color Color($id) white
	setIfExists color Notes($id-Color) $color
	setIfExists text Notes($id) ""

	set isModuleNotes 0 
	if [info exists Subnet($id)] {
	    set isModuleNotes 1
	}
	
	if $isModuleNotes {
	    set subnet $Subnet($id)
	} else {
	    set subnet $Subnet([lindex [parseConnectionID $id] 0])
	}
	set canvas $Subnet(Subnet${subnet}_canvas)

	if {$position == "tooltip"} {
	    if { $isModuleNotes } {
		Tooltip $canvas.module$id $text
	    } else {
		canvasTooltip $canvas $id $text
	    }
	} else {
	    if { $isModuleNotes } {
		Tooltip $canvas.module$id $ToolTipText(Module)
	    } else {
		canvasTooltip $canvas $id $ToolTipText(Connection)
	    }
	}
	
	if { $position == "none" || $position == "tooltip"} {
	    $canvas delete $id-notes $id-notes-shadow
	    continue
	}

        set shadowCol [expr [brightness $color]>0.2?"black":"white"]
	
	if { ![canvasExists $canvas $id-notes] } {
	    $canvas create text 0 0 -text "" \
		-tags "$id-notes notes" -fill $color
	    $canvas create text 0 0 -text "" -fill $shadowCol \
		-tags "$id-notes-shadow shadow"
	}

	if { $isModuleNotes } { 
	    set opt [getModuleNotesOptions $id]
	} else {
	    set opt [getConnectionNotesOptions $id]
	}
	
	$canvas coords $id-notes [lrange $opt 0 1]
	$canvas coords $id-notes-shadow [shadow [lrange $opt 0 1]]    
	eval $canvas itemconfigure $id-notes [lrange $opt 2 end]
	eval $canvas itemconfigure $id-notes-shadow [lrange $opt 2 end]
	$canvas itemconfigure $id-notes	-fill $color \
	    -font $Font(Notes) -text "$text"
	$canvas itemconfigure $id-notes-shadow -fill $shadowCol \
	    -font $Font(Notes) -text "$text"
	
	if {!$isModuleNotes} {
	    $canvas bind $id-notes <ButtonPress-1> "notesWindow $id"
	    $canvas bind $id-notes <ButtonPress-2> \
		"set Notes($id-Position) none"
	} else {
	    $canvas bind $id-notes <ButtonPress-1> \
		"notesWindow $id notesDoneModule"
	    $canvas bind $id-notes <ButtonPress-2> \
		"set Notes($id-Position) none"
	}
	canvasTooltip $canvas $id-notes $ToolTipText(Notes)		
	$canvas raise shadow
	$canvas raise notes
    }
    return 1
}
    


proc moduleEndDrag {modid x y} {
    global Subnet ignoreModuleMove CurrentlySelectedModules startX startY
    if $ignoreModuleMove return
    $Subnet(Subnet$Subnet($modid)_canvas) delete tempbox
    # If only one module was selected and moved, then unselect when done
    if {([expr abs($startX-$x)] > 2 || [expr abs($startY-$y)] > 2) && \
	    [llength $CurrentlySelectedModules] == 1} unselectAll    
}

proc moduleHelp {modid} {
    set w .mHelpWindow[$modid name]
	
    # does the window exist?
    if [winfo exists $w] {
	raise $w
	return;
    }
	
    # create the window
    toplevel $w
    append t "Help for " [$modid name]
    wm title $w $t
	
    frame $w.help
    iwidgets::scrolledhtml $w.help.txt -update 0 -alink yellow -link purple
#    text $w.help.txt -relief sunken -wrap word -bd 2 -yscrollcommand "$w.help.sb set"
    $w.help.txt render [$modid-c help]
    scrollbar $w.help.sb -relief sunken -command "$w.help.txt yview"
    pack $w.help.txt $w.help.sb -side left -padx 5 -pady 5 -fill y

    frame $w.fbuttons 
    button $w.fbuttons.ok -text "Close" -command "destroy $w"
    
    pack $w.help $w.fbuttons -side top -padx 5 -pady 5
    pack $w.fbuttons.ok -side right -padx 5 -pady 5 -ipadx 3 -ipady 3

#    $w.help.txt insert end [$modid-c help]
}

proc moduleDestroy {modid} {
    global Subnet CurrentlySelectedModules
    networkHasChanged
    if [isaSubnetIcon $modid] {
	foreach submod $Subnet(Subnet$Subnet(${modid}_num)_Modules) {
	    moduleDestroy $submod
	}
    } else {
	netedit deletemodule_warn $modid
    }

    # Deleting the module connections backwards works for dynamic modules
    set connections $Subnet(${modid}_connections)
    for {set j [expr [llength $connections]-1]} {$j >= 0} {incr j -1} {
	destroyConnection [lindex $connections $j]
    }

    # Delete Icon from canvases
    $Subnet(Subnet$Subnet($modid)_canvas) delete $modid $modid-notes $modid-notes-shadow
    destroy $Subnet(Subnet$Subnet($modid)_canvas).module$modid
    $Subnet(Subnet$Subnet($modid)_minicanvas) delete $modid
    
    # Remove references to module is various state arrays
    array unset Subnet ${modid}_connections
    array unset Disabled $modid
    array unset Notes $modid*
    listFindAndRemove CurrentlySelectedModules $modid
    listFindAndRemove Subnet(Subnet$Subnet($modid)_Modules) $modid

    $modid delete
    if { ![isaSubnetIcon $modid] } {
	after 500 "netedit deletemodule $modid"
    }
    
    # Kill the modules UI if it exists
    if {[winfo exists .ui$modid]} {
	destroy .ui$modid
    }
}

proc moduleDestroySelected {} {
    global CurrentlySelectedModules 
    foreach mnum $CurrentlySelectedModules { moduleDestroy $mnum }
}


global Box
set Box(InitiallySelected) ""
set Box(x0) 0
set Box(y0) 0

proc startBox {canvas X Y keepselected} {
    global Box CurrentlySelectedModules
    
    set Box(InitiallySelected) $CurrentlySelectedModules
    if {!$keepselected} {
	unselectAll
	set Box(InitiallySelected) ""
    }
    #Canvas Relative current X and Y positions
    set Box(x0) [expr $X - [winfo rootx $canvas] + [$canvas canvasx 0]]
    set Box(y0) [expr $Y - [winfo rooty $canvas] + [$canvas canvasy 0]]
    # Create the bounding box graphic
    $canvas create rectangle $Box(x0) $Box(y0) $Box(x0) $Box(y0)\
	-tags "tempbox temp"    
}

proc makeBox {canvas X Y} {    
   global Box CurrentlySelectedModules
    #Canvas Relative current X and Y positions
    set x1 [expr $X - [winfo rootx $canvas] + [$canvas canvasx 0]]
    set y1 [expr $Y - [winfo rooty $canvas] + [$canvas canvasy 0]]
    #redraw box
    $canvas coords tempbox $Box(x0) $Box(y0) $x1 $y1
    # select all modules which overlap the current bounding box
    set overlappingModules ""
    set overlap [$canvas find overlapping $Box(x0) $Box(y0) $x1 $y1]
    foreach id $overlap {
	set tags [$canvas gettags $id] 
	set pos [lsearch -exact $tags "module"]
	if { $pos != -1 } {
	    set modname [lreplace $tags $pos $pos]
	    lappend overlappingModules $modname
	    if { ![$modname is_selected] } {
		$modname addSelected
	    }
	}
    }
    # remove those not initally selected or overlapped by box
    foreach mod $CurrentlySelectedModules {
	if {[lsearch $overlappingModules $mod] == -1 && \
		[lsearch $Box(InitiallySelected) $mod] == -1} {
	    $mod removeSelected
	}
    }
}

proc unselectAll {} {
    global CurrentlySelectedModules
    foreach i $CurrentlySelectedModules {
	$i removeSelected
    }
}

proc selectAll { { subnet 0 } } {
    unselectAll
    global Subnet
    foreach mod $Subnet(Subnet${subnet}_Modules) {
	$mod addSelected
    }
}


# Courtesy of the Tcl'ers Wiki (http://mini.net/tcl)
proc brightness { color } {
    foreach {r g b} [winfo rgb . $color] break
    set max [lindex [winfo rgb . white] 0]
    return [expr {($r*0.3 + $g*0.59 + $b*0.11)/$max}]
 } ;#RS, after [Kevin Kenny]

proc blend { c1 c2 { alpha 0.5 } } {
    foreach {r1 g1 b1} [winfo rgb . $c1] break
    foreach {r2 g2 b2} [winfo rgb . $c2] break
    set max [expr double([lindex [winfo rgb . white] 0])]
    set oma   [expr (1.0 - $alpha)/$max]
    set alpha [expr $alpha / $max]

    set r [expr int(255*($r1*$alpha+$r2*$oma))]
    set g [expr int(255*($g1*$alpha+$g2*$oma))]
    set b [expr int(255*($b1*$alpha+$b2*$oma))]
    return [format "\#%02x%02x%02x" $r $g $b]
} 



proc clipBBoxes { args } {
    if { [llength $args] == 0 } { return "0 0 0 0" }
    set box1 [lindex $args 0]
    set args [lrange $args 1 end]
    while { [llength $args] } {
	set box2 [lindex $args 0]
	set args [lrange $args 1 end]
	foreach i {0 1} {
	    if {[lindex $box1 $i]<[lindex $box2 $i] } {
		set box1 [lreplace $box1 $i $i [lindex $box2 $i]]
	    }
	}
	foreach i {2 3} {
	    if {[lindex $box1 $i]>[lindex $box2 $i] } {
		set box1 [lreplace $box1 $i $i [lindex $box2 $i]]
	    }
	}
	if { [lindex $box1 2] <= [lindex $box1 0] || \
	     [lindex $box1 3] <= [lindex $box1 1] } {
	    return "0 0 0 0"
	}	     
    }
    return $box1
}	    

proc findModulePosition { subnet x y } {
    # if loading the module from a network, dont change its saved position
    if { [string length [info script]] } { return "$x $y" }
    global Subnet
    set canvas $Subnet(Subnet${subnet}_canvas)
    set wid  180
    set hei  80
    set canW [expr [winfo width $canvas] - $wid]
    set canH [expr [winfo height $canvas] - $hei]
    set maxx [$canvas canvasx $canW]
    set maxy [$canvas canvasy $canH]
    set x1 $x
    set y1 $y
    set acceptableNum 0
    set overlapNum 1 ;# to make the wile loop a do-while loop
    while { $overlapNum > $acceptableNum && $acceptableNum < 10 } {
	set overlapNum 0
	foreach tagid [$canvas find overlapping $x1 $y1 \
			   [expr $x1+$wid] [expr $y1+$hei]] {
	    foreach tags [$canvas gettags $tagid] {
		if { [lsearch $tags module] != -1 } {
		    incr overlapNum
		}
	    }
	}
	if { $overlapNum > $acceptableNum } {
	    set y1 [expr $y1 + $hei/3]
	    if { $y1 > $maxy } {
		set y1 [expr $y1-$canH+10+10*$acceptableNum]
		set x1 [expr $x1 + $wid/3]
		if { $x1 > $maxx} {
		    set x1 [expr $x1-$canW+10+10*$acceptableNum]
		    incr acceptableNum
		    incr overlapNum ;# to make sure loop executes again
		}
	    }

	}
    }
    return "$x1 $y1"
}

proc clampModuleToCanvas { x1 y1 } {
    global mainCanvasWidth mainCanvasHeight
    set wid  180
    set hei  80
    if { $x1 < 0 } { set x1 0 }
    if { [expr $x1+$wid] > $mainCanvasWidth } { 
	set x1 [expr $mainCanvasWidth-$wid]
    }
    if { $y1 < 0 } { set y1 0 }
    if { [expr $y1+$hei] > $mainCanvasHeight } { 
	set y1 [expr $mainCanvasHeight-$hei]
    }
    return "$x1 $y1"
}

	

proc isaSubnet { modid } {
    return [expr [string first Subnet $modid] == 0]
}

proc isaSubnetIcon { modid } {
    return [expr [string first SubnetIcon $modid] == 0]
}

proc isaSubnetEditor { modid } {
    return [expr ![isaSubnetIcon $modid] && [isaSubnet $modid]]
}


trace variable Notes wu notesTrace
trace variable Disabled wu disabledTrace


proc syncNotes { Modname VarName Index mode } {
    global Notes $VarName
    set Notes($Modname) [set $VarName]
}

# This proc will set Varname to the global value of GlobalName if GlobalName exists
# if GlobalName does not exist it will set Varname to DefaultVal, otherwise
# nothing is set
proc setIfExists { Varname GlobalName { DefaultVal __none__ } } {
    upvar $Varname var $GlobalName glob
#    upvar 
    if [info exists glob] {
	set var $glob
    } elseif { ![string equal $DefaultVal __none__] } {
	set var $DefaultVal
    }
}

# This proc will set Varname to the global value of GlobalName if GlobalName exists
# if GlobalName does not exist it will set Varname to DefaultVal, otherwise
# nothing is set
proc renameGlobal { Newname OldName } {
    upvar \#0 $Newname new
    upvar \#0 $OldName old
    if [info exists old] {
	set new $old
	unset old
    }
}

# This proc will unset a global variable without compaining if it doesnt exist
proc unsetIfExists { Varname } {
    upvar $Varname var
    if [info exists var] {
	unset var
    }
}


proc notesTrace { ArrayName Index mode } {
    # the next lines are to handle notes $id-Color and $id-Position changes
    set pos [string last - $Index]
    if { $pos != -1 } { set Index [string range $Index 0 [expr $pos-1]] }
    if { ![string length $Index] } return 
    networkHasChanged
    drawNotes $Index
    return 1
}

proc disabledTrace { ArrayName Index mode } {
    if ![string length $Index] return
    networkHasChanged
    global Subnet Disabled Notes Color    
    if [info exists Subnet($Index)] {
	return
    } else {
	if { [info exists Disabled($Index)] && $Disabled($Index) } {
	    set Notes($Index-Color) $Color(ConnDisabled)
	} else {	    
	    setIfExists Notes($Index-Color) Color($Index)
	}
	set conn [parseConnectionID $Index]
	if [string equal w $mode] {
	    drawConnections [list $conn]	
	    $Subnet(Subnet$Subnet([oMod conn])_canvas) raise $Index
	    checkForDisabledModules [oMod conn] [iMod conn]
	}
    }
    return 1
}




# Returns 1 if the window is mapped.  Use this function if you don't
# know whether the window exists yet.
proc windowIsMapped { w } {
    if {[winfo exists $w]} {
	if {[winfo ismapped $w]} {
	    return 1
	}
    }
    return 0
}
