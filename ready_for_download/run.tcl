fpga -f download.bit
connect mb mdm
after 5000
rst
stop
dow vprd_ref_design.elf
run
