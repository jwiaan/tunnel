app := client
.PHONY: clean
$(app): main.cc client.cc client.h connection.cc connection.h ../common.cc ../common.h ../poller.cc ../poller.h ../tunnel.pb.cc ../tunnel.pb.h
	g++ -Wall -o$@ main.cc client.cc connection.cc ../common.cc ../poller.cc ../tunnel.pb.cc -lssl -lprotobuf
../tunnel.pb.cc ../tunnel.pb.h: ../tunnel.proto
	protoc tunnel.proto --proto_path=.. --cpp_out=..
clean:
	rm -f *.o $(app)
	clang-format -i *.cc *.h
