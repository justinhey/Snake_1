﻿



#include<stdio.h>
#include<stdlib.h>
#include<ncurses.h>
#include<sys/time.h>
#include<signal.h>
#include"snake.h"




struct position food;		/* food position */
Snake_Node *head, *tail;	/* double linked list's head and tail */
int x_dir = 1, y_dir = 0;	/* init dirction of the snake moving */
int ttm = 5, ttg = 5;		/* two timers defined to control speed */



void main(void)
{
	Init_Disp();
	Food_Disp();
	DLL_Snake_Create();
	signal(SIGALRM, Snake_Move);
	set_ticker(DFL_SPEED);
	Key_Ctrl();
	Wrap_Up();
}


void Init_Disp()
{
	char wall = ' ';
	int i, j;
	initscr();              // 初始化curses库和tty
	cbreak();				// 开启输入立即响应
	noecho();
	curs_set(0);			// 	使光标不可见

	/* display some message about title and wall */
	attrset(A_NORMAL);		// 普通字符输出(不加亮显示)
	attron(A_REVERSE);		// 字符反白显示
	for (int i = 0; i < LINES; i++)
	{
		mvaddch(i, LEFT_EDGE, wall);
		mvaddch(i, RIGHT_EDGE, wall);
	}
	for (int j = 0; j < COLS; j++)
	{
		mvaddch(0, j, '=');
		mvaddch(TOP_ROW, j, wall);
		mvaddch(BOT_ROW, j, wall);
	}
	attroff(A_REVERSE);		/* turn off REVERSE */
	mvaddstr(1, 2, "Game: snake    version: 1.0    date: 2011/08/22");
	mvaddstr(2, 2, "Author: Dream Fly	Blog: blog.csdn.net/jjzhoujun2010");
	mvaddstr(3, 2, "Usage: Press 'f' to speed up, 's' to speed down,'q' to quit.");
	mvaddstr(4, 2, "       Nagivation key controls snake moving.");
	refresh();
}



/*
 * Function: Food_Disp()
 * Usage: display food position
 * Return: none
 */
void Food_Disp()
{
	/*
	  Srand是种下随机种子数，你每回种下的种子不一样，
	  用Rand得到的随机数就不一样。为了每回种下一个
	  不一样的种子，所以就选用Time(0)，Time(0)是得到
	  当前时时间值（因为每时每刻时间是不一样的了）
	*/
	srand(time(0));
	// rand() % (COLS - 2) 生成 0到(COLS - 2)范围内的随机数
	food.x_pos = rand() % (COLS - 2) + 1;
	food.y_pos = rand() % (LINES - TOP_ROW - 2) + TOP_ROW + 1;
	mvaddch(food.y_pos, food.x_pos, FOOD_SYMBOL);/* display the food */
	refresh();
}

/*
 * Function: DLL_Snake_Create()
 * Usage: create double linked list, and display the snake first node
 * Return: none
 */
void DLL_Snake_Create()
{
	Snake_Node *temp = (Snake_Node *)malloc(sizeof(Snake_Node));
	head = (Snake_Node *)malloc(sizeof(Snake_Node));
	tail = (Snake_Node *)malloc(sizeof(Snake_Node));
	if (temp == NULL || head == NULL || tail == NULL)
	{
		perror("malloc");
	}
	temp->x_pos = 5;
	temp->y_pos = 10;
	head->prev = NULL;
	tail->next = NULL;
	head->next = temp;
	temp->next = tail;
	tail->prev = temp;
	temp->prev = head;
	mvaddch(temp->y_pos, temp->x_pos, SNAKE_SYMBOL);
	refresh();
}

/*
 *  接下来就是蛇的移动问题，这个是核心部分以及最困难的设计部分了，我采用的是蛇用双向链表的结构来构造出来，
	分别有一个head 和tail指针，用来添加和删除元素。这里若要实现移动的话（未碰到食物前），就是在链表的
	头部（head的下一个）插入一个新元素，记录下此时的坐标，用mvaddch(y,x,c)函数添加蛇的图形'@',与此同时，
	在链表尾部（tail的前一个）删除一个节点，同时这里的坐标用mvaddch(y,x, ' ')添加了' '空白字符，实现删除
	效果，最后加上refresh(). 这样就可以看到蛇在“移动”了。当然，要是碰到食物的话，尾部节点处就不用删除，
	达到增长长度的效果。
	那么，接下来的问题是：如何触发蛇的移动呢？如何实现均匀移动以及通过按键 ‘f’ 或 's' 改变运动速度呢？
	这里我采用的是信号计时中断调用的函数  signal(SIGALRM, Snake_Move) 和 间隔计数器 来实现，通过产生相同
	间隔的时间片段来不断地调用Snake_Move()函数来执行相应的功能。加减速的功能是通过设定其他变量ttm, ttg来
	实现再此基本计数器上面再次分频的效果来加减速，ttm, ttg 越大，减速越明显，反之则相反效果。  具体的间隔
	计数器的函数设计见下：（参考了以上所提书本上的函数）
 */

 /*
  * Function: set_ticker(number_of_milliseconds)
  * Usage: arrange for interval timer to issue SIGALRM's at regular intervals
  * Return: -1 on error, 0 for ok
  * arg in milliseconds, converted into whole seconds and microseconds
  * note: set_ticker(0) turns off ticker
  */
int set_ticker(int n_msecs)
{
	struct itimerval new_timeset;
	long n_sec, n_usecs;

	n_sec = n_msecs / 1000;
	n_usecs = (n_msecs % 1000) * 1000L;

	// 间隔
	new_timeset.it_interval.tv_sec = n_sec;
	new_timeset.it_interval.tv_usec = n_usecs;
	// 初始值
	new_timeset.it_value.tv_sec = n_sec;
	new_timeset.it_value.tv_usec = n_usecs;

	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}


void Snake_Move()
{
	static int length = 1;	/* length of snake */
	int Length_Flag = 0;	/* default snake's length no change */
	int moved = 0;

	// SIG_ALRM 警告钟, SIG_IGN 屏蔽该信号
	signal(SIG_ALRM, SIG_IGN);

	/*
		judge if the snake crash the wall
		屏幕左上角是原点，向右是x正向，向下是y轴正向
	*/
	if ((head->next->x_pos == RIGHT_EDGE - 1 && x_dir == 1)
		|| (head->next->x_pos == LEFT_EDGE + 1 && x_dir == -1)
		|| (head->next->y_pos == TOP_ROW + 1 && y_dir = -1)
		|| (head->next->y_pos == BOT_ROW - 1 && y_dir == 1))
	{
		gameover(1);
	}
	/*
		judge if the snake crash itself
		inch() 函数和相关联的 mvinch() 函数允许你探测屏幕的内容
	*/
	if (mvinch(head->next->y_pos + y_dir, head->next->x_pos + x_dir) == '@')
	{
		gameover(2);
	}
	// 信号每收到一次这个函数就执行一次，ttg-- 之后值是1 蛇才会动
	if (ttm > 0 && ttg-- == 1)
	{
		/* snake moves */
		DLL_Snake_Insert(head->next->x_pos + x_dir, head->next->y_pos + y_dir);
		ttg = ttm;	/* reset */
		moved = 1;	/* snake can move */
	}
	if (moved)
	{
		/* snake eat the food */
		if (head->next->x_pos == food.x_pos && head->next->y_pos == food.y_pos)
		{
			Length_Flag = 1;
			length++;
			/* Mission Complete */
			if (length >= MAX_NODE)
			{
				gameover(0);
			}
			/* reset display the food randomly */
			Food_Disp();
		}
		if (Length_Flag == 0)
		{
			/* delete the tail->prev node */
			mvaddch(tail->prev->y_pos, tail->prev->x_pos);
			DLL_Snake_Delete_Node();
		}
		mvaddch(head->next->y_pos, head->next->x_pos, SNAKE_SYMBOL);
		refresh();
	}
	signal(SIGALRM, Snake_Move);
}


/* Function: Wrap_Up()
 * Usage: turn off the curses
 * Return: none
 */
void Wrap_Up()
{
	set_ticker(0);		/* turn off the timer */
	getchar();
	endwin();
	exit(0);
}


/* Function: Key_Ctrl()
 * Usage: using keyboard to control snake action; 'f' means speed up,
 * 's' means speed down, 'q' means quit, navigation key control direction.
 * Return: none
 */
void Key_Ctrl()
{
	int c;
	keypad(stdscr, true);			/* use little keyboard Navigation Key */
	while (c = getch(), c != 'q')
	{
		if (c == 'f')
		{
			if (ttm == 1) {
				continue;
			}
			ttm--;
		}
		else if (c == 's')
		{
			if (ttm == 8) {
				continue;
			}
			ttm++;
		}

		if (c == KEY_LEFT)
		{
			// 只有蛇头没有蛇身时tail->prev->prev就是head节点
			if (tail->prev->prev->prev != NULL && x_dir == 1 & y_dir == 0) {
				continue; // it can't turn reverse when snake have length
			}
			x_dir = -1;
			y_dir = 0;
		}
		else if (c == KEY_RIGHT)
		{
			if (tail->prev->prev->prev != NULL && x_dir == -1 && y_dir == 0)
			{
				continue;
			}
			x_dir = 1;
			y_dir = 0;
		}
		else if (c == KEY_UP)
		{
			if (tail->prev->prev->prev != NULL && x_dir == 0 && y_dir == 1)
			{
				continue;
			}
			x_dir = 0;
			y_dir = -1;

		}
		else if (c == KEY_DOWN)
		{
			if (tail->prev->prev->prev != NULL && x_dir == 0 && y_dir == -1)
			{
				continue;
			}
			x_dir = 0;
			y_dir = 1;
		}
	}
}

/* Function: DLL_Snake_Insert(int x, int y)
 * Usage: Insert node in the snake.
 * Return: none
 */
void Dll_Snake_Insert(int x, int y)
{
	Snake_Node *temp = (Snake_Node *)malloc(sizeof(Snake_Node));
	if (temp == NULL)
	{
		perror("malloc");
	}
	temp->x_pos = x;
	temp->y_pos = y;
	temp->prev = head->next->prev;
	head->next->prev = temp;
	temp->next = head->next;
	head->next = temp;
}


/* Function: gameover(int n)
 * Usage: gameover(0) means Mission Completes; gameover(1) means crashing
 * the wall; gameover(2) means crash itself.
 * Return: none
 */
void gameover(int n) {
	switch (n)
	{
		case 0:
			mvaddstr(LINES / 2, COLS / 3 - 4, "Mission Completes,press any key to exit.\n");
			break;
		case 1:
			mvaddstr(LINES / 2, COLS / 3 - 4, "Game Over, crash the wall,press any key to exit.\n");
			break;
		case 2:
			mvaddstr(LINES / 2, COLS / 3 - 4, "Game Over, crash yourself,press any key to exit.\n");
		default:
			break;
	}
	refresh();
	/* delete the whole double linked list */
	DLL_Snake_Delete();
	Wrap_Up();
}

/* Function: DLL_Snake_Delete_Node()
 * Usage: delete a tail node, not the whole linked list
 * Return: none
 */
void DLL_Snake_Delete_Node()
{
	Snake_Node *temp;
	temp = tail->prev;
	tail->prev = tail->prev->prev;
	temp->prev->next = tail;
	free(temp);
}

/* Function: DLL_Snake_Delete()
 * Usage: delete the whole double linked list
 * Return: none
 */
void Dll_Snake_Delete()
{
	while (head->next != tail)
	{
		DLL_Snake_Delete_Node();
	}
	head->next = tail->prev = NULL;
	free(head);
	free(tail);
}




