# 生成客户端私钥
gnutls-3.8.1-w32\bin\certtool --generate-privkey --outfile client-key.pem

# 创建客户端证书请求（CSR）并添加预设模板
gnutls-3.8.1-w32\bin\certtool --generate-request --load-privkey client-key.pem --outfile client-csr.pem --template client-template.conf

# 使用CA证书签署客户端证书
gnutls-3.8.1-w32\bin\certtool --generate-certificate --load-request client-csr.pem --load-ca-certificate ca-cert.pem --load-ca-privkey ca-key.pem --outfile client-cert.pem --template client-template.conf
