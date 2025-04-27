reset
ppp=24.0
isr_freq=1e6
unset multiplot
set terminal png size 1600,600
set output "2-analyse.png"

##1:i,2:time, 3:vgrid, 4:grid_phase, 5:fgrid, 6:internal_speed, 7:phasedetector, 8:filt, 9: isr_timer

set multiplot layout 2,2
set grid

set title "grid synchronization"
set yrange [-1:1]
set key left
plot "testdata.dat" u 2:3 w lines t "grid"\
	, "" u 2:8 w l t "filt"
unset title 

unset yrange
set yrange [49.5:51.5]
plot "testdata.dat" u 2:5 t "fgrid" w l, "" u 2:6 t "finternal" w l

set key right
set yrange [42000:48000]
plot "testdata.dat" u 2:9 w lines t "isr_timer"

set yrange [-0.5:0.5]
set xlabel "time / s"
plot "testdata.dat" u 2:7 w l t "det"\
	,"" u 2:($5-$6) t "fgrid-finternal" w l

unset multiplot
unset output
