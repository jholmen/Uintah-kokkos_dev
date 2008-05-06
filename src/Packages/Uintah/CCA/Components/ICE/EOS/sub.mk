# Makefile fragment for this subdirectory

SRCDIR   := Packages/Uintah/CCA/Components/ICE/EOS

SRCS     += $(SRCDIR)/EquationOfState.cc \
	$(SRCDIR)/EquationOfStateFactory.cc \
	$(SRCDIR)/IdealGas.cc \
	$(SRCDIR)/HardSphereGas.cc \
	$(SRCDIR)/TST.cc \
	$(SRCDIR)/JWL.cc      \
	$(SRCDIR)/JWLC.cc     \
	$(SRCDIR)/Gruneisen.cc     \
	$(SRCDIR)/Tillotson.cc     \
	$(SRCDIR)/Thomsen_Hartka_water.cc     \
	$(SRCDIR)/Murnahan.cc

PSELIBS := \
	Packages/Uintah/CCA/Ports \
	Packages/Uintah/Core/Grid \
	Packages/Uintah/Core/Parallel \
	Packages/Uintah/Core/Exceptions \
	Packages/Uintah/Core/Math \
	Core/Exceptions Core/Thread Core/Geometry 

LIBS	:= 


