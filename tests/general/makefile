%.o: %.cpp
	occ /! /DWIN32_LEAN_AND_MEAN $^


%.o: %.c
	occ /9 /DWIN32_LEAN_AND_MEAN /! $^

%.exe: %.o
	olink /c /! /T:CON32 /o$@ c0xpe.o $^ clwin.l climp.l

test: bzip2.exe parsecsv.exe label.exe delegate.exe world.exe jarjar.exe
	bzip2 bzip2.test
	fc /b bzip2.test.bz2 bzip2.test.bz2.cmpx
	bzip2 -d bzip2.test.bz2
	fc /b bzip2.test bzip2.test.cmpx
	del out.pcl
	parsecsv
	fc /b test.cmpx out.pcl
	label
	label1 > label1.out
	fc /b label1.cmpx label1.out
	delegate > delegate.out
	fc /b delegate.cmpx delegate.out

clean:
	$(CLEAN)

world.exe: world.c
	occ /9 /! /Wcm $^
	world > world.out
	fc /b world.cmpx world.out

jarjar.exe: jarjar.c
	occ /9 /! /Wcc $^
	jarjar > jarjar.out
	fc /b jarjar.cmpx jarjar.out	