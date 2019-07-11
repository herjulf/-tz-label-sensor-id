#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define DEVICE "/dev/usb/lp0"
#define ESC '\x1b'

int cw,w,h;
char *data=NULL;
int minx=1024,maxx=0,miny=16384,maxy=0;

char b[8]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
char *fname=NULL;
int devflag=0;

void
hexdump(unsigned char *s, int l)
{
	int i;
	for(i=0;i<l;i++)
	{
		fprintf(stderr," %02x",(unsigned int)(s[i]&0xff));
	}
	fprintf(stderr," [");
	for(i=0;i<l;i++)
	{
		if(isprint(s[i]))
			fputc(s[i],stderr);
		else
			fputc('.',stderr);
	}
	fprintf(stderr,"]\n");
}

int
send_command(int fd, unsigned char *wbuf, int len, unsigned char *rbuf)
{
	int r;
	r=write(fd,wbuf,len);
	if(r<0)
	{
		fprintf(stderr,"write(%s) failed errno=%d (%s)\n",fname,errno,strerror(errno));
		exit(1);
	}
	else
	{
#ifdef DEBUG
		fprintf(stderr,"wrote %d bytes:",r);
		hexdump(wbuf,r);
#endif
	}

	if(!devflag)
		return(r);

	r=read(fd,rbuf,255);
	if(r<0)
	{
		fprintf(stderr,"read(%s) failed errno=%d (%s)\n",fname,errno,strerror(errno));
		exit(1);
	}
	if(r>0)
	{
#ifdef DEBUG
		fprintf(stderr,"\nread %d bytes:",r);
		hexdump(rbuf,r);
#endif
	}
#if(1)
	fprintf(stderr,"\n");
	if(r==32)
	{
		fprintf(stderr,"Err info1/2    = 0x%02x, 0x%02x\n",rbuf[8],rbuf[9]);
		fprintf(stderr,"Media width,type,len  = 0x%02x,0x%02x,0x%02x\n",rbuf[10],rbuf[11],rbuf[17]);
		fprintf(stderr,"Status type  = 0x%02x\n",rbuf[18]);
		fprintf(stderr,"Phase type,num   = 0x%02x,0x%02x\n",rbuf[19],rbuf[20]*256+rbuf[21]);
		fprintf(stderr,"Notification = 0x%02x\n",rbuf[22]);
	}
#endif
	return(r);
}

int
read_p1(FILE *fp)
{
	char *d=data;
	int bn=0,c;
	int fw=0,fh=0;
	while((c=getc(fp))!=EOF)
	{
		switch(c)
		{
		case '1':
			*d|=b[bn];
			/* fall through */
		case '0':
			bn++;
			fw++;
			if(bn==8)
			{
				bn=0;
			}
			if((bn==0)||(fw==w))
			{
				d++;
				if(fw==w)
				{
					bn=0;
					fh++;
					fw=0;
				}
			}
		}
		if(fh==h)
			return(0);
	}
	return(1);
}

int
read_p4(FILE *fp)
{
	int r;
	r=fread(data,1,w*h,fp);
	if(r==(w*h))
		return(0);
	return(1);
}


int
getdata(FILE *fp,char *buf,int bmax)
{
	char *p;
	int n,c;
	p=buf;
	n=0;
	while((c=getc(fp))!=EOF)
	{
		if(isspace(c))
		{
			if((n==0)||(*buf!='#')||(c=='\n'))
			{
				*p='\0';
				return(n);
			}
		}
		*p++=c;
		n++;
		if(n==(bmax-1))
			break;
	}
	*p='\0';
	return(n);
}

int
gdata(FILE *fp,char *buf, int bmax)
{
	int n;
	while(1)
	{
		n=getdata(fp,buf,bmax);
		if((n>0)&&(*buf=='#'))
			fprintf(stderr,"PBM comment %s\n",buf);
		else
			return(n);
	}
}

int
readpbm(char *fname)
{
	char buf[256];
	int r,mode=0;
	FILE *fp;

	if(fname==NULL)
	{
		fp=stdin;
	}
	else
	{
		fp=fopen(fname,"r");
		if(fp==NULL)
		{
			perror(fname);
			exit(1);
		}
	}

	r=gdata(fp,buf,256);
	fprintf(stderr,"Got data [%s]\n",buf);
	if(strcmp(buf,"P1")==0)
		mode=1;
	if(strcmp(buf,"P4")==0)
		mode=4;
	if(mode==0)
	{
		fprintf(stderr,"unknown pbm magic %s\n",buf);
		exit(1);
	}
	
	r=gdata(fp,buf,256);
	fprintf(stderr,"Got data [%s]\n",buf);
	w=atoi(buf);
	r=gdata(fp,buf,256);
	fprintf(stderr,"Got data [%s]\n",buf);
	h=atoi(buf);
	fprintf(stderr,"Got pbm w=%d, h=%d\n",w,h);
	cw=(w+7)/8;

	fprintf(stderr,"Got cw=%d, \n", cw);

	data=calloc(cw*h,1);
	if(data==NULL)
	{
		fprintf(stderr,"Calloc(%d) failed\n",cw*h);
		exit(1);
	}
	if(mode==1)
		r=read_p1(fp);
	if(mode==4)
		r=read_p4(fp);
	
	return(0);
}

void
scanpbm(void)
{
	char *d=data;
	int i,j;
	for(i=0;i<h;i++)
	{
		d=data+cw*i;
		for(j=0;j<w;j++)
		{
			if(d[j/8]&b[j&0x07])
			{
				if(j>maxx)
					maxx=j;
				if(j<minx)
					minx=j;
				if(i>maxy)
					maxy=i;
				if(i<miny)
					miny=i;
#ifdef DEBUG
				fputc('X',stderr);
#endif
			}
#ifdef DEBUG
			else
				fputc('.',stderr);
#endif
		}
#ifdef DEBUG
		fputc('\n',stderr);
#endif
	}
#ifndef DEBUG
	fprintf(stderr,"minx,maxx=%d,%d miny,maxy=%d,%d\n",minx,maxx,miny,maxy);
#endif
}

int
main(int argc, char *argv[])
{
	int r,i;
	int dfd;
	unsigned char rbuf[256];
	unsigned char wbuf[256];
	char *fname=NULL;

	switch(argc)
	{
		case 1:
			readpbm(NULL);
			break;
		case 2:
			readpbm(argv[1]);
			break;
		case 3:
		case 4:
			if(strcmp(argv[1],"-f")==0)
			{
				fname=strdup(argv[2]);
				if(argc==4)
					readpbm(argv[3]);
				else
					readpbm(NULL);
				break;
			}
			/* fall through */
		default:
			fprintf(stderr,"Usage: %s [-f file] [file.pbm]\n",argv[0]);
			exit(1);
	}

	scanpbm();

	if(fname==NULL)
	{
		dfd=1;
		fname="stdout";
	}
	else
	{
		if(strncmp(fname,"/dev/",5)==0)
		{
			/* expect a device file to already exist */
			dfd=open(fname,O_RDWR);
			devflag=1;
		}
		else
		{
			dfd=open(fname,O_WRONLY|O_CREAT);
		}

		if(dfd<0)
		{
			fprintf(stderr,"open(%s) failed errno=%d (%s)\n",fname,errno,strerror(errno));
			exit(1);
		}
	}

	if(1) {
	  wbuf[0]=ESC;
	  wbuf[1]=0x40; // Clear Print Buffer
	  r=send_command(dfd,wbuf,2,rbuf);
	}

	if(1) {
	  wbuf[0]=ESC;
	  wbuf[1]=0x69; // 
	  wbuf[2]=0x53; // Send printer status
	  r=send_command(dfd,wbuf,3,rbuf);
	}

	if(1) {
	  wbuf[0]=ESC;
	  wbuf[1]=0x69; // 
	  wbuf[2]=0x52; // Set Transfer Mode
	  wbuf[3]=0x01;
	  r=send_command(dfd,wbuf,4,rbuf);
	}

	if(1) {
	  /* 
	     ESC i M # <br>(1b 69 4d ##)
	     bit 0-4: Feed amount (default=large): 0-7=none, 8-11=small,
	     12-25=medium, 26-31=large<br>
	     bit 6: Auto cut/cut mark (default=on): 0=off, 1=on<br>
	     bit 7: Mirror print (default=off): 0=off, 1=on.
	     (note that it seems that QL devices do not reverse the
	     data stream themselves, but rely on the driver doing it!)
	  */

	       wbuf[0]=ESC;
	       wbuf[1]=0x69; // 
	       wbuf[2]=0x4d; // Set Feed ammount
	       //wbuf[3]=0x40; 
	       /*
		 Mirror 80
		 Cut    40 Auto cut
		 
		*/
	       wbuf[3]=0x40;
	       r=send_command(dfd,wbuf,4,rbuf);
	}

	if(0) {
	  wbuf[0]=ESC;
	  wbuf[1]=0x69; // 
	  wbuf[2]=0x44; // Set Transfer Mode
	  wbuf[3]=0x11;
	  r=send_command(dfd,wbuf,4,rbuf);
	}


	if(0) {
	  wbuf[0]=ESC;
	  wbuf[1]=0x69; // 
	  wbuf[2]=0x7a; // Set media and quality 
	  wbuf[3]= 0x0;
	  wbuf[4]= 24;
	  wbuf[5]= 24;
	  wbuf[6]= 24;
	  wbuf[7]=0x0;
	  wbuf[8]=0x0;
	  r=send_command(dfd,wbuf,13,rbuf);
	}

	for(i=maxy;i>=miny;i--)
	{
		wbuf[0]='G';
		wbuf[1]=cw+4;
		wbuf[2]=0x00;	/* 32 pixel non printing left margin */
		wbuf[3]=0x00;
		wbuf[4]=0x00;
		wbuf[5]=0x00;
		wbuf[6]=0x00;
		memcpy(wbuf+7,data+cw*i,cw);
		r=send_command(dfd,wbuf,15,rbuf);
	}
		/* a few blank lines */

	if(0) {
	  wbuf[0]='Z';
	  for(i=0;i<10;i++)
	    r=send_command(dfd,wbuf,1,rbuf);
	}

	wbuf[0]=0x1a; // Eject print pbuffer. CUT.
	//wbuf[0]=0x0c; // Eject print pbuffer. No cut
	r=send_command(dfd,wbuf,1,rbuf);
	
	exit(0);
}
