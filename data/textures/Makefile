SVG=$(wildcard *.svg)
PNG=$(SVG:.svg=.png)

all: $(PNG)

%.png: %.svg
	inkscape $< --export-dpi=100 --export-png=$@

.PHONY: all
