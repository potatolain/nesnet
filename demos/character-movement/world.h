// Quick-n-dirty little header file to hide some forward declarations of functions our hello world uses.
// Realistically, this only exists so I can show main right away, which demonstrates the library well.
// Stop staring at this header and questioning why I'd even think about this, and go play with NESNet! ;)
void put_str(unsigned int adr, const char *str);
void clear_screen();
void show_boilerplate();
void show_connection_failure();
void show_the_message(char* whatIsThis);