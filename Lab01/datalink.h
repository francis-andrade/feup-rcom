struct linklayer{
  char port[20];
  int baudrate;
  unsigned int sequenceNumber;
  unsigned int timeout;
  unsigned int numTransmissions;
  char frame[MAX_SIZE];
}

void byte_stuff();
void byte_destuff();
