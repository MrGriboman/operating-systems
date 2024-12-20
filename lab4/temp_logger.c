// socat -d -d pty,raw,echo=0 pty,raw,echo=0

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // For sleep()

#define USB_DEV_READ "/dev/pts/4"
#define LOG_FILE "./log.txt"
#define HOUR_LOG_FILE "./hourly_avg_temp.txt"
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

void clean_log_file(FILE *log_file, struct tm *lt, char *file_name) {
  char buffer[256];
  char **valid_lines = NULL; // Array to store valid lines
  size_t valid_line_count = 0;

  rewind(log_file); // Ensure we start at the beginning of the file
  while (fgets(buffer, sizeof(buffer), log_file)) {
    char *time_ptr = strstr(buffer, "Time: ");
    if (!time_ptr)
      continue; // Skip lines without "Time: "

    time_ptr += 6; // Move pointer to the time value
    int day = atoi(time_ptr);
    time_ptr = strchr(time_ptr, '/') + 1;
    int month = atoi(time_ptr);
    time_ptr = strchr(time_ptr, '/') + 1;
    int year = atoi(time_ptr);

    // If the day is greater or equal to the current day, keep the line
    if (day >= lt->tm_mday && file_name == LOG_FILE ||
        month >= lt->tm_mon && file_name == HOUR_LOG_FILE ||
        year >= lt->tm_year && file_name == DAILY_LOG_FILE) {
      valid_lines =
          realloc(valid_lines, sizeof(char *) * (valid_line_count + 1));
      valid_lines[valid_line_count] = strdup(buffer); // Save a copy of the line
      valid_line_count++;
    } else
      break;
  }

  // Truncate the file and rewrite with valid lines
  freopen(file_name, "w", log_file); // Reopen in write mode to clear the file
  for (size_t i = 0; i < valid_line_count; i++) {
    fputs(valid_lines[i], log_file); // Write back valid lines
    free(valid_lines[i]);            // Free the allocated memory for the line
  }
  free(valid_lines);
  freopen(file_name, "a+", log_file);
}

int main() {
  FILE *usb_read = fopen(USB_DEV_READ, "r");
  if (usb_read == NULL) {
    perror("Couldn't open USB device");
    exit(EXIT_FAILURE);
  }

  FILE *log_file = fopen("./log.txt", "a+");
  if (log_file == NULL) {
    perror("Couldn't open the log file");
    exit(EXIT_FAILURE);
  }

  FILE *hour_log_file = fopen(HOUR_LOG_FILE, "a+");
  if (hour_log_file == NULL) {
    perror("Couldn't open the hourly average temperature log file");
    exit(EXIT_FAILURE);
  }

  FILE *daily_log_file = fopen(DAILY_LOG_FILE, "a+");
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
      struct tm *lt = localtime(&current_time);
      if (difftime(current_time, start_time) >= 3600) {
        // Calculate hourly average temperature
        double avg_temp = temp_count > 0 ? temp_sum / temp_count : 0.0;

        // Log hourly average temperature
        fprintf(hour_log_file, "Time: %d/%d/%d %d:%d:%d Temp: %.2f\n",
                lt->tm_mday, lt->tm_mon, lt->tm_year, lt->tm_hour, lt->tm_min,
                lt->tm_sec, avg_temp);
        fflush(hour_log_file);

        // Reset hourly counters
        temp_sum = 0.0;
        temp_count = 0;
        start_time = current_time;

        // cleanup
        clean_log_file(log_file, lt, LOG_FILE);
        clean_log_file(hour_log_file, lt, HOUR_LOG_FILE);
        clean_log_file(daily_log_file, lt, DAILY_LOG_FILE);
      }

      // Check if a new day has started
      if (lt->tm_mday != current_day) {
        // Calculate daily average temperature
        double daily_avg_temp =
            daily_temp_count > 0 ? daily_temp_sum / daily_temp_count : 0.0;

        // Log daily average temperature
        fprintf(daily_log_file, "Date: %02d/%02d/%04d Temp: %.2f\n",
                lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900,
                daily_avg_temp);
        fflush(daily_log_file);

        // Reset daily counters
        daily_temp_sum = 0.0;
        daily_temp_count = 0;

        // Update the current day
        current_day = lt->tm_mday;
      }

      sleep(1);
    }
  }

  fclose(usb_read);
  fclose(log_file);
  fclose(hour_log_file);
  fclose(daily_log_file);
  return 0;
}
