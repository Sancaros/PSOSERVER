# 生成CA私钥
gnutls-3.8.1-w32\bin\certtool --generate-privkey --outfile ca-key.pem

# 创建CA证书请求（CSR）并添加预设模板
gnutls-3.8.1-w32\bin\certtool --generate-request --load-privkey ca-key.pem --outfile ca-csr.pem --template ca-template.conf

# 使用自签名方式生成CA证书
gnutls-3.8.1-w32\bin\certtool --generate-self-signed --load-privkey ca-key.pem --infile ca-csr.pem --outfile ca-cert.pem --template ca-template.conf

# 生成服务器私钥
gnutls-3.8.1-w32\bin\certtool --generate-privkey --outfile server-key.pem

# 创建服务器证书请求（CSR）并添加预设模板
gnutls-3.8.1-w32\bin\certtool --generate-request --load-privkey server-key.pem --outfile server-csr.pem --template server-template.conf

# 使用CA证书签署服务器证书
gnutls-3.8.1-w32\bin\certtool --generate-certificate --load-request server-csr.pem --load-ca-certificate ca-cert.pem --load-ca-privkey ca-key.pem --outfile server-cert.pem --template server-template.conf

# 生成客户端私钥
gnutls-3.8.1-w32\bin\certtool --generate-privkey --outfile client-key.pem

# 创建客户端证书请求（CSR）并添加预设模板
gnutls-3.8.1-w32\bin\certtool --generate-request --load-privkey client-key.pem --outfile client-csr.pem --template client-template.conf

# 使用CA证书签署客户端证书
gnutls-3.8.1-w32\bin\certtool --generate-certificate --load-request client-csr.pem --load-ca-certificate ca-cert.pem --load-ca-privkey ca-key.pem --outfile client-cert.pem --template client-template.conf
