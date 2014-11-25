PROGRAM=fgview.exe
OBJS=	app.obj \
	main.obj
SRCS=$(OBJS:.obj=.cpp)

CC=cl -nologo
LD=cl -nologo

CFLAGS=-O2 -MD -EHsc
LDFLAGS=$(CFLAGS)

LIBS=user32.lib shell32.lib ole32.lib d2d1.lib windowscodecs.lib dwrite.lib shlwapi.lib

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(LD) $(LDFLAGS) -Fe$@ $(OBJS) $(LIBS) -link -subsystem:windows

app.obj: app.h fgview.h
main.obj: app.h fgview.h

.cpp.obj:
	$(CC) $(CFLAGS) -c $*.cpp
