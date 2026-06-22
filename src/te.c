#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>

struct termios original,rm;
int col,row;
int cx,cy;

enum KEY_ACTION{
  //all the key action that mention below is maped their respected ASCII value 
  //and this is all a soft code and not been implemented by the terminal
  KEY_NULL = 0,
  CTRL_C = 3,
  CTRL_D = 4,
  CTRL_H = 8,
  TAB = 9,
  ENTER = 13,
  CTRL_Q = 17,
  CTRL_S = 19,
  
  ESC = 27,
  BACKSPACE = 127,

  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  HOME_KEY,
  END_KEY,
};

void GetTerminalSize();
void EnableRawMode(); //Enable the raw mode 
int ReadKey(); // it read a keypressed and return the keypressed
int ProcessKey(int); //It process the keypressed 
void enterAltScreen(); //Enters to a alternate screen
void exitAltScreen(); //Exits the alternate screen
void DrawScreen(); //draws the screen 
void clearup(); //used to reset the terminal to original settings

//Main funtion 
int main(){
  atexit(clearup);
  GetTerminalSize();
  EnableRawMode();
  enterAltScreen();
  DrawScreen();
  int key; // A key press will be stored at a time 
  while (1){
    DrawScreen();
    key = ReadKey();
    int res = ProcessKey(key);
    if (res == 1) {
      exitAltScreen();
      break;
    }
  }

  return 0;


}

int ReadKey(){
  char chr;
  if (read(STDIN_FILENO,&chr,1) != 1 ) return KEY_NULL;
  switch(chr){
    case CTRL_Q : return CTRL_Q;
    case ENTER : return ENTER;
    case BACKSPACE: return BACKSPACE;
    case ESC:
      char seq[2];
      if (read(STDIN_FILENO,&seq[0],1) != 1) return ESC;
      if (read(STDIN_FILENO,&seq[1],1) != 1) return ESC;
      if (seq[0] == '[')
      {
        switch(seq[1]){
          case 'A' : return ARROW_UP;
          case 'B' : return ARROW_DOWN;
          case 'C' : return ARROW_RIGHT;
          case 'D' : return ARROW_LEFT;
          case 'F' : return HOME_KEY;
          case 'H' : return HOME_KEY;
        }
      }
    return ESC;
      
  }
  return chr;
}

int ProcessKey(int key){
  switch(key){
    case CTRL_Q: return 1;
    case ENTER:
      if (cx > 0){
        cy++; //move down a row
        cx=0; //move the cur to start of the line
      }
      break;

    //left,right,up and down cur update 
    case ARROW_UP: write(STDOUT_FILENO,"\033[A",3); break;
    case ARROW_DOWN: write(STDOUT_FILENO,"\033[B",3); break;
    case ARROW_RIGHT: write(STDOUT_FILENO,"\033[C",3); break;
    case ARROW_LEFT: write(STDOUT_FILENO,"\033[D",3); break;

    case BACKSPACE:
      if (cx>0){
        //gb_delete(&lines[cy]); 
        cx--;
      }
      break;

    default:
      if(!iscntrl(key)){printf("%c",key);fflush(stdout);}
      break;
  }
  return 0;
}

void GetTerminalSize(){
  struct winsize ws;
  ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws);
  row = ws.ws_row;
  col = ws.ws_col;
}

void enterAltScreen(){
  write(STDOUT_FILENO,"\033[?1049h",8); // the "\033[?1049h" is a ANSI Escape Sequences
}
void exitAltScreen(){
  write(STDOUT_FILENO,"\033[?1049l",8);
}
void DrawScreen(){
  write(STDOUT_FILENO,"\033[?25l",6);
  write(STDOUT_FILENO,"\033[H",3); //mov the cur to the start of the screen
  for (int i = 0 ; i<row-1 ; i++){
    write(STDOUT_FILENO,"-\r\n",3); //write the "~" at the left of the screen
  }

  //status bar;
  write(STDOUT_FILENO,"TE",2); 

  //repostion the cursor to the editor area
  char pos[32];
  snprintf(pos,sizeof(pos),"\033[%d;%dH",cy+1,cx+1);
  write(STDOUT_FILENO,pos,strlen(pos));
  write(STDOUT_FILENO,"\033[?25h",6); //shows the cursor
}

void EnableRawMode(){
  tcgetattr(STDIN_FILENO,&original); //Gets the terminal current settings
  rm = original; // rm - Raw Mode
  
  //local flag 
  rm.c_lflag &= ~ECHO; //disable echo;
  rm.c_lflag &= ~ICANON; //disable line input;
  rm.c_lflag &= ~IEXTEN; //disable extended input processing runing other shortcuts like ctrl+v
  //Input flag
  rm.c_iflag &= ~IXON; //disable software flow control
  rm.c_iflag &= ~ICRNL; // disable return carrage it convert \r to \n, so we disable it 
  
  //output flag
  //rw.c_oflag &= ~(OPOST); // output post processing (transform the output before printing it )
  
  //control flag
  rm.c_cflag |= (CS8);

  // control chars - set return condition: min number of bytes and timer.
  rm.c_cc[VMIN] = 0; //Return each bytes, or zero for timeout;
  rm.c_cc[VTIME] = 1; //100ms timeout
  
  //apply the changes to the struct
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&rm);
}

void clearup(){
  exitAltScreen();
  tcsetattr(STDIN_FILENO,TCSAFLUSH,&original);
  write(STDOUT_FILENO,"\033[?25h",6);
}
