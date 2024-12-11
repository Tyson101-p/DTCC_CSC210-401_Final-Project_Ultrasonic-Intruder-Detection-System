/*
DTCC CSC210-401
Tai Pham
Final project
-----------------------

This program control a Raspberry Pi with additional componets to detect when there is an intruder getting closer

*/

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// Define GPIO pins
#define TRIG 18  // GPIO18
#define ECHO 24  // GPIO24
#define BUZZER 17 // GPIO17
#define LED 22    // GPIO22

// Timeout for ECHO signal (in microseconds)
#define TIMEOUT 50000  // 50 milliseconds

// Log file pointer
FILE *logFile = NULL;

// Function to get the current time as a string
const char* currentTime() {
    static char buffer[20];
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

// Function to measure distance using the ultrasonic sensor
float measureDistance() {
    // Trigger the ultrasonic sensor
    digitalWrite(TRIG, HIGH);
    usleep(10); // 10 microseconds pulse
    digitalWrite(TRIG, LOW);

    // Wait for the ECHO to go HIGH
    long startTime = micros();
    while (digitalRead(ECHO) == LOW) {
        if (micros() - startTime > TIMEOUT) {
            printf("Timeout waiting for ECHO\n");
            return -1;  // Timeout occurred
        }
    }

    startTime = micros();
    // Wait for the ECHO to go LOW
    while (digitalRead(ECHO) == HIGH) {
        if (micros() - startTime > TIMEOUT) {
            printf("Timeout waiting for ECHO\n");
            return -1;  // Timeout occurred
        }
    }
    long travelTime = micros() - startTime;

    // Calculate distance (speed of sound = 34300 cm/s)
    float distance = (travelTime / 2.0) * 0.0343;
    
    // Sanity check for distance
    if (distance < 0 || distance > 500) {
        printf("Invalid distance measurement: %.2f cm\n", distance);
        return -1;  // Invalid reading
    }

    return distance;
}

// Function to log the intruder detection to a file
void logIntruderDetection(float distance) {
    if (logFile != NULL) {
        fprintf(logFile, "Intruder detected at %s - Distance: %.2f cm\n", currentTime(), distance);
        fflush(logFile);  // Ensure the data is written to the file immediately
    } else {
        printf("Error: Log file not open\n");
    }
}

// Exit handler for cleanup
void exitHandler(int sig) {
    printf("\nExiting the program...\n");

    // Turn off buzzer and LED
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED, LOW);

    // Close the log file before exiting
    if (logFile != NULL) {
        fclose(logFile);
    }
    
    exit(0);
}   

int main(int argc, char *argv[]) {
    // Initialize wiringPi library

    float threshold = 30.0;  // Default threshold in cm
    int delayTime = 500;     // Default delay time in ms

    // Parsing command-line arguments
    if (argc > 1) {
        threshold = atof(argv[1]);
    }
    if (argc > 2) {
        delayTime = atoi(argv[2]);
    }

    // Initialize wiringPi library
    if (wiringPiSetupGpio() == -1) {
        printf("Failed to initialize wiringPi\n");
        return 1;
    }

    // Set up GPIO pins
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(LED, OUTPUT);

    // Ensure the TRIG pin is low at startup
    digitalWrite(TRIG, LOW);
    usleep(2000); // 2 ms delay

    // Open the log file in append mode
    logFile = fopen("intruder_log.txt", "a");
    if (logFile == NULL) {
        printf("Error: Could not open log file\n");
        return 1;
    }

    // Set up signal handler for graceful exit (Ctrl+C)
    signal(SIGINT, exitHandler);

    printf("Ultrasonic Intruder Alert System Initialized.\n");
    printf("Threshold: %.2f cm, Delay: %d ms\n", threshold, delayTime);

    while (1) {
        float distance = measureDistance();

        if (distance == -1) {
            // Invalid reading, retry
            usleep(delayTime * 1000);
            continue;
        }

        printf("Distance: %.2f cm\n", distance);

        if (distance < threshold) { // Threshold for intruder detection (30 cm)
            printf("Intruder detected!\n");
            digitalWrite(BUZZER, HIGH);
            digitalWrite(LED, HIGH);

            // Log the intruder detection
            logIntruderDetection(distance);
        } else {
            digitalWrite(BUZZER, LOW);
            digitalWrite(LED, LOW);
        }

        delay(delayTime); // Check every 500 ms
    }

    return 0;
}
