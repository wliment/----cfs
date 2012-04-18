  #include <ncurses.h> 

int main() 
{ 
   initscr();          /*  初 化，进入NCURSES 模式               */ 
   printw("Hello World !!!"); /*  在虚拟屏幕上打印Hello, World!!!    */ 
   refresh();          /*  将虚拟屏幕上的内容写到显示器上，并刷新  */ 
	printw("sssssssssssssssssss");
   refresh();          /*  将虚拟屏幕上的内容写到显示器上，并刷新  */ 
   getch();            /*  等待用户输入                            */ 
   endwin();           /*  退出NCURSES 模式                      */ 

	return 0; 
} 