#define AL_C_DATA 1
#define AL_C_START 2
#define AL_C_END 3

#define AL_T_SIZE 0
#define AL_T_NAME 1

typedef enum{
  SENDER, RECEIVER
} Mode;

typedef struct {
  int filedescriptor;
  int status;
} Applicationlayer;

int sender(Applicationlayer app, const char* device_name);

int receiver(Applicationlayer app);
