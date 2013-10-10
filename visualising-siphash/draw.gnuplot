set terminal pngcairo transparent enhanced linewidth 2 font 'arial,10' size 1200, 1100
set output 'siphash-round.png'
set samples 1001

set lmargin at screen 0.05
set rmargin at screen 0.88
set bmargin at screen 0.05
set tmargin at screen 0.98

set border 0
set yrange [255:0]
set xrange [0:255]

set xtics nomirror
set ytics nomirror

set xtics 8
set mxtics 4
set ytics 8
set mytics 4

set pm3d map
set pm3d corners2color c4

set palette defined (0 "white", 0.80 "black", 1 "red")

splot "data.tsv" u 2:1:3 with pm3d

