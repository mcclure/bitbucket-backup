#ifndef __SPROXEL_PY_CONSOLE_H__
#define __SPROXEL_PY_CONSOLE_H__


void init_python_console();
void close_python_console();

void pycon_raw(const char *s);
void pycon(const char *fmt, ...);

class ConsoleWidget* get_python_console_widget();


#endif
