
all: graphX.ps graphX.dvi

graphX.ps: graphX.dvi logo.ps graphBar.ps graphLine.ps graphPoint.ps \
		seqlLine.ps seqlPolygon.ps strip1.ps strip2.ps surface.ps
	dvips -f graphX.dvi > graphX.ps

graphX.dvi: graphX.latex testGraph.latex testStrip.latex testSurface.latex \
		graphX.toc graphX.ind
	latex graphX.latex

graphX.toc: graphX.latex
	latex graphX.latex

graphX.ind: graphX.idx
	latex graphX.latex
	makeindex graphX.idx

	
