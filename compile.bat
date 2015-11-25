g++ Server\main.cpp -o Server\server.exe -IServer\libmicrohttpd\include -LServer\libmicrohttpd\lib -llibmicrohttpd
xcopy /y/q Server\libmicrohttpd\bin\*.dll Server\
g++ Client\main.cpp -o Client\client.exe -DCURL_STATICLIB -IClient\curl\include -LClient\curl\lib -lcurl -lrtmp -lwinmm -lssh2 -lssl -lcrypto -lgdi32 -lcrypt32 -lz -lidn -lwldap32 -lws2_32
xcopy /y/q Client\curl\bin\*.dll Client\
