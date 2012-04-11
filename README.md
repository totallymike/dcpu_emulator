Mike's DCPU-16 Emulator.
========================

The DCPU-16 is a fictional CPU, designed by Markus Persson, aka Notch.  The spec is located at http://0x10c.com, along with further information about it and the game it is designed for.  
  
This project comes with the source to build a small test binary.  The binary was written by Markus Persson, aka Notch.  The test code just packs it into a binary.

To run,  
  
```
./vm --help
```  
  
To run the test:  
  
```
make test
```

That compiles everything necessary and then runs the vm against the test binary.  
