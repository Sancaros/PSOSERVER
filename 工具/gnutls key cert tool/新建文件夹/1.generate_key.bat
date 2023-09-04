gnutls-3.8.1-w32\bin\certtool --generate-privkey --outfile server.key
gnutls-3.8.1-w32\bin\certtool --generate-privkey --outfile private.key
gnutls-3.8.1-w32\bin\certtool --generate-request --load-privkey private.key --outfile certificate.csr
gnutls-3.8.1-w32\bin\certtool --generate-self-signed --load-privkey private.key --infile certificate.csr --outfile certificate.crt
