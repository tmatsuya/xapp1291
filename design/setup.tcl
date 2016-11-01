###
# VPRD setup script 
###
create_project vprd ./vprd_project -part xc7k325tffg900-2
set_property board_part xilinx.com:kc705:part0:1.2 [current_project]
add_files -fileset constrs_1 -norecurse ./design/vprd_fmc20.xdc
#set_property ip_repo_paths  {./repos/hw_repos} [current_fileset]
update_ip_catalog
source ./design/vprd_fmc20_bd.tcl
assign_bd_address
set_property range 128M [get_bd_addr_segs {processor_ss/microblaze_0/Data/SEG_axi_emc_0_MEM0}]
regenerate_bd_layout
save_bd_design
make_wrapper -files [get_files ./vprd_project/vprd.srcs/sources_1/bd/vprd/vprd.bd] -top
add_files -norecurse ./vprd_project/vprd.srcs/sources_1/bd/vprd/hdl/vprd_wrapper.v
set_property synth_checkpoint_mode Hierarchical [get_files  ./vprd_project/vprd.srcs/sources_1/bd/vprd/vprd.bd]
