INSTALL=../libs

.inp.out:
	ord68 $*.inp $*.out

$(LIB):	$(LIB).out
	lb68 $(INSTALL)/$(LIB).lib -f $(LIB).out
