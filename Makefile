all: fpga sw

fpga: .DUMMY
	make -C fpga

sw: .DUMMY
	make -C sw

.DUMMY: