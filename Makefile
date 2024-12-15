.PHONY: all key clean
all:
	make -C client
	make -C server
key:
	openssl genrsa -out key
	openssl req -new -key key -out csr -subj /name=tunnel
	openssl x509 -req -in csr -key key -out crt
clean:
	rm -f *.pb.cc *.pb.h
	clang-format -i *.cc *.h
