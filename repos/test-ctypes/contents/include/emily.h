// Call exactly once, at startup.
void emilyStartup();

// Alternate forms of emilyStartup. Supported args are same as command line app.
void emilyStartupArgs(char **argv);                // Null terminated
void emilyStartupCountArgs(int argc, char **argv); // Count provided

extern int (*callbackTest)(int x);