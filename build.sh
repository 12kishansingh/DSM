rm -f *.o main
g++ -I./headers \
    main.c \
    src/*.c \
    src/clientsCommands/*.c \
    src/serverCommands/*.c \
    src/clientsCommands/*.cpp \
    src/serverCommands/*.cpp \
    -luring \
    -o main