#gcc-4.4 ./*.c -O2 -I ./nouse/libghttp/include/ -L ./nouse/libghttp/lib/ -lghttp
#arm-hisiv100nptl-linux-gcc  ./*.c -I ./libghttp/include/ -L ./libghttp/lib/ -lghttp -Wall
gcc get_mac.c  test_main.c net_http.c cJSON.c http_request.c -L nouse/libghttp/lib -lghttp -lm
#arm-hisiv100nptl-linux-gcc -I ./libghttp/include libghttp.a 
#arm-none-eabi-gcc -I ./libghttp/include libghttp.a 
