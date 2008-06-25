setMode -bs
setCable -port svf -file "XXX.svf"
addDevice -p 1 -file "XXX.bit"
addDevice -p 2 -part xcf04s
Program -p 1 -defaultVersion 0
quit

