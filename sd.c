/*

SD.C : Serial doodad.

Copyright (C) 2014 Danny Chouinard

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

See the full text of the license in file COPYING.

To reach me, execute this command in a bourne shell:
$ echo qnaal.pubhvaneq@tznvy.pbz | tr "[a-z]" "[n-za-m]"

That's a rot13 of my e-mail address.

*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>

FILE *fp;
time_t t;
char twirl[4]={"/-\\|"};
fd_set FDS;
int noctrl=0;
int baud=9600;
int pbaud=115200;
int oldstatus=0;
int status;
int klock=0;
int tictacw=0;
int tictacx=0;
int samples=-1;
int longterm=0;
int period=1;
int printdrift=0;
int f_cpu=10000000;
int i,k,s;
int timestamp=0;
int help=0;
int timing=0;
int programming=0;
int sdsdp=0;
int holdreset=0;
int bbterm=0;
uint8_t twirlc=0;
int invertdtr=0;
int pulsedtr=0;
unsigned char c;
char filename[1024];
char part[20]={"Unknown!"};
int raw=0;
int signature;
int pagesize=0;
int pages=0;
int flashsize=0;
int verbose=1;
int dochiperase=0;
int dowritelfuse=0;
int dowritehfuse=0;
int dowriteefuse=0;
int doreadflash=0;
int dowriteflash=0;
int idelay=-1;
int sdpidelay=-1;
int lfuse=0xFF;
int hfuse=0xFF;
int efuse=0xFF;
int newlfuse=0xFF;
int newhfuse=0xFF;
int newefuse=0xFF;
int cd=1;
int ri=1;
int dsr=1;
int cts=1;
int dtr=1;
int rts=1;
int signaltype=1;
int transition=0;
int h,m,s,n,level,oldcdlevel;
int i;
int comfd=0;
long double f,d;
long double sf;
struct tm *ts;
struct timeval tvs,tvn,tvp,tvd;
int clocal=CLOCAL;
int speed=B9600;
int crtscts=0;
int parity=0;
int bits=CS8;
int stopbits=0;
int cdmode=0;
int option;
int interval=0;
int audio=0;
struct termios stbuf;
struct termio cons, oricons;
char device[128];
char buffer[128];
int flags;
char tictacstring[200];
char capturefile[256];
char macro[1024];
int ocrnl=0;
int localecho=0;
int hex=0;
struct timeval timeout;
struct timeval start,now,diff;
int sendmacro=-1;
int capture=0;
int programend;
FILE *capturefp;
uint8_t *flashbuffer;

void bbexit(int code) {
  ioctl(0,TCSETA,&oricons);
  printf("\n");
  exit(code);
}

void bbatexit(void) {
  ioctl(0,TCSETA,&oricons);
  bbexit(0);
}

void intexit(int code) {
  printf("\r\nINT signal received.\r\n");
  bbexit(code);
}

void termexit(int code) {
  printf("\r\nTERM signal received.\r\n");
  bbexit(code);
}

void killexit(int code) {
  printf("\r\nKILL signal received.\r\n");
  bbexit(code);
}

void bigassclock(void) {
  static unsigned char initted=0;
  struct timeval tv;
  unsigned int i,j,l,op;
  unsigned char c,g,n,p;
  static unsigned char *o;
  char *ct;
  static unsigned char d[]={
  "A0002620262026202620262026202620A000042404240424042404240424042404240424\
A000082008200820A000280028002800A000A000082008200820A000082008200820A0002620\
262026202620A0000820082008200820A000280028002800A000082008200820A000A0002800\
28002800A000262026202620A000A00008200820082008200820082008200820A00026202620\
2620A000262026202620A000A000262026202620A00008200820082008200200020002002000\
02002000020002000200"
  };
  if(!initted) {
    printf("\n\n\n\n\n\n\n\n\n");
    o=malloc(4096);
    initted=1;
  }
  gettimeofday(&tv,0);
  ct=ctime(&tv.tv_sec);
  op=0; o[op++]=13; o[op++]=033; o[op++]='['; o[op++]='9'; o[op++]='A';
  for(l=0;l<9;l++) {
    o[op++]=' ';
    for(j=0;j<8;j++) {
      c=ct[j+11]; if(c==':') c='9'+1;
      c-='0'; p=0;
      for(i=0;i<4;i++) {
        p=1-p; n=d[c*36+l*4+i]; if(n=='A') n='9'+1;
        n-='0';
        if(p==1 && n!=0 ) {
          o[op++]=033; o[op++]='['; o[op++]='7'; o[op++]='m';
          for(g=0;g<n;g++) o[op++]=' ';
          o[op++]=033; o[op++]='['; o[op++]='0'; o[op++]='m';
        } else {
          for(g=0;g<n;g++) o[op++]=' ';
        }
      }
      if(j<7) { o[op++]=' '; o[op++]=' '; }
    }
    o[op++]='\n';
  }
  o[op]=0; write(2,o,op);
}

void rtson(void) {
  ioctl(comfd,TIOCMGET,&flags);
  flags|=TIOCM_RTS;
  ioctl(comfd,TIOCMSET,&flags);
}

void rtsoff(void) {
  ioctl(comfd,TIOCMGET,&flags);
  flags&=~TIOCM_RTS;
  ioctl(comfd,TIOCMSET,&flags);
}

void dtron(void) {
  ioctl(comfd,TIOCMGET,&flags);
  flags|=TIOCM_DTR;
  ioctl(comfd,TIOCMSET,&flags);
}

void dtroff(void) {
  ioctl(comfd,TIOCMGET,&flags);
  flags&=~TIOCM_DTR;
  ioctl(comfd,TIOCMSET,&flags);
}

void dtrtoggle(void) {
  if(dtr) dtroff();
  else dtron();
  dtr^=1;
}

void rtstoggle(void) {
  if(rts) rtsoff();
  else rtson();
  rts^=1;
}

uint8_t cdstatus(void) {
  uint8_t r=0;
  int status;
  r=ioctl(comfd,TIOCMGET,&status);
  if(r) {
    fprintf(stderr,"ioctl failed with code %d.\n",r);
    exit(1);
  }
  if(status&TIOCM_CAR) r=1;
  return(r);
}

uint8_t fakecd(void) {
  static int status;
  if((rand()%100)==59) status^=1;
  return(status);
}

void tvdiff(struct timeval *op1, struct timeval *op2, struct timeval *result) {
  int units, micros;
  micros=op1->tv_usec-op2->tv_usec;
  units=op1->tv_sec-op2->tv_sec;
  if(micros<0) {
    micros+=1000000;
    units--;
  }
  result->tv_sec=units;
  result->tv_usec=micros;
}

void ptv(int shortform,struct timeval *v) {
  if(shortform) {
    printf("%2d.%06d",(int)v->tv_sec,(int)v->tv_usec);
  } else {
    printf("%6d.%06d",(int)v->tv_sec,(int)v->tv_usec);
  }
}

void usage(void) {
  fprintf(stderr,"\n");
  fprintf(stderr,"Serial Doodad.  (C)2014 Danny Chouinard.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Usage: sd mode_option [mode sub-options].\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -t ...  Timing mode                                 (-th for usage).\n");
  fprintf(stderr,"  -p ...  AVR programming mode                        (-ph for usage).\n");
  fprintf(stderr,"  -b ...  BBTERM mode                                 (-bh for usage).\n");
  fprintf(stderr,"  -k      Show a large clock mode.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -d dev  Specify serial device. (All modes, \"/dev/tty\" prefix optional).\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Environment variables: SDDEV, SDBAUD.\n");
  fprintf(stderr,"\n");
  exit(1);
}

void bbtermusage(void) {
  fprintf(stderr,"\n");
  fprintf(stderr,"Serial Doodad, terminal mode. (C)2014 Danny Chouinard.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Usage: sd -b [terminal mode options]\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -d dev    Serial device (/dev/tty optional).\n");
  fprintf(stderr,"  -c file   Set capture filename.\n");
  fprintf(stderr,"  -b N      Set baud rate.\n");
  fprintf(stderr,"  -f        Invert DTR state.\n");
  fprintf(stderr,"  -n        Don't obey special control codes from serial line.\n");
  fprintf(stderr,"  -o        Output CR upon LF.\n");
  fprintf(stderr,"  -e        Local echo.\n");
  fprintf(stderr,"  -x        Enable hardware modem signals.\n");
  fprintf(stderr,"  -w        Hex mode on.\n");
  fprintf(stderr,"  -y        Enable CTS/RTS handshake.\n");
  fprintf(stderr,"  -a        Two stop bits instead of one.\n");
  fprintf(stderr,"  -m text   Set macro string.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"While in terminal mode, Type Control-A H to get help.\n");
  fprintf(stderr,"\n");
}

void timingusage(void) {
  fprintf(stderr,"\n");
  fprintf(stderr,"Serial Doodad, timing mode. (C)2014 Danny Chouinard.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Usage: sd -t [timing mode options]\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -a       Ring the bell every signal.\n");
  fprintf(stderr,"  -b baud  Set baud rate.\n");
  fprintf(stderr,"  -s N     Signal input (0=RD, 1=CD, 2=DSR, 3=CTS, 4=RI).\n");
  fprintf(stderr,"  -t N     Signal transition (0:High-Low 1:Low-High, 2=Any).\n");
  fprintf(stderr,"  -d dev   Serial device (/dev/tty optional).\n");
  fprintf(stderr,"  -f       Invert DTR state.\n");
  fprintf(stderr,"  -i ms    Sampling minimum interval in milliseconds.\n");
  fprintf(stderr,"  -l       Start a long-term sample.\n");
  fprintf(stderr,"  -p N     Perform a long-term checkpoint, interval N.\n");
  fprintf(stderr,"  -n N     Do N samples (default=infinite).\n");
  fprintf(stderr,"  -x       Print timestamps.\n");
  fprintf(stderr,"  -y N     Pulse DTR at modulo N seconds.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -o HZ    Show adjustment parameters for Tic Tac clock frequency HZ.\n");
  fprintf(stderr,"  -w       Set time on Tic Tac clock.\n");
  fprintf(stderr,"  -u str   Transmit string to Tic Tac clock.\n");
}

void programusage(void) {
  fprintf(stderr,"\n");
  if(!sdsdp) {
    fprintf(stderr,"          TD  3 ---|<---/\\/\\/--- RESET\n");
    fprintf(stderr,"          DTR 4 --------/\\/\\/--- MOSI\n");
    fprintf(stderr,"          GND 5 ---------------- GND\n");
    fprintf(stderr,"          DSR 6 ---------------- MISO\n");
    fprintf(stderr,"          RTS 7 --------/\\/\\/--- SCK\n");
    fprintf(stderr,"\n");
  }
  fprintf(stderr,"Serial Doodad, AVR programming mode");
  if(sdsdp) fprintf(stderr," with SDP module");
  fprintf(stderr,".  (C)2014 Danny Chouinard.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Usage: sd -p  [programming mode options]\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -b       Use binary instead of Intel Hex file format.\n");
  fprintf(stderr,"  -d dev   Serial device (/dev/tty optional, append \"+sdp\" with SDP module).\n");
  fprintf(stderr,"  -s N     Switch baud rate on SDP module.\n");
  fprintf(stderr,"  -i N     delay in microseconds at clock transitions.\n");
  fprintf(stderr,"  -v N     Set verbose level (0-3).  Use -v2 to show part and fuses.\n");
  fprintf(stderr,"  -r FN    Read FLASH contents and write into FN file.\n");
  fprintf(stderr,"  -w FN    Write FLASH contents from FN file.\n");
  fprintf(stderr,"  -E N     Write efuse.\n");
  fprintf(stderr,"  -L N     Write lfuse.\n");
  fprintf(stderr,"  -H N     Write hfuse.\n");
  fprintf(stderr,"  -e       End with RESET held disabled.\n");
  fprintf(stderr,"  -z       Perform chip erase.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"Environment variables: SDDEV, SDBAUD, SDPBAUD, SDPIDELAY.\n");
  fprintf(stderr,"\n");
}

unsigned char getonebyte(int from, int sec, int usec) {
  unsigned char c=0;
  int a;
  fd_set FDS;
  struct timeval timeout;
  timeout.tv_sec=sec;
  timeout.tv_usec=usec;
  FD_ZERO(&FDS);
  if(from&1) FD_SET(comfd,&FDS);
  if(from&2) FD_SET(0,&FDS);
  a=select(comfd+1,&FDS,NULL,NULL,&timeout);
  if(FD_ISSET(comfd,&FDS)) {
    a=read(comfd,&c,1);
    if(a) {
      return(c);
    }
  }
  return(0);
}

void readstatus(void) {
  int r,status;
  r=ioctl(comfd,TIOCMGET,&status);
  if(r) {
    fprintf(stderr,"ioctl failed with code %d.\n",r);
    exit(1);
  }
  ri=cd=dsr=cts=0;
  if(status&TIOCM_CD) cd=1;
  if(status&TIOCM_DSR) dsr=1;
  if(status&TIOCM_CTS) cts=1;
  if(status&TIOCM_RI) ri=1;
}

void getonesample(void) {
  static int oldsig=0;
  int first=1;
  int sig=0;
  if(signaltype==0) getonebyte(1,10000,0);
  else {
    while(1) {
      readstatus();
      switch(signaltype) {
        case 1: sig=cd; break;
        case 2: sig=dsr; break;
        case 3: sig=cts; break;
        case 4: sig=ri; break;
      }
      if(first) {
        first=0;
        oldsig=sig;
      } else {
        if(sig!=oldsig) {
          oldsig=sig;
          if(transition==2) return;
          if(sig==0 && transition==0) return;
          if(sig!=0 && transition==1) return;
        }
      }
      usleep(100);
    }
  }
}

void doprintdrift(long double ratio, int f_cpu) {
  long double clock, drift,t;
  unsigned int d0,d1,d2,d3,slow;
  d1=d2=d3=0;
  slow=0;
  clock=(long double)f_cpu/f;
  drift=(1.0L-ratio)*4294967296.0L;
  if(drift<0.0L) {
    slow=1;
    drift=0.0L-drift;
  }
  t=drift;
  d0=t/16777216.0L;
  t-=d0*16777216.0L;
  d1=t/65536.0L;
  t-=d1*65536.0L;
  d2=t/256.0L;
  t-=d2*256.0L;
  d3=t;
  if(d0) {
    printf(" [XXXXXX] XX");
  } else {
    printf(" [%02X%02X%02X] ",d1,d2,d3);
    if(slow) printf("S");
    else printf("F");
  }
}

void dtrpulse(int ontime, int offtime) {
  dtrtoggle();
  usleep(ontime);
  dtrtoggle();
  usleep(offtime);
}

void xmittictacstring() {
  unsigned char c,b;
  int i,j;
  printf("X"); fflush(stdout);
  usleep(200000);
  printf("M"); fflush(stdout);
  dtrpulse(793457,100000); // Approx 13000/16384 second.
  printf("I"); fflush(stdout);
  dtrpulse(125000,100000);  // Short press.
  printf("T"); fflush(stdout);
  dtrpulse(400000,300000); // Long press, longish delay.
  printf(": "); fflush(stdout);
  i=strlen(tictacstring);
  tictacstring[i++]=0xff;
  tictacstring[i]=0;
  i=0;
  while(tictacstring[i]) {
    b=(unsigned char)tictacstring[i];
    c=b;
    for(j=0;j<8;j++) {
      if(b&128) dtrpulse(4000,2000);
      else dtrpulse(1000,2000);
      b<<=1;
    }
    usleep(10000);
    if(c>=32 && c<=126) printf("%c",c);
    fflush(stdout);
    i++;
  }
  usleep(100000);
}

void showpart(void) {
  if(!strcmp(part,"?")) {
    fprintf(stderr,"Unknown part/Read error.\n");
    exit(1);
  }
  printf("0x%06X %s  Flash: %gKB(%d[%d]).  Fuses: L:0x%02X H:0x%02X E:0x%02X\n",signature,part,(double)flashsize/(double)1024,pages,pagesize,lfuse,hfuse,efuse);
//  printf("       Signature = 0x%06X\n",signature);
//  printf("      Flash size = %d\n",flashsize);
//  printf("       Page size = %d\n",pagesize);
//  printf("           Pages = %d\n",pages);
//  printf("           LFUSE = 0x%02X\n",lfuse);
//  printf("           HFUSE = 0x%02X\n",hfuse);
//  printf("           EFUSE = 0x%02X\n",efuse);
}

void showchar(char c) {
  if(c>=' ' &&c<127) putchar(c);
  if(c<32) printf("^%c",'A'+c-1);
}

void showstring(char *s) {
  int l;
  l=strlen(s);
  for(i=0;i<l;i++) showchar(s[i]);
}

uint8_t chat(char *sendstring, char ack) {
  char var[20];
  char val[20];
  int sendtimer;
  int sendtime;
  int mode=0;
  int index;
  char recb;
  char sb;
  int stringlen;
  int stringp;
  recb=0;
  index=0;
  stringp=0;
  mode=0;
  sendtime=2+115200/baud/2;
  sendtimer=0;
  if(verbose>3) {
    fprintf(stderr,"DCCD baud=%d, sendtime=%d, sendtimer=%d.\n",baud,sendtime,sendtimer);
  }
  if(verbose>2) {
    printf("Chat: Sending string \"");
    showstring(sendstring);
    printf("\"");
    if(ack) {
      printf("   expecting character '");
      showchar(ack);
      printf("'.\n");
    }
  }
  stringlen=strlen(sendstring);
  recb=0;
  while(recb!=ack || stringp<stringlen) {
    recb=getonebyte(1,0,1000);
    if(recb) {
      sendtimer=0;
      if(recb=='>') mode=1;
      if(recb=='=') mode=2;
      if(recb=='\r' || recb=='\n') mode=0;
      if(verbose>1) {
        putchar(recb);
        fflush(stdout);
      }
      switch(recb) {
        case '>':  mode=1; index=0; break;
        case '=':  if(mode==1) mode=2; index=0; break;
        case '\r': break;
        case '\n':
          mode=0;
          if(index!=0 && val[0]!=0 && var[0]!=0) {
            if(!strcmp(var,"PAGESIZE")) {
              pagesize=strtol(val,NULL,0);
            }
            if(!strcmp(var,"PAGESIZE")) {
              pagesize=strtol(val,NULL,0);
            }
            if(!strcmp(var,"PAGES")) {
              pages=strtol(val,NULL,0);
            }
            if(!strcmp(var,"PART")) {
              strcpy(part,val);
            }
            if(!strcmp(var,"SIGNATURE")) {
              signature=strtol(val,NULL,0);
            }
            if(!strcmp(var,"LFUSE")) {
              lfuse=strtol(val,NULL,0);
            }
            if(!strcmp(var,"EFUSE")) {
              efuse=strtol(val,NULL,0);
            }
            if(!strcmp(var,"HFUSE")) {
              hfuse=strtol(val,NULL,0);
            }
          }
          index=0; val[0]=var[0]=0;
          break;
        default:
          if(mode==1) {
            var[index++]=recb;
            var[index]=0;
          }
          if(mode==2) {
            val[index++]=recb;
            val[index]=0;
          }
      }
    } else {
      sendtimer++;
    }
    if(sendtimer>sendtime) {
      sendtimer=0;
      if(stringp<stringlen) {
        sb=sendstring[stringp++];
        if(sb>=32 && sb<127 && verbose>2) {
          printf("\033[7m%c\033[0m",sb); fflush(stdout);
        }
        write(comfd,&sb,1);
      }
    }
  }
  flashsize=pagesize*pages;
  return(0);
}

uint8_t chatreadflash(char *sendstring, char ack) {
  int address;
  char recb;
  char sb;
  int n,i,stringlen;
  int stringp;
  address=0;
  recb=0;
  n=0;
  stringp=0;
  stringlen=strlen(sendstring);
  while(recb!=ack || stringp<stringlen) {
    if(stringp<stringlen) {
      sb=sendstring[stringp++];
      write(comfd,&sb,1);
    }
    recb=getonebyte(1,0,10000);
    if(recb=='>') {
      if(verbose) printf("\r");
      for(i=0;i<pagesize;i++) {
        if(address>=flashsize) {
          fprintf(stderr,"Woah!  Read more data than flash is supposed to hold!\n.");
          return(1);
        }
        flashbuffer[address]=getonebyte(1,2,0);
        address++;
      }
      if(verbose==1) {
        printf("\r%06X  ",address);
        fflush(stdout);
      }
      i=getonebyte(1,1,0);
      if(i!='\r') {
        fprintf(stderr,"Did not receive expected carriage return at end of page.\n");
        return(1);
      }
      i=getonebyte(1,1,0);
      if(i!='\n') {
        fprintf(stderr,"Did not receive expected newline at end of page.\n");
        return(1);
      }
    } else {
      if(verbose>1) {
        if(recb) {
          putchar(recb);
          fflush(stdout);
        }
      }
    }
  }
  if(verbose>1) printf("\rDone.   \n");
  if(address!=flashsize) {
    fprintf(stderr,"Oops, only read %d bytes from flash sized %d bytes.\n",address,flashsize);
    return(1);
  }
  return(0);
}

void setbaud(int b) {
  switch(b) {
    case 0: speed = B0;break;
    case 50: speed = B50;break;
    case 75: speed = B75;break;
    case 110: speed = B110;break;
    case 150: speed = B150;break;
    case 300: speed = B300;break;
    case 600: speed = B600;break;
    case 1200: speed = B1200;break;
    case 2400: speed = B2400;break;
    case 4800: speed = B4800;break;
    case 9600: speed = B9600;break;
    case 19200: speed = B19200;break;
    case 38400: speed = B38400;break;
    case 57600: speed = B57600;break;
    case 115200: speed = B115200;break;
    default: fprintf(stderr,"Invalid baudrate.\n"); exit(1);
  }
  tcgetattr(comfd,&stbuf);
  stbuf.c_cflag&=~CBAUD;
  stbuf.c_cflag|=speed;
  if (tcsetattr(comfd, TCSANOW, &stbuf) < 0) {
    fprintf(stderr,"Can't tcsetattr set device.\n");
    exit(1);
  }
}

void sck(uint8_t v) {
  if(v) rtson();
  else rtsoff();
}

void mosi(uint8_t v) {
  if(v) dtron();
  else dtroff();
}

uint8_t miso(void) {
  readstatus();
  return(dsr);
}

void reseton(void) {
  ioctl(comfd,TIOCCBRK);
}

void resetoff(void) {
  ioctl(comfd,TIOCSBRK);
}

void resetpulse(void) {
  c=0;
  write(comfd,&c,1);
}

uint8_t xfr(uint8_t b) {
  uint8_t v;
  uint8_t i;
  uint8_t r;
  char buf[1];
  char bitshow[20];
  buf[0]=0x0;
  r=0;
  strcpy(bitshow,"76543210  76543210");
  if(verbose>2) printf("\r%02X->MISO %s MOSI->XX",b,bitshow); fflush(stdout);
  for(i=0;i<8;i++) {
    if(b&(128>>i)) {
      mosi(1);
      if(verbose>2) bitshow[i]='-';
    } else {
      mosi(0);
      if(verbose>2) bitshow[i]='_';
    }
    usleep(idelay);
    sck(1);
    usleep(idelay);
    v=miso();
    sck(0);
    if(v) {
      if(verbose>2) bitshow[i+10]='-';
      r|=(128>>i);
    } else {
      if(verbose>2) bitshow[i+10]='_';
    }
    if(verbose>2) {
      printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
      printf("%s MOSI->%02X",bitshow,r);
    }
    fflush(stdout);
  }
  if(verbose>2) printf("\n");
  return(r);
}

int xfrxpct(uint8_t b, uint8_t r) {
  uint8_t rb;
  rb=xfr(b);
  if(rb!=r) {
    fprintf(stderr,"Readback error, expected %02x, got %02x.\n",r,rb);
    return(1);
  }
  return(0);
}

void readbackerror(void) {
  fprintf(stderr,"Read back error.\n");
  exit(1);
}

int8_t readlfuse(void) {
  if(xfrxpct(0x50,0x00)) return(1);
  if(xfrxpct(0x00,0x50)) return(1);
  if(xfrxpct(0x00,0x00)) return(1);
  lfuse=xfr(0x00);
  if(verbose>2) printf("LFUSE = 0x%02X\n",lfuse);
  return(0);
}

int8_t readefuse(void) {
  if(xfrxpct(0x50,0x00)) return(1);
  if(xfrxpct(0x08,0x50)) return(1);
  if(xfrxpct(0x00,0x08)) return(1);
  efuse=xfr(0x00);
  if(verbose>2) printf("EFUSE = 0x%02X\n",efuse);
  return(0);
}

int8_t readhfuse(void) {
  if(xfrxpct(0x58,0x00)) return(1);
  if(xfrxpct(0x08,0x58)) return(1);
  if(xfrxpct(0x00,0x08)) return(1);
  hfuse=xfr(0x00);
  if(verbose>2) printf("HFUSE = 0x%02X\n",hfuse);
  return(0);
}

int readsignature(void) {
  uint8_t b;
  if(sdsdp) return(0);           // This is all done by inq on SDP module.
  signature=0;
  for(i=0;i<3;i++) {
    if(xfrxpct(0x30,0x00)) return(1);
    if(xfrxpct(0x00,0x30)) return(1);
    if(xfrxpct(i,0x00)) return(1);
    signature<<=8;
    b=xfr(0x00);
    signature|=b;
    if(verbose>1) printf("Signature byte %d = 0x%02X\n",i,b);
  }
  switch(signature) {
    case 0x1E9483: strcpy(part,"AT90PWM[23]16"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9383: strcpy(part,"AT90PWM[23]B"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E9388: strcpy(part,"AT90PWM81"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E96C1: strcpy(part,"AT90SCR100"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9382: strcpy(part,"ATA6289"); pagesize=64; flashsize=8192; break;
    case 0x1E9781: strcpy(part,"CAN128"); pagesize=256; flashsize=0x1FFFF; break;
    case 0x1E9581: strcpy(part,"CAN32"); pagesize=256; flashsize=0x7FFF; break;
    case 0x1E9681: strcpy(part,"CAN64"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9703: strcpy(part,"ATmega1280"); pagesize=256; flashsize=0x1FFFF; break;
    case 0x1E9704: strcpy(part,"ATmega1281"); pagesize=256; flashsize=0x1FFFF; break;
    case 0x1E9705: strcpy(part,"ATmega1284P"); pagesize=256; flashsize=0x1FFFF; break;
    case 0x1EA701: strcpy(part,"ATmega128RFA1"); pagesize=256; flashsize=0x1ffff; break;
    case 0x1E9401: strcpy(part,"ATmega161"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9404: strcpy(part,"ATmega162"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9402: strcpy(part,"ATmega163"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E940F: strcpy(part,"ATmega164"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9407: strcpy(part,"ATmega165[P]"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E940B: strcpy(part,"ATmega168P"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9405: strcpy(part,"ATmega169[P][A]"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9403: strcpy(part,"ATmega16[A]"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E940C: strcpy(part,"ATmega16HVA"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E940E: strcpy(part,"ATmega16HVA2"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E940D: strcpy(part,"ATmega16HVB"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9484: strcpy(part,"ATmega16M1"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9489: strcpy(part,"ATmega16U2"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9488: strcpy(part,"ATmega16U4"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9801: strcpy(part,"ATmega2560"); pagesize=256; flashsize=0x3FFFF; break;
    case 0x1E9802: strcpy(part,"ATmega2561"); pagesize=256; flashsize=0x3FFFF; break;
    case 0x1E9502: strcpy(part,"ATmega32"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9501: strcpy(part,"ATmega323"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9511: strcpy(part,"ATmega324PA"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9505: strcpy(part,"ATmega325"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9506: strcpy(part,"ATmega3250"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E950F: strcpy(part,"ATmega328P"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9503: strcpy(part,"ATmega329"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9504: strcpy(part,"ATmega3290"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9586: strcpy(part,"ATmega32C1"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9510: strcpy(part,"ATmega32HVB"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9584: strcpy(part,"ATmega32M1"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E958A: strcpy(part,"ATmega32U2"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9587: strcpy(part,"ATmega32U4"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9588: strcpy(part,"ATmega32U6"); pagesize=128; flashsize=0x7FFF; break;
    case 0x1E9507: strcpy(part,"ATmega406"); pagesize=128; flashsize=0x9FFF; break;
    case 0x1E920A: strcpy(part,"ATmega48P"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E9608: strcpy(part,"ATmega640"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9609: strcpy(part,"ATmega644"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E960A: strcpy(part,"ATmega644P[A]"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9605: strcpy(part,"ATmega645"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9606: strcpy(part,"ATmega6450"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9603: strcpy(part,"ATmega649"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9604: strcpy(part,"ATmega6490"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E960B: strcpy(part,"ATmega649"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9686: strcpy(part,"ATmega64C1"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9610: strcpy(part,"ATmega64HVE"); pagesize=128; flashsize=0xFFFF; break;
    case 0x1E9684: strcpy(part,"ATmega64M1"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E9307: strcpy(part,"ATmega8"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E9306: strcpy(part,"ATmega8515"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E9308: strcpy(part,"ATmega8535"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E930F: strcpy(part,"ATmega88P[A]"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E9389: strcpy(part,"ATmega8U2"); pagesize=128; flashsize=0x1FFF; break;
    case 0x1E9003: strcpy(part,"ATtiny10"); pagesize=32; flashsize=0x3FF; break;
    case 0x1E9007: strcpy(part,"ATtiny13"); pagesize=32; flashsize=1024; break;
    case 0x1E9487: strcpy(part,"ATtiny167"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E910F: strcpy(part,"ATtiny20"); pagesize=64; flashsize=0x7FF; break;
    case 0x1E910A: strcpy(part,"ATtiny2313[A]"); pagesize=32; flashsize=0x7FF; break;
    case 0x1E910B: strcpy(part,"ATtiny24[A]"); pagesize=32; flashsize=0x7FF; break;
    case 0x1E9108: strcpy(part,"ATtiny25"); pagesize=32; flashsize=0x7FF; break;
    case 0x1E910C: strcpy(part,"ATtiny261[A]"); pagesize=32; flashsize=0x7FF; break;
    case 0x1E900A: strcpy(part,"ATtiny4"); pagesize=32; flashsize=0x1FF; break;
    case 0x1E920E: strcpy(part,"ATtiny40"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E920D: strcpy(part,"ATtiny4313"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E920C: strcpy(part,"ATtiny43U"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E9207: strcpy(part,"ATtiny44[A]"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E9206: strcpy(part,"ATtiny45"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E9208: strcpy(part,"ATtiny461[A]"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E9209: strcpy(part,"ATtiny48"); pagesize=64; flashsize=0xFFF; break;
    case 0x1E9009: strcpy(part,"ATtiny5"); pagesize=32; flashsize=0x1FF; break;
    case 0x1E930C: strcpy(part,"ATtiny84[A]"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E930B: strcpy(part,"ATtiny85"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E930D: strcpy(part,"ATtiny861[A]"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E9387: strcpy(part,"ATtiny87"); pagesize=128; flashsize=0x1FFF; break;
    case 0x1E9311: strcpy(part,"ATtiny88"); pagesize=64; flashsize=0x1FFF; break;
    case 0x1E9008: strcpy(part,"ATtiny9"); pagesize=32; flashsize=0x3FF; break;
    case 0x1E9782: strcpy(part,"AT90USB128[67]"); pagesize=256; flashsize=0x1FFFF; break;
    case 0x1E9482: strcpy(part,"AT90USB162"); pagesize=128; flashsize=0x3FFF; break;
    case 0x1E9682: strcpy(part,"AT90USB64[67]"); pagesize=256; flashsize=0xFFFF; break;
    case 0x1E974C: strcpy(part,"ATxmega128A1"); pagesize=512; flashsize=139264; break;
    case 0x1E9742: strcpy(part,"ATxmega128A3"); pagesize=512; flashsize=139264; break;
    case 0x1E9748: strcpy(part,"ATxmega128D3"); pagesize=512; flashsize=139264; break;
    case 0x1E9441: strcpy(part,"ATxmega16A4"); pagesize=256; flashsize=20480; break;
    case 0x1E9442: strcpy(part,"ATxmega16D4"); pagesize=256; flashsize=20480; break;
    case 0x1E9744: strcpy(part,"ATxmega192A3"); pagesize=512; flashsize=204800; break;
    case 0x1E9749: strcpy(part,"ATxmega192D3"); pagesize=512; flashsize=204800; break;
    case 0x1E9842: strcpy(part,"ATxmega256A3"); pagesize=512; flashsize=270336; break;
    case 0x1E9843: strcpy(part,"ATxmega256A3B"); pagesize=512; flashsize=270336; break;
    case 0x1E9844: strcpy(part,"ATxmega256D3"); pagesize=512; flashsize=270336; break;
    case 0x1E9541: strcpy(part,"ATxmega32A4"); pagesize=256; flashsize=36864; break;
    case 0x1E9542: strcpy(part,"ATxmega32D4"); pagesize=256; flashsize=36864; break;
    case 0x1E964E: strcpy(part,"ATxmega64A1"); pagesize=256; flashsize=69632; break;
    case 0x1E9642: strcpy(part,"ATxmega64A3"); pagesize=256; flashsize=69632; break;
    case 0x1E964A: strcpy(part,"ATxmega64D3"); pagesize=256; flashsize=69632; break;
    default: fprintf(stderr,"Unknown part!\n"); return(1);
  }
  if(flashsize) {
    flashsize++;
    pages=flashsize/pagesize;
  }
  if(readlfuse()) return(1);
  if(readhfuse()) return(1);
  if(readefuse()) return(1);
  return(0);
}

uint programinit(void) {
  int n;
  char tmpstr[64];
  if(sdsdp) {
    dtron();
    for(i=0;i<20;i++) {
      getonebyte(1,0,1000); // Flush input and delay a little.
    }
    dtroff();
    usleep(10000);
    chat("\r",'#');
    usleep(10000);
    if(pbaud!=baud && pbaud!=0) {
      if(verbose>2) printf("Switching temporary baud rate to %d.\n",pbaud); fflush(stdout);
      sprintf(tmpstr,"sbr%d\r",16000000/5/pbaud);
      chat(tmpstr,'#');
      usleep(20000);
      baud=pbaud;
      setbaud(baud);
      if(verbose>2) printf("Switched.\n"); fflush(stdout);
      usleep(20000);
    }
    sprintf(tmpstr,"ver%d\r",verbose);
    chat(tmpstr,'#');
    sprintf(tmpstr,"del%d\r",idelay);
    chat(tmpstr,'#');
    chat("inq\r",'#');
    if(verbose>1) printf("\n");
  } else {
    sck(0);
    reseton();
    if(verbose>1) printf("Entering AVR programming mode.\n"); fflush(stdout);
    n=0;
    while(n<5) {
      usleep(200000);
      resetpulse();
      usleep(50000);
      xfr(0xAC);
      xfr(0x53);
      if(!xfrxpct(0x00,0x53)) n=10;
      xfr(0x00);
      n++;
    }
    if(n<10) return(1);
    if(verbose>1) printf("Programming enabled.\n");
  }
  readsignature();
  if(verbose) {
    showpart();
  }
  return(0);
}

int writelfuse(uint8_t v) {
  char tmpstr[64];
  if(verbose) printf("Programming LFUSE with value 0x%02X.\n",v);
  if(sdsdp) {
    sprintf(tmpstr,"lfu0x%02X\r",v);
    chat(tmpstr,'#');
    chat("inq\r",'#');
    if(verbose>1) printf("\n");
  } else {
    xfr(0xAC);
    if(xfrxpct(0xA0,0xAC)) return(1);
    if(xfrxpct(0x00,0xA0)) return(1);
    xfr(v);
  }
  return(0);
}

int writehfuse(uint8_t v) {
  char tmpstr[64];
  if(verbose) printf("Programming HFUSE with value 0x%02X.\n",v);
  if(sdsdp) {
    sprintf(tmpstr,"hfu0x%02X\r",v);
    chat(tmpstr,'#');
    chat("inq\r",'#');
    if(verbose>1) printf("\n");
  } else {
    xfr(0xAC);
    if(xfrxpct(0xA8,0xAC)) return(1);
    if(xfrxpct(0x00,0xA8)) return(1);
    xfr(v);
  }
  return(0);
}

int writeefuse(uint8_t v) {
  char tmpstr[64];
  if(verbose>1) printf("Programming EFUSE with value 0x%02X.\n",v);
  if(sdsdp) {
    sprintf(tmpstr,"efu0x%02X\r",v);
    chat(tmpstr,'#');
    chat("inq\r",'#');
    if(verbose>1) printf("\n");
  } else {
    xfr(0xAC);
    if(xfrxpct(0xA4,0xAC)) return(1);
    if(xfrxpct(0x00,0xA4)) return(1);
    xfr(v);
  }
  return(0);
}

int waitbusy() {
  // I tried polling but it caused problems.  Will have to re-visit.
  usleep(10000);
  return(0);
}

int erasechip(void) {
  if(verbose>0) {
    printf("Performing Chip Erase.\n");
  }
  if(sdsdp) {
    chat("era\r",'#');
  } else {
    if(xfrxpct(0xAC,0x00)) return(1);
    if(xfrxpct(0x80,0xAC)) return(1);
    if(xfrxpct(0x00,0x80)) return(1);
    if(xfrxpct(0x00,0x00)) return(1);
  }
  return(0);
}

uint8_t nibble(char c) {
  if(c>='0' && c<='9') return(c-'0');
  if(c>='a' && c<='f') return(c-'a'+10);
  if(c>='A' && c<='F') return(c-'A'+10);
  return(0);
}

uint8_t hexbyte(char m, char l) {
  return((nibble(m)<<4)|nibble(l));
}

int ihexread(uint8_t *buf, int maxlen, FILE *filep) {
  uint8_t b,len,m,l,checksum,type;
  int i,j,address,maxaddress=0;
  int linen=0;
  char line[600];
  while((fgets(line,598,filep))!=NULL) {
    linen++;
    if(!(line[0]==':')) {
      fprintf(stderr,"Line %d isn't an Intel Hex record.\n",linen);
      return(-1);
    } else {
      checksum=0;
      i=1;
      len=hexbyte(line[i],line[i+1]); i+=2;
      checksum+=len;
      m=hexbyte(line[i],line[i+1]); i+=2;
      checksum+=m;
      l=hexbyte(line[i],line[i+1]); i+=2;
      checksum+=l;
      address=(m<<8)|l;
      type=hexbyte(line[i],line[i+1]); i+=2;
      checksum+=type;
      if(type!=0 && type!=1) {
        fprintf(stderr,"Can't handle record type %d in Index Hex record in line %d.\n",type,linen);
        exit(1);
      }
      if(len!=0 && type==0) {
        for(j=0;j<len;j++) {
          if(address>=maxlen) {
            fprintf(stderr,"Intel hex record at address %04X in line %d is larger then maximum flashsize of %d bytes.\n",address,linen,maxlen);
            exit(1);
          }
          b=hexbyte(line[i],line[i+1]); i+=2;
          checksum+=b;
          buf[address++]=b;
          if(address>maxaddress) {
            maxaddress=address;
          }
        }
      }
      checksum^=0xFF;
      checksum++;
      b=hexbyte(line[i],line[i+1]); i+=2;
      if(b!=checksum) {
        fprintf(stderr,"Checksum error @ line %d, expected %d, got %d.\n",linen,checksum,b);
        return(-1);
      }
    }
  }
  return(maxaddress);
}

int ihexwrite(uint8_t *buf, int len, FILE *filep) {
  int i,address=0;
  uint8_t checksum;
  char out[80];
  char byte[3];
  while(address<len) {
    i=0;
    sprintf(out,":XX%04X00",address);
    checksum=((address>>8)&0xFF)+(address&0xFF)+i;
    while(i<16 && address<len) {
      checksum+=buf[address];
      sprintf(byte,"%02X",buf[address++]);
      strcat(out,byte);
      i++;
    }
    sprintf(byte,"%02X",i);
    out[1]=byte[0];
    out[2]=byte[1];
    checksum+=i;
    checksum^=0xFF;
    checksum++;
    fprintf(filep,"%s%02X\r\n",out,checksum);
  }
  fprintf(filep,":00000001FF\r\n");
  return(0);
}

uint8_t writeprogram() {
  if(verbose) printf("\rWriting %d bytes (0x%X bytes) to file \"%s\".\n",programend+1,programend+1,filename);
  fp=fopen(filename,"w");
  if(fp==NULL) {
    fprintf(stderr,"Can't open file \"%s\" for writing.\n",filename);
    return(1);
  }
  if(raw) fwrite(flashbuffer,1,programend+1,fp);
  else ihexwrite(flashbuffer,programend+1,fp);
  fclose(fp);
  free(flashbuffer);
  return(0);
}

int readprogram(void) {
  FILE *fp;
  if(flashsize==0) {
    fprintf(stderr,"Flash size unknown.\n");
    return(1);
  }
  flashbuffer=(uint8_t *)malloc(flashsize);
  memset(flashbuffer,0xFF,flashsize);
  fp=fopen(filename,"r");
  if(fp==NULL) {
    fprintf(stderr,"Can't open file \"%s\" for reading.\n",filename);
    return(1);
  }
  if(raw) programend=(int)fread(flashbuffer,1,flashsize,fp);
  else programend=ihexread(flashbuffer,flashsize,fp);
  fclose(fp);
  if(verbose>0) printf("Read 0x%06X bytes program from file \"%s\".\n",programend,filename);
  return(0);
}

int writeflash(void) {
  char tmpstr[64];
  uint8_t hbyte,lbyte;
  int npages;
  int c=0;
  int i,m,l;
  int address=0;
  if(readprogram()) return(1);
  if(programend==0) {
    fprintf(stderr,"Flash program of zero length.\n");
    return(1);
  }
  npages=programend/pagesize+1;
  if(verbose) printf("Flashing 0x%06X bytes in %d pages of %d bytes.\n",programend,npages,pagesize);
  erasechip();
  if(sdsdp) {
    sprintf(tmpstr,"wri%d\r",npages);
    chat(tmpstr,'<');
    for(l=0;l<npages;l++) {
      if(verbose) {
        printf("\rPage %d of %d  ",l+1,npages);
        fflush(stdout);
      }
      for(i=0;i<pagesize;i++) {
        c=flashbuffer[address++];
        write(comfd,&c,1);
      }
      c=getonebyte(1,1,0);
      if((c!='-' && l==npages-1) || (c!='<' && l!=npages-1)) {
        fprintf(stderr,"\r\nGot unexpected character %d after sending one page.\r\n",c);
        return(1);
      }
    }
    if(c!='-') {
      fprintf(stderr,"Did not get final acknowledge.\n");
      return(1);
    }
    if(verbose) printf("\rFlashed %d pages.\n",npages);
  } else {
    c=0;
    while(address<programend) {
      usleep(10000);
      for(i=0;i<pagesize;i+=2) {
        if(c==0) {
          if(verbose>1) printf("0x%06X ",address+i*2); fflush(stdout);
        }
        lbyte=flashbuffer[(address+i)];
        hbyte=flashbuffer[(address+i+1)];
        m=((i>>1)&0XFF00)>>8;
        l=((i>>1)&0x00FF)&0X00FF;
        xfr(0x40);
        if(xfrxpct(m,0x40)) return(1);
        xfr(l);
        xfr(lbyte);
        if(verbose>1) {
          printf("%02X",lbyte); fflush(stdout);
        }
        xfr(0x48);
        if(xfrxpct(m,0x48)) return(1);
        xfr(l);
        xfr(hbyte);
        if(verbose>1) {
          printf("%02X",hbyte); fflush(stdout);
        }
        if(++c==16) {
          c=0;
          if(verbose>1) printf("\n");
        }
      }
      m=((address>>1)&0XFF00)>>8;
      l=((address>>1)&0X00FF);
      if(verbose>0) {
        printf("Writing page #%d of %d.\n",address/pagesize,programend/pagesize);
      }
      xfr(0x4C);
      if(xfrxpct(m,0x4C)) return(1);
      xfr(l);
      xfr(0x00);
      address+=pagesize;
    }
    usleep(10000);
  }
  free(flashbuffer);
  if(verbose) printf("Done.\n");
  return(0);
}

int readflash(void) {
  uint8_t hbyte,lbyte;
  int r;
  int address=0;
  if(flashsize==0) {
    fprintf(stderr,"Flash size unknown.\n");
    return(1);
  }
  flashbuffer=(uint8_t *)malloc(flashsize);
  if(verbose>0) printf("Reading 0x%06X bytes of flash contents to memory.\n",flashsize);
  address=0;
  if(sdsdp) {
    chatreadflash("REA\r",'#');
  } else {
    while(address<flashsize/2) {
      if(verbose>0) {
        if((address&15)==0) {
          printf("%06X ",address<<1); fflush(stdout);
        }
      }
      xfr(0x28);
      if(xfrxpct(address>>8,0x28)) return(1);
      xfr(address&0xFF);
      hbyte=xfr(0);
      if(verbose>0) printf("--%02X",hbyte); fflush(stdout);
      xfr(0x20);
      if(xfrxpct(address>>8,0x20)) return(1);
      xfr(address&0xFF);
      lbyte=xfr(0);
      printf("\b\b\b\b%02X%02X",lbyte,hbyte); fflush(stdout);
      flashbuffer[address<<1]=lbyte;
      flashbuffer[(address<<1)+1]=hbyte;
      address++;
      if(verbose>0) {
        if((address&15)==0) {
          printf("\n");
        }
      }
    }
  }
  if(verbose>1) printf("\n");
  programend=flashsize-1;
  while(programend>0 && flashbuffer[programend]==0xFF) programend--;
  programend|=1;
  r=writeprogram();
  return(r);
}

void opendev(void) {
  if(device[0]!='/') {
    sprintf(buffer,"/dev/tty%s",device);
    strcpy(device,buffer);
  }
  if(device[0]) {
    if ((comfd = open(device, O_RDWR|O_EXCL|O_NDELAY)) <0) {
      fprintf(stderr,"Can't open device %s.\n",device);
      exit(1);
    }
    if(verbose>1) printf("Opened serial device %s.\n",device);
    if(dtr) dtron();
    else dtroff();
    if(rts) rtson();
    else rtsoff();
    if(invertdtr) dtrtoggle();
    tcgetattr(comfd,&stbuf);
    stbuf.c_cflag &= ~(CBAUD | CSIZE | CSTOPB | CLOCAL | PARENB | CRTSCTS );
    stbuf.c_cflag |= (speed | bits | CREAD | clocal | parity | stopbits | crtscts );
    stbuf.c_iflag &= ~(IGNCR | ICRNL | IUCLC | INPCK | IXON | IXANY | IGNPAR );
    stbuf.c_oflag &= ~(OPOST | OLCUC | OCRNL | ONLCR | ONLRET);
    stbuf.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOE | ECHONL);
    stbuf.c_lflag &= ~(ECHO | ECHOE);
    stbuf.c_cc[VMIN] = 1;
    stbuf.c_cc[VTIME] = 0;
    stbuf.c_cc[VEOF] = 1;
    if (tcsetattr(comfd, TCSANOW, &stbuf) < 0) {
      fprintf(stderr,"Can't tcsetattr set device.\n");
      exit(1);
    }
    setbaud(baud);
  }
}

void doklock(void) {
  while(1) {
    gettimeofday(&tvs,0);
    ts=localtime(&tvs.tv_sec);
    usleep(1000000-tvs.tv_usec);
    bigassclock();
    if(audio) {
      printf("\007");
      fflush(stdout);
    }
  }
}

void dopulsedtr(void) {
  while(samples) {
    gettimeofday(&tvs,0);
    if(!(tvs.tv_sec%pulsedtr)) {
      dtrtoggle();
      fprintf(stderr,"X");
      if(audio) fprintf(stderr,"\007");
      usleep(100000);
      dtrtoggle();
      gettimeofday(&tvs,0);
      samples--;
    } else {
      fprintf(stderr,".");
    }
    usleep(1000100-tvs.tv_usec);
  }
  fprintf(stderr,"\n");
  exit(0);
}

void dotictac() {
  if(tictacx) {
    xmittictacstring();
    printf("\n");
  }
  if(tictacw) {
    gettimeofday(&tvs,0);
    usleep(1000000-tvs.tv_usec);
    t=time(0)+6;
    ts=localtime(&t);
    i=0;
    sprintf(tictacstring,"T%02d%02d%02d%02d%02d%02d%02d%0d",
        ts->tm_hour, ts->tm_min, ts->tm_sec, ts->tm_mday,ts->tm_mon+1,
        (ts->tm_year+1900)/100, ts->tm_year%100, ts->tm_wday);
    xmittictacstring();
    printf("  SYNC..."); fflush(stdout);
    while(time(0)!=t) usleep(100);
    dtrpulse(1000,0);
    printf("\b\b\b\b\b\b\bTime set.\n");
  }
  if(tictacw|tictacx) exit(0);
  for(i=0;i<100;i++) {
    cdstatus();
    usleep(1000);
  }
  exit(0);
}

void dotiming(void) {
  // Flush start noise.
  for(i=0;i<10;i++) {
    getonebyte(1,0,1000);
    readstatus();
  }
  if(longterm) {
    sprintf(filename,"%s/.rs232u-ltmeasure.txt",getenv("HOME"));
    if(longterm==1) {
      printf("Waiting for pulse..."); fflush(stdout);
      getonesample();
      gettimeofday(&tvs,0);
      if(audio) printf("\007");
      printf("\n");
      fp=fopen(filename,"w");
      fprintf(fp,"%ld %6ld\n",tvs.tv_sec,tvs.tv_usec);
      fclose(fp);
      printf("Timestamp %ld.%06ld written in %s.\n",tvs.tv_sec,tvs.tv_usec,filename);
      exit(0);
    }
    if(longterm==2) {
      fp=fopen(filename,"r");
      fscanf(fp,"%ld %ld",&tvs.tv_sec,&tvs.tv_usec);
      fclose(fp);
      while(samples--) {
        getonesample();
        gettimeofday(&tvn,0);
        if(audio) printf("\007"); fflush(stdout);
        tvdiff(&tvn,&tvs,&tvd);
        n=tvd.tv_sec;
        if(tvd.tv_usec>500000) n++;
        n=(n+period/2)/period*period;
        printf("E: %8ld.%06ld  P: %-8d  ",tvd.tv_sec,tvd.tv_usec,n/period);
        f=((long double)tvd.tv_sec+(long double)tvd.tv_usec/1000000)/(long double)n;
        printf(" F: %0.13LF",f);
        if(printdrift) doprintdrift(f,f_cpu);
        printf("\n");
      }
      exit(0);
    }
  }
  getonesample();
  gettimeofday(&tvs,0);
  bcopy(&tvs,&tvp,sizeof(tvs));
  n=0;
  if(!timestamp) printf("    Sample     Interval      Elapsed     Clock            Average %s\n",device);
  while(samples--) {
    getonesample();
    gettimeofday(&tvn,0);
    tvdiff(&tvn,&tvp,&tvd);
    bcopy(&tvn,&tvp,sizeof(tvn));
    if(audio) printf("\007"); fflush(stdout);
    if(timestamp) {
      printf("%8d %ld.%06d  %ld.%06d\n",n,(long int)tvn.tv_sec,(int)tvn.tv_usec,(long int)tvd.tv_sec,(int)tvd.tv_usec);
      fflush(stdout);
      n++;
    } else {
      printf("\r");
      if(interval) usleep(interval*1000);
      printf("%c ",twirl[twirlc++]);
      if(twirlc>3) twirlc=0;
      printf("%8d",++n);
      tvdiff(&tvn,&tvp,&tvd);
      ptv(0,&tvd);
      bcopy(&tvn,&tvp,sizeof(tvn));
      tvdiff(&tvn,&tvs,&tvd);
      ptv(0,&tvd);
      s=tvd.tv_sec;
      h=(s/3600);
      s-=(h*3600);
      m=(s/60);
      s-=(m*60);
      printf("  %02d:%02d:%02d",h,m,s);
      f=((long double)tvd.tv_sec+(long double)tvd.tv_usec/1000000)/(long double)n;
      printf("%19.13LF",f);
      if(printdrift) doprintdrift(f,f_cpu);
      printf(" "); fflush(stdout);
    }
  }
  if(!timestamp) printf("\n");
  usleep(1000); // Without this, stdout freezes a bit at close.  No idea why.
}

// Triggered by ^U from bbdemo's osccal range measurement function,
// this routine expects ^U, F_CPU, ^M, 0.5sec_delay, F. It then prints
// the measured frequency estimate from the length of the delay.
void measureoscal(void) {
  struct timeval tvs,tve,tvd;
  long double f;
  int n;
  int fcpu=0;
  unsigned char c;
  c=0;
  n=getonebyte(1,9999,0)-'0';
  if(n>=0 && n<=5) {
    n=1<<n;
    while(c!=10 && c!=13) {
      c=getonebyte(1,9999,0);
      if(c>='0' && c<='9') {
        fcpu=fcpu*10+c-'0';
      } else {
        if(c!=10 && c!=13) return;
      }
    }
    gettimeofday(&tvs,0);
    if(fcpu) {
      if(fcpu!=32768) fcpu/=5;
      c=getonebyte(1,9999,0);        // Should be Finish.
      gettimeofday(&tve,0);
      if(c=='F') {
        tvdiff(&tve,&tvs,&tvd);
        f=(long double)fcpu/((long double)tvd.tv_sec+(long double)tvd.tv_usec/(long double)1000000)*n;
        printf(" %8.0LF",f);
        if(capture) fprintf(capturefp," %8.0LF\n",f);
        printf("\r\n");
      }
    }
  }
}

// Triggered by bbdemo's frequency measurement function, this routine
// expects ^T, F_CPU, ^M, and then periodic periods (.) separated by
// 0.5 second delays.
void measurefreq(void) {
  int n,rollerp;
  struct timeval tvs,tvn,tvp,tvd;
  long double f;
  int phaselength=5;
  int fcpu=0;
  unsigned char c;
  unsigned char roller[]={"/-\\|"};
  rollerp=0;
  n=0;
  c=0;
  while(c!=10 && c!=13) {
    c=getonebyte(1,9999,0);
    if(c>='0' && c<='9') {
      fcpu=fcpu*10+c-'0';
    } else {
      if(c!=10 && c!=13) return;
    }
  }
  if(fcpu==32768) phaselength=1;
  gettimeofday(&tvs,0);
  bcopy(&tvs,&tvp,sizeof(tvs));
  printf("     Pulse      Elapsed Interval      Average       Frequency\n");
  while(1) {
    printf("\r");
    c=getonebyte(3,9999,0);
    gettimeofday(&tvn,0);
    if(c!='.') return;
    printf("%c",roller[rollerp++]);
    if(rollerp==4) rollerp=0;
    printf(" %8d",++n);        // Print pulse count.
    tvdiff(&tvn,&tvs,&tvd);
    ptv(0,&tvd);               // Print elapsed time.
    tvdiff(&tvn,&tvp,&tvd);
    ptv(1,&tvd);               // Print interval since last (1=shortform);
    tvdiff(&tvn,&tvs,&tvd);
    f=((long double)tvd.tv_sec+(long double)tvd.tv_usec/1000000)/(long double)n;
    printf(" %12.10LF",f);     // Print interval average.
    f=(long double)fcpu/(long double)phaselength/f;
    printf(" %15.5LF",f);        // Print estimated frequency.
    printf("   "); fflush(stdout);
    bcopy(&tvn,&tvp,sizeof(tvn));
  }
}

void sendtime(void) {
  char timeb[10];
  time_t t;
  struct tm *lc;
  struct timeval tvn;
  printf("\033[7m[Sending Time]\033[0m"); fflush(stdout);
  gettimeofday(&tvn,0);
  if(tvn.tv_usec>1000000-(1000000/baud*11)) {
    t=time(0);
    while(time(0)==t) usleep(1000);
  }
  t=time(0)+2;
  lc=localtime(&t);
  sprintf(timeb,"%02d:%02d:%02d",lc->tm_hour,lc->tm_min,lc->tm_sec);
  for(i=0;i<strlen(timeb);i++) {
    usleep(20000);
    write(comfd,&timeb[i],1);
  }
  usleep(20000);
  while(time(0)!=t) usleep(1000);
  timeb[0]='\r';
  write(comfd,&timeb[0],1);
}

void printtimeod(time_t t, int usecs) {
  struct tm *ts;
  ts=localtime(&t);
  printf("%02d:%02d:%02d.%06d",ts->tm_hour,ts->tm_min,ts->tm_sec,usecs);
}

void setend(void) {
  gettimeofday(&now,NULL);
  diff.tv_sec=now.tv_sec-start.tv_sec;
  diff.tv_usec=now.tv_usec-start.tv_usec;
  if(now.tv_usec<start.tv_usec) {
    diff.tv_usec=1000000+now.tv_usec-start.tv_usec;
    diff.tv_sec--;
  }
  printf("\033[7m<L=");
  printtimeod(now.tv_sec,now.tv_usec);
  printf(">");
  printf("<E=%u.%06u>\033[0m",(unsigned int)diff.tv_sec,(unsigned int)diff.tv_usec);
  fflush(stdout);
}

void setstart(void) {
  gettimeofday(&start,NULL);
  printf("\033[7m<S=");
  printtimeod(start.tv_sec,start.tv_usec);
  printf(">\033[0m");
  fflush(stdout);
}

void captureoff(void) {
  if(capture) {
    fclose(capturefp);
    capture=0;
    printf("\033[7m[Capture OFF]\033[0m\r\n");
  }
}

void captureon(void) {
  if(capture==0 && capturefile[0]!=0) {
    capturefp=fopen(capturefile,"w");
    if(capturefile==NULL) {
      fprintf(stderr,"Could not open %s for writing",capturefile);
      bbexit(1);
    }
    capture=1;
    printf("\033[7m[Capture ON]\033[0m\r\n");
  }
}

void capturetoggle(void) {
  if(capture) captureoff();
  else captureon();
}

void bbsndbrk(void) {
  printf("\033[7m[BREAK]\033[0m");
  fflush(stdout);
  resetoff();
  usleep(800000);
  reseton();
}

void hexon(void) {
  hex=1;
  printf("\033[7m[HEX MODE ON]\033[0m"); fflush(stdout);
}

void hexoff(void) {
  hex=0;
  printf("\033[7m[HEX MODE OFF]\033[0m");  fflush(stdout);
}

void hextoggle(void) {
  if(hex) hexoff();
  else hexon();
}

int bbrec(void) {
  char c;
  int r;
  r=read(comfd,&c,1);
  if(r==0) return(0);
  if(r!=1) {
    fprintf(stderr,"Read() from serial device returned error value %d.\n",r);
    return(1);
  }
  if(noctrl) {
    if(!((c>31 && c<127) || c==8 || c==10 || c==13)) {
      c=0;
    }
  }
  if(c==20) { measureoscal(); c=0;}
  if(c==21) { measurefreq(); c=0;}
  if(c==29) { sendtime(); c=0;}
  if(c==22) { sendmacro=0; c=0; }
  if(c==23) { hexon(); c=0; }
  if(c==24) { hexoff(); c=0; }
  if(c==1) { setstart(); c=0; }
  if(c==2) { setend(); c=0; }
  if(c==5) { captureon(); c=0; }
  if(c==6) { captureoff(); c=0; }
  if(capture) {
    if((c>31 && c<127) || c==8 || c==10 || c==9 ) {
      fputc(c,capturefp);
    }
  }
  if(!!hex && (c<32 || c>126) && c!=13 && c!=10 && c!=8 && c!=9) {
    printf("\033[7m[");
    printf("%02x]\033[0m",(unsigned char)c);
  } else {
    // Prevent Control-S to console.
    if(c!=19) putchar(c);
    if(ocrnl!=0 && c==13) putchar(10);
  }
  fflush(stdout);
  return(0);
}

void bbheader(void) {
  printf(" \033[7m");
  for(i=0;i<75;i++) putchar(' ');
  printf(" \033[0m\r\n");
}

void bbpline(char *text) {
  int i;
  printf(" \033[7m  \033[0m %s",text);
  for(i=0;i<70-strlen(text);i++) putchar(' ');
  printf(" \033[7m  \033[0m\r\n");
}

void bbkbdhelp(void) {
  printf("\r\n");
  bbheader();
  bbpline("");
  bbpline("A:Send Control-A  D:Flip DTR       R:Toggle RTS      H:Help");
  bbpline("C:Toggle Capture  S:Start timer    E:End Timer       T:Toggle HEX");
  bbpline("M:Send macro");
  bbpline("");
  bbpline("Baud rate selection:");
  bbpline("");
  bbpline("0:50      1:75      2:110     3:150     4:300     5:600     6:1200");
  bbpline("7:2400    8:4800    9:9600    W:19200   X:38400   Y:57600   Z:115200");
  bbpline("");
  bbheader();
}

void bbsetbaud(int b) {
  baud=b;
  setbaud(baud);
  printf("\033[7m[Baud Rate=%d]\033[0m",baud);  fflush(stdout);
}


int bbkbd(void) {
  static int esc;
  char c;
  int r;
  r=read(0,&c,1);
  if(r==0) return(0);
  if(r!=1) {
    fprintf(stderr,"Read() from stdin(keyboard) returned error value %d.\n",r);
    return(1);
  }
  if(c==1 && esc==0) {
    esc=1;
    c=0;
  }
  if(esc==1) {
    if(c>='a' && c<='z') c=c-'a'+'A';
    if(c<32 && c>0) c=c-1+'A';
    switch(c) {
      case '0': bbsetbaud(50); break;
      case '1': bbsetbaud(75); break;
      case '2': bbsetbaud(110); break;
      case '3': bbsetbaud(150); break;
      case '4': bbsetbaud(300); break;
      case '5': bbsetbaud(600); break;
      case '6': bbsetbaud(1200); break;
      case '7': bbsetbaud(2400); break;
      case '8': bbsetbaud(4800); break;
      case '9': bbsetbaud(9600); break;
      case 'W': bbsetbaud(19200); break;
      case 'X': bbsetbaud(38400); break;
      case 'Y': bbsetbaud(57600); break;
      case 'Z': bbsetbaud(115200); break;
      case 'A': c=1; write(comfd,&c,1); break;
      case 'C': capturetoggle(); break;
      case 'D': dtrtoggle(); break;
      case 'M': sendmacro=0; break;
      case 'R': rtstoggle(); break;
      case 'S': setstart(); break;
      case 'E': setend(); break;
      case 'T': hextoggle(); break;
      case 'H': bbkbdhelp(); break;
      case 'Q': exit(0);
    }
    if(c) esc=0;
  } else {
    if(localecho) {
      if(esc==0) {
        putchar(c);
        fflush(stdout);
      }
    }
    write(comfd,&c,1);
  }
  return(0);
}

void dobbterm() {
  oldstatus=0;
  // printf("Type Control-A H to get help.\r\n");
  fflush(stdout);
  ioctl(0,TCGETA,&oricons);
  ioctl(0,TCGETA,&cons);
  signal(SIGKILL,killexit);
  signal(SIGINT,intexit);
  signal(SIGTERM,termexit);
  cons.c_iflag &= ~(INLCR|ICRNL|IUCLC|ISTRIP|IXON|BRKINT);
  cons.c_oflag &= ~OPOST;
  cons.c_lflag &= ~(ICANON|ISIG|ECHO);
  cons.c_cc[VMIN] = 1;
  cons.c_cc[VTIME] = 0;
  ioctl(0,TCSETA,&cons);
  atexit(bbatexit);
  c=0;
  readstatus();
  oldstatus=(cd<<5)+(dsr<<4)+(cts<<3)+(ri<<2)+(dtr<<1)+rts;
  while(1) {
    c=0;
    readstatus();
    status=(cd<<5)+(dsr<<4)+(cts<<3)+(ri<<2)+(dtr<<1)+rts;
    if(status!=oldstatus) {
      gettimeofday(&tvs,0);
      printf("\033[7m[%ld.%06ld CD=%d DSR=%d CTS=%d RI=%d DTR=%d RTS=%d ]\033[0m\r\n", \
             tvs.tv_sec,tvs.tv_usec,cd,dsr,cts,ri,dtr,rts);
      oldstatus=status;
    }
    timeout.tv_sec=0; timeout.tv_usec=10000;
    FD_ZERO(&FDS); FD_SET(0,&FDS); FD_SET(comfd,&FDS);
    if(select(comfd+1,&FDS,NULL,NULL,&timeout)) {
      if(FD_ISSET(comfd,&FDS)) {
        if(bbrec()) exit(1);
      }
      if(FD_ISSET(0,&FDS)) {
        if(bbkbd()) exit(1);
      }
    } else {
      if(sendmacro>=0) {
        if(macro[sendmacro]) {
          write(comfd,&macro[sendmacro],1);
          sendmacro++;
        } else {
          sendmacro=-1;
        }
      }
    }
  }
}

int doprogramming(void) {
  if(programinit()) {
    fprintf(stderr,"Could not enter AVR programming mode.\n");
    return(1);
  }
  if(pagesize==0) {
    fprintf(stderr,"Could not read page size.\n");
    return(1);
  }
  if(doreadflash) {
    readflash();
  }
  if(dowriteflash) {
    writeflash();
  }
  if(dowritelfuse) writelfuse(newlfuse);
  if(dowritehfuse) writehfuse(newhfuse);
  if(dowriteefuse) writeefuse(newefuse);
  if(dochiperase) erasechip();
  if(holdreset) {
    if(sdsdp) {
      if(holdreset==1) printf("The -e option has no effect when using the SDP module.\n");
    } else {
      resetoff();
      if(holdreset==1) {
        printf("\n");
        printf("RESET held disabled, the micro-controller should be running.\n");
        printf("Note that this means the serial port is sending a continuous BREAK\n");
        printf("signal that may prevent other programs from using it.  Run SD again\n");
        printf("without the -e option to stop.\n");
        printf("\n");
        printf("To prevent this warning from showing, use the e option twice (-ee).\n");
        printf("\n");
      }
    }
  }
  return(0);
}

int main(int argc, char *argv[]) {
  macro[0]=0;
  device[0]=0;
  if(getenv("SDDEV")) strcpy(device,getenv("SDDEV"));
  if(getenv("SDBAUD")) baud=atoi(getenv("SDBAUD"));
  if(getenv("SDPBAUD")) pbaud=atoi(getenv("SDPBAUD"));
  if(getenv("SDPIDELAY")) sdpidelay=atoi(getenv("SDPIDELAY"));
  strcpy(capturefile,"capture.txt");
  int parseargs=1;
  tictacstring[0]=0;
  n=1;
  while(parseargs!=0 && (option=getopt(argc,argv,"abd:hkpt"))!=-1) {
    switch(option) {
      case 'a': audio=1; break;
      case 'b': bbterm=1; parseargs=0; break;
      case 'd': strcpy(device,optarg); break;
      case 'h': help=1; break;
      case 'k': klock=1; parseargs=0; break;
      case 'p': programming=1; parseargs=0; break;
      case 't': timing=1; parseargs=0; break;
    }
  }
  if(timing) {
    while((option=getopt(argc,argv,"ab:d:fhi:ln:o:p:s:t:u:wxy:"))!=-1) {
      switch(option) {
        case 'a': audio=1; break;
        case 'b': baud=atoi(optarg); break;
        case 'd': strcpy(device,optarg); break;
        case 'f': invertdtr=1; break;
        case 'i': interval=atoi(optarg); break;
        case 'l': longterm=1; break;
        case 'n': samples=atoi(optarg); break;
        case 'o': printdrift=1; f_cpu=atoi(optarg); break;
        case 'p': longterm=2; period=atoi(optarg); break;
        case 's': signaltype=atoi(optarg); break;
        case 't': transition=atoi(optarg); break;
        case 'u': invertdtr=1; tictacx=1; strcpy(tictacstring,optarg); break;
        case 'w': invertdtr=1; tictacw=1; break;
        case 'x': timestamp=1; break;
        case 'y': pulsedtr=atoi(optarg); break;
        case 'h':
        default:  timingusage(); exit(1); break;
      }
    }
  }
  if(programming) {
    if(optind<argc) {
      while((option=getopt(argc,argv,"bB:d:ehi:mr:v:w:E:H:L:zs:v:"))!=-1) {
        switch(option) {
          case 'd': strcpy(device,optarg); break;
          case 'b': raw=1; break;
          case 'B': baud=strtol(optarg,NULL,0); break;
          case 'e': holdreset++; break;
          case 'i': idelay=atoi(optarg); break;
          case 'r': doreadflash=1; strcpy(filename,optarg); break;
          case 'v': verbose=atoi(optarg); break;
          case 'w': dowriteflash=1; strcpy(filename,optarg); break;
          case 'L': dowritelfuse=1; newlfuse=strtol(optarg,NULL,0); break;
          case 'H': dowritehfuse=1; newhfuse=strtol(optarg,NULL,0); break;
          case 'E': dowriteefuse=1; newefuse=strtol(optarg,NULL,0); break;
          case 'z': dochiperase=1; break;
          case 's': pbaud=strtol(optarg,NULL,0); break;
          case 'h':
          default:  programusage(); exit(1); break;
        }
      }
    }
  }
  if(bbterm) {
    while((option=getopt(argc,argv,"p:ab:fl:hnd:oem:c:xywt:"))!=-1) {
      switch(option) {
        case 'n': noctrl=1; break;
        case 'c': strcpy(capturefile,optarg); break;
        case 'b': baud=atoi(optarg); break;
        case 'd': strcpy(device,optarg); break;
        case 'o': ocrnl=1; break;
        case 'e': localecho=1; break;
        case 'x': clocal=0; break;
        case 'f': invertdtr=1; break;
        case 'y': crtscts=CRTSCTS; break;
        case 'a': stopbits=CSTOPB; break;
        case 'm': strcpy(macro,optarg); strcat(macro,"\r"); break;
        case 'w': hex=1; break;
        case 'h':
        default : bbtermusage(); exit(0); break;
      }
    }
  }
  if(help) {
    usage();
    exit(0);
  }
  if(klock) doklock();
  if(!device[0]) {
    fprintf(stderr,"Serial device not specified.\n");
    usage();
    exit(1);
  }
  i=strlen(device);
  if(i>4) {
    if(device[i-4]=='+') {
      if((device[i-1]=='P' && device[i-2]=='D' && device[i-3]=='S') || \
         (device[i-1]=='p' && device[i-2]=='d' && device[i-3]=='s')) {
        sdsdp=1;
        device[i-4]=0;
      }
    }
  }
  if(idelay==-1) {
    if(sdsdp) {
      idelay=10;
      if(sdpidelay>=0) idelay=sdpidelay;
    }
    else idelay=1000;
  }
  opendev();
  if(tictacw|tictacx) dotictac();
  if(programming) {
    if(doprogramming()) exit(1);
  }
  if(bbterm) dobbterm();
  if(pulsedtr) dopulsedtr();
  if(timing) dotiming();
  exit(0);
}
