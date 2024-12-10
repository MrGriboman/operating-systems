// socat -d -d pty,raw,echo=0 pty,raw,echo=0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // For sleep()

#define USB_DEV_READ "/dev/pts/4"
#define TEMP_LOG_FILE "./hourly_avg_temp.txt"
#define DAILY_LOG_FILE "./daily_avg_temp.txt"

// Function to flush incomplete line data
void flush_incomplete_line(FILE *usb_read) {
  int ch;
  while ((ch = fgetc(usb_read)) != EOF) {
    if (ch == '\n' || ch == '\0') {
      break; // Stop flushing at the end of the current line
    }
  }
}

int main() {
  FILE *usb_read = fopen(USB_DEV_READ, "r");
  if (usb_read == NULL) {
    perror("Couldn't open USB device");
    exit(EXIT_FAILURE);
  }

  FILE *log_file = fopen("./log.txt", "a");
  if (log_file == NULL) {
    perror("Couldn't open the log file");
    exit(EXIT_FAILURE);
  }

  FILE *avg_log_file = fopen(TEMP_LOG_FILE, "a");
  if (avg_log_file == NULL) {
    perror("Couldn't open the hourly average temperature log file");
    exit(EXIT_FAILURE);
  }

  FILE *daily_log_file = fopen(DAILY_LOG_FILE, "a");
  if (daily_log_file == NULL) {
    perror("Couldn't open the daily average temperature log file");
    exit(EXIT_FAILURE);
  }

  // Flush incomplete line on start
  flush_incomplete_line(usb_read);

  double temp_sum = 0.0, daily_temp_sum = 0.0;
  int temp_count = 0, daily_temp_count = 0;
  time_t start_time = time(NULL);
  time_t current_time;

  struct tm *local_time = localtime(&start_time);
  int current_day = local_time->tm_mday; // Track the current day

  char line[256]; // Fixed-size buffer for a single line
  size_t line_pos = 0;

  while (true) {
    int ch = fgetc(usb_read);

    // Check for EOF or read error
    if (ch == EOF) {
      clearerr(usb_read); // Clear EOF for continued reading
      sleep(1);           // Synchronize with USB device
      continue;
    }

    // Accumulate characters into `line` buffer
    if (line_pos < sizeof(line) - 1) { // Ensure space for null terminator
      line[line_pos++] = (char)ch;
    }

    // Check for line termination (null terminator or newline)
    if (ch == '\0' || ch == '\n') {
      line[line_pos] = '\0'; // Null-terminate the string

      // Log raw data
      fprintf(log_file, "%s", line);
      fflush(log_file);

      // Extract temperature value
      char *temp_ptr = strstr(line, "Temp: ");
      if (temp_ptr != NULL) {
        temp_ptr += 6;                // Move past "Temp: "
        double temp = atof(temp_ptr); // Parse temperature
        temp_sum += temp;
        temp_count++;
        daily_temp_sum += temp;
        daily_temp_count++;
      }

      // Reset line buffer
      line_pos = 0;

      // Check if an hour has passed
      current_time = time(NULL);
      if (difftime(current_time, start_time) >= 5) {
        // Calculate hourly average temperature
        double avg_temp = temp_count > 0 ? temp_sum / temp_count : 0.0;

        // Log hourly average temperature
        struct tm *lt = localtime(&current_time);
        fprintf(avg_log_file, "Time: %d/%d/%d %d:%d:%d Temp: %.2f\n", lt->tm_mday, lt->tm_mon, lt->tm_year, lt->tm_hour, lt->tm_min, lt->tm_sec, avg_temp);
        fflush(avg_log_file);

        // Reset hourly counters
        temp_sum = 0.0;
        temp_count = 0;
        start_time = current_time;
      }

      // Check if a new day has started
      struct tm *current_local_time = localtime(&current_time);
      if (current_local_time->tm_mday != current_day) {
        // Calculate daily average temperature
        double daily_avg_temp =
            daily_temp_count > 0 ? daily_temp_sum / daily_temp_count : 0.0;

        // Log daily average temperature
        fprintf(daily_log_file,
                "Date: %02d/%02d/%04d Temp: %.2f\n",
                current_local_time->tm_mday, current_local_time->tm_mon + 1,
                current_local_time->tm_year + 1900, daily_avg_temp);
        fflush(daily_log_file);

        // Reset daily counters
        daily_temp_sum = 0.0;
        daily_temp_count = 0;

        // Update the current day
        current_day = current_local_time->tm_mday;
      }
      sleep(1);
    }
  }

  fclose(usb_read);
  fclose(log_file);
  fclose(avg_log_file);
  fclose(daily_log_file);
  return 0;
}
