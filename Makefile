all:
		g++ -o HospitalA HospitalA.cpp
		g++ -o HospitalB HospitalB.cpp
		g++ -o HospitalC HospitalC.cpp
		g++ -o scheduler scheduler.cpp
		g++ -o client client.cpp
clean: 
	  $(RM) HospitalC
	  $(RM) HospitalB
	  $(RM) HospitalA
	  $(RM) scheduler
	  $(RM) client