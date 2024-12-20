#include "temp_logger.h"

#define LOG_FILE "./log.txt"
#define HOUR_LOG_FILE "./hourly_avg_temp.txt"
#define DAILY_LOG_FILE "./daily_avg_temp.txt"

#ifdef _WIN32
#define USB_DEV_READ "COM5"
#else
#define USB_DEV_READ "/dev/pts/4"
#endif

#ifdef _WIN32
HANDLE open_com_port(const char *port_name)
{
  HANDLE hComm = CreateFileA(port_name,
                             GENERIC_READ,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

  if (hComm == INVALID_HANDLE_VALUE)
  {
    printf("Error opening port %s (error code: %ld)\n", port_name, GetLastError());
    exit(EXIT_FAILURE);
  }

  // Configure the COM port
  DCB dcb = {0};
  dcb.DCBlength = sizeof(dcb);
  dcb.BaudRate = CBR_9600;
  dcb.ByteSize = 8;
  dcb.StopBits = ONESTOPBIT;
  dcb.Parity = NOPARITY;

  if (!SetCommState(hComm, &dcb))
  {
    printf("Error setting port state (error code: %ld)\n", GetLastError());
    CloseHandle(hComm);
    exit(EXIT_FAILURE);
  }

  return hComm;
}

DWORD read_com_port(HANDLE port, char *buffer, size_t size)
{
  printf("reading...\n");
  DWORD received;
  BOOL success = ReadFile(port, buffer, size, &received, NULL);
  if (!success)
  {
    printf("Failed to read from port");
    return -1;
  }
  printf("Read.\n");
  return received;
}

void flush_incomplete_line(HANDLE hComm)
{
  char ch;
  DWORD bytes_read;
  while (ReadFile(hComm, &ch, 1, &bytes_read, NULL) && bytes_read > 0)
  {
    if (ch == '\n' || ch == '\0')
    {
      break; // Stop flushing at the end of the current line
    }
  }
}
#else
void flush_incomplete_line(FILE *usb_read)
{
  int ch;
  while ((ch = fgetc(usb_read)) != EOF)
  {
    if (ch == '\n' || ch == '\0')
    {
      break; // Stop flushing at the end of the current line
    }
  }
}
#endif

void clean_log_file(FILE *log_file, struct tm *lt, const char *file_name)
{
  char buffer[256];
  char **valid_lines = NULL;
  size_t valid_line_count = 0;

  rewind(log_file);
  while (fgets(buffer, sizeof(buffer), log_file))
  {
    char *time_ptr = strstr(buffer, "Time: ");
    if (!time_ptr)
      continue;

    time_ptr += 6;
    int day = atoi(time_ptr);
    time_ptr = strchr(time_ptr, '/') + 1;
    int month = atoi(time_ptr);
    time_ptr = strchr(time_ptr, '/') + 1;
    int year = atoi(time_ptr);

    if ((day >= lt->tm_mday && strcmp(file_name, LOG_FILE) == 0) ||
        (month >= lt->tm_mon && strcmp(file_name, HOUR_LOG_FILE) == 0) ||
        (year >= lt->tm_year && strcmp(file_name, DAILY_LOG_FILE) == 0))
    {
      valid_lines = realloc(valid_lines, sizeof(char *) * (valid_line_count + 1));
      valid_lines[valid_line_count] = strdup(buffer);
      valid_line_count++;
    }
  }

  freopen(file_name, "w", log_file);
  for (size_t i = 0; i < valid_line_count; i++)
  {
    fputs(valid_lines[i], log_file);
    free(valid_lines[i]);
  }
  free(valid_lines);
  freopen(file_name, "a+", log_file);
}

int main()
{
#ifdef _WIN32
  HANDLE hComm = open_com_port(USB_DEV_READ);
#else
  FILE *usb_read = fopen(USB_DEV_READ, "r");
  if (usb_read == NULL)
  {
    perror("Couldn't open USB device");
    exit(EXIT_FAILURE);
  }
#endif

  FILE *log_file = fopen(LOG_FILE, "a+");
  if (log_file == NULL)
  {
    perror("Couldn't open the log file");
    exit(EXIT_FAILURE);
  }

  FILE *hour_log_file = fopen(HOUR_LOG_FILE, "a+");
  if (hour_log_file == NULL)
  {
    perror("Couldn't open the hourly average temperature log file");
    exit(EXIT_FAILURE);
  }

  FILE *daily_log_file = fopen(DAILY_LOG_FILE, "a+");
  if (daily_log_file == NULL)
  {
    perror("Couldn't open the daily average temperature log file");
    exit(EXIT_FAILURE);
  }

#ifdef _WIN32
  flush_incomplete_line(hComm);
#else
  flush_incomplete_line(usb_read);
#endif
  double temp_sum = 0.0, daily_temp_sum = 0.0;
  int temp_count = 0, daily_temp_count = 0;
  time_t start_time = time(NULL);
  time_t current_time;

  struct tm *local_time = localtime(&start_time);
  int current_day = local_time->tm_mday;

  char line[256];
  size_t line_pos = 0;

  while (true)
  {
    char ch = 'a';
#ifdef _WIN32
    printf("REading begins\n");
    int bytes_read = read_com_port(hComm, &ch, 1);
    if (bytes_read <= 0)
    {
      printf("nothing to read\n");
      Sleep(1000);
      continue;
    }
#else
    int ch_int = fgetc(usb_read);
    if (ch_int == EOF)
    {
      clearerr(usb_read);
      sleep(1);
      continue;
    }
    ch = (char)ch_int;
#endif

    if (line_pos < sizeof(line) - 1)
    {
      line[line_pos++] = ch;
    }

    if (ch == '\n' || ch == '\0')
    {
      line[line_pos] = '\0';

      fprintf(log_file, "%s", line);
      fflush(log_file);

      char *temp_ptr = strstr(line, "Temp: ");
      if (temp_ptr)
      {
        temp_ptr += 6;
        double temp = atof(temp_ptr);
        temp_sum += temp;
        temp_count++;
        daily_temp_sum += temp;
        daily_temp_count++;
      }

      line_pos = 0;

      current_time = time(NULL);
      struct tm *lt = localtime(&current_time);

      if (difftime(current_time, start_time) >= 3600)
      {
        double avg_temp = temp_count > 0 ? temp_sum / temp_count : 0.0;
        fprintf(hour_log_file, "Time: %02d/%02d/%04d %02d:%02d:%02d Temp: %.2f\n",
                lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900,
                lt->tm_hour, lt->tm_min, lt->tm_sec, avg_temp);
        fflush(hour_log_file);

        temp_sum = 0.0;
        temp_count = 0;
        start_time = current_time;

        clean_log_file(log_file, lt, LOG_FILE);
        clean_log_file(hour_log_file, lt, HOUR_LOG_FILE);
        clean_log_file(daily_log_file, lt, DAILY_LOG_FILE);
      }

      if (lt->tm_mday != current_day)
      {
        double daily_avg_temp = daily_temp_count > 0 ? daily_temp_sum / daily_temp_count : 0.0;
        fprintf(daily_log_file, "Date: %02d/%02d/%04d Temp: %.2f\n",
                lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900,
                daily_avg_temp);
        fflush(daily_log_file);

        daily_temp_sum = 0.0;
        daily_temp_count = 0;
        current_day = lt->tm_mday;
      }

#ifdef _WIN32
      Sleep(1000);
#else
      sleep(1);
#endif
    }
  }

#ifdef _WIN32
  CloseHandle(hComm);
#else
  fclose(usb_read);
#endif
  fclose(log_file);
  fclose(hour_log_file);
  fclose(daily_log_file);
  return 0;
}
