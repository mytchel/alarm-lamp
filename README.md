# Fuses

We are using an external 6MHz crystal clock so you need to 
set the low fuse byte to 0xFD. I do not understand how the 
fuses work so I fucked on chip by setting it to what would 
make sense to me but is not correct. The fuses bits do not
seem to work as high or low but as programmed or unprogrammed.
I do not know how to unprogram it. Second attempt worked.

Use [fusecalc](http://www.engbedded.com/fusecalc).
