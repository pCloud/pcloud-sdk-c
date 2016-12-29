CC=clang
#CC=gcc
#CFLAGS       = -fPIC -g #-pedantic -Wall -Wextra -ggdb3
LDFLAGS      = -lz -ldl -lpmobile -Lpmobile -lmbedtls -lfuse -lpthread -Llibs/mbedtls/library
SHAREFLAGS = -shared

DEBUGFLAGS   = -O0 -D _DEBUG
RELEASEFLAGS = -O2 -D NDEBUG -combine -fwhole-program

USESSL=mbedtls

#CFLAGS=-Wall -Wpointer-arith -g -O2 -fsanitize=address
CFLAGS=-Wall -Wpointer-arith -g -fPIC -I./pmobile
#CFLAGS=-Wall -Wpointer-arith -g -O2

TARGET=pSDK.so

STAT_LIBS= pmobile/libpmobile.a libs/mbedtls/library/libmbedtls.a

OBJ=pSDK.o libs/sqlite/sqlite3.o

all: $(TARGET) test 

libs/mbedtls/library/libmbedtls.a:
	cd libs/mbedtls && $(MAKE)

pmobile/libpmobile.a: 
	cd pmobile && $(MAKE)

#$(TARGET): $(OBJ)
#	$(LINK.c) -shared $< -o $@
$(TARGET): pmobile/libpmobile.a libs/mbedtls/library/libmbedtls.a $(OBJ) 
	$(CC) $(SHAREFLAGS) $(CFLAGS) $(LDFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(OBJ) $(STAT_LIBS)
	
test: test.o $(TARGET)
	$(CC) test.c -o test -rdynamic ./$(TARGET) $(FLAGS) $(CFLAGS) $(LDFLAGS) $(DEBUGFLAGS) 
	

clean:
	rm -f *~ *.o $(TARGET) ./test && cd libs/mbedtls && $(MAKE) clean && cd ../.. && cd pmobile && $(MAKE) clean

