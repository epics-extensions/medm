
all: graphApp.ps graphApp.dvi

graphApp.ps: graphApp.dvi logo.ps 
	dvips -f graphApp.dvi > graphApp.ps

graphApp.dvi: graphApp.latex testGraphApp1.latex graphApp.toc graphApp.ind
	latex graphApp.latex

graphApp.toc: graphApp.latex
	latex graphApp.latex

graphApp.ind: graphApp.idx
	latex graphApp.latex
	makeindex graphApp.idx

	
