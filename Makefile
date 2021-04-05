all:
		g++ -o hospitalA hospitalA.cpp
		g++ -o hospitalB hospitalB.cpp
		g++ -o hospitalC hospitalC.cpp
		g++ -o scheduler scheduler.cpp
		g++ -o client client.cpp
clean: 
	  $(RM) HospitalC
	  $(RM) HospitalB
	  $(RM) HospitalA
	  $(RM) scheduler
	  $(RM) client