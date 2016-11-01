proc vprd args {

	if {[llength $args] != 1} {
		puts "error: hdf file name missing from command line"
		puts "Please specify hdf to process"
		puts "Example Usage: vprd.tcl design1.hdf"
	} else {

		set hdf [lindex $args 0]

		#set workspace
		puts "Create Workspace"
		sdk setws vprd.sdk
		
		#create hw project
		puts "Create HW Project"
		sdk createhw -name vprd_kc705_hw_platform_0 -hwspec ./$hdf

		#create bsp
		puts "Create BSP"
		sdk createbsp -name vprd_bsp -hwproject vprd_kc705_hw_platform_0 -proc processor_ss_microblaze_0 -os standalone

		#create application project
		puts "Create Application Project"
		sdk createapp -name vprd_ref_design -hwproject vprd_kc705_hw_platform_0 -proc processor_ss_microblaze_0 -os standalone -lang C -app {Empty Application} -bsp vprd_bsp

		#copy example source files to app project
		puts "Get Application Design Source Files"
		sdk importsources -name vprd_ref_design -path ./vprd_ref_design/src -linker-script 

		#build project
		puts "Build Project"
		sdk project -build -type all
	}
}
