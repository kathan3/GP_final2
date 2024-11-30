CC = g++
RM = /bin/rm -rf

CFLAGS = -O3 -Wall

LIBDIRS = -L.
LIBS = -lGL -lGLEW -lm -lglfw

BIN = sample
SRCS = main.cpp 

OBJS = main.o 

# compile all '.o' files from their like named '.cpp' files and then link
#   them into a file name ${BIN}
${BIN}: ${OBJS}
	${CC} ${OBJS} ${LIBDIRS} ${LIBS} -o $@

# Pattern rule to compile .cpp files to .o files in the same directory
%.o: %.cpp
	${CC} ${CFLAGS} ${INCDIRS} -c $< -o $@

# specify clobber and clean as phony so they still run even if files
#   exist with the same names
.PHONY : clean remake
clean :
	${RM} ${BIN}
	${RM} ${OBJS}

remake : clean ${BIN}

#make a list of dependencies using makedepend
depend:
	makedepend -- $(CFLAGS) -- -Y $(SRCS)
