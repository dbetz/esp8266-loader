CON
   _clkmode = xtal1 + pll16x
   _xinfreq = 5_000_000

#ifdef MINI
  LED1 = 6
  LED2 = 7
#else
  LED1 = 16
  LED2 = 23
#endif

#ifdef SLOW
  DIVISOR = 2
#else
  DIVISOR = 8
#endif

PUB main : mask
  mask := |< LED1 | |< LED2
  OUTA := |< LED1
  DIRA := mask
  repeat
    OUTA ^= mask
    waitcnt(CNT + CLKFREQ / DIVISOR)
