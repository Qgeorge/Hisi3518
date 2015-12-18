gcc  test1.c -I ../../json-c-lib/include/json-c/ -L ../../json-c-lib/lib/ -ljson-c -o test1
gcc  test2.c -I ../../json-c-lib/include/json-c/ -L ../../json-c-lib/lib/ -ljson-c -static -o test2_s 
gcc  test_parse.c -I ../../json-c-lib/include/json-c/ -L ../../json-c-lib/lib/ -ljson-c -static -o test_parse
