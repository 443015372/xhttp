xhttp:
	g++ main.cpp XThread.cpp md5.cpp -o xhttp -std=c++11 -lpthread

clean:
	@rm -rf xhttp
