# invoke SourceDir generated makefile for labra.pem3
labra.pem3: .libraries,labra.pem3
.libraries,labra.pem3: package/cfg/labra_pem3.xdl
	$(MAKE) -f D:\ti\JTKJ-Harjoitustyo2020/src/makefile.libs

clean::
	$(MAKE) -f D:\ti\JTKJ-Harjoitustyo2020/src/makefile.libs clean

