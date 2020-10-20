# invoke SourceDir generated makefile for labra.pem3
labra.pem3: .libraries,labra.pem3
.libraries,labra.pem3: package/cfg/labra_pem3.xdl
	$(MAKE) -f C:\Users\lassi\workspace_v613\JTKJ_labra/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\lassi\workspace_v613\JTKJ_labra/src/makefile.libs clean

