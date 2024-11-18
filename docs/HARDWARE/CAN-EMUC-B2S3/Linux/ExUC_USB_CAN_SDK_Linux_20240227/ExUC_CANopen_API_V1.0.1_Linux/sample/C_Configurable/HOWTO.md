[Windows]
----------------------------------------------------
x86:
    need: library_v1.0.0\windows\32
    use: $ make clean; make

x64:
    need: library_v1.0.0\windows\64
    use: modify Makefile
         $ make clean; make

[Linux]
----------------------------------------------------
(build sample code by dynamic library)
need: library_v1.0.0\linux\ lib_canopen_32.so or lib_canopen_64.so
use: $ make clean; make

(build sample code by static library)
need: library_v1.0.0\linux\ lib_canopen_32.a or lib_canopen_64.a
use: $ make clean STATIC=1; make STATIC=1
