.PHONY: key
key:
	openssl genrsa -out key
	openssl req -new -key key -out csr -subj /name=tunnel
	openssl x509 -req -in csr -key key -out crt
