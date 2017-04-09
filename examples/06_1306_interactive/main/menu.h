#ifndef __MENU_H_
#define __MENU_H_

#define THREAD_STACKSIZE 1024 // Empirically chosen

// maxlen is ignored, sets menu data
int printMenu(char *data,int maxlen);

void menu_application_thread(void *);

#endif
