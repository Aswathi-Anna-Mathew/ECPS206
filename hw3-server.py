from socket import *
import RPi.GPIO as GPIO
import time
import Adafruit_TCS34725

tcs = Adafruit_TCS34725.TCS34725(integration_time=Adafruit_TCS34725.TCS34725_INTEGRATIONTIME_154MS, gain=Adafruit_TCS34725.TCS34725_GAIN_1X)

# initialize GPIO pins
GPIO.setmode(GPIO.BOARD)
GPIO.setup(29, GPIO.OUT)                                    # GREEN LED
GPIO.setup(31, GPIO.OUT)                                    # RED LED

# set all LEDs to False value
GPIO.output(29, False)
GPIO.output(31, False)

tcs.set_interrupt(False)

values = [0] * 5
iteration = 0
average = 0


def calculate_avg(c):
    avg = 0
    values[iteration % 5] = c
    if (iteration > 5):
        for i in range(5):
            avg += values[i]
    return avg/5

# Create a TCP/IP socket
sock = socket(AF_INET, SOCK_DGRAM)

# Bind the socket to the port
server_address = ('169.234.34.161', 50000)
sock.bind(server_address)

sock.settimeout(2.5)

try:
    while True:
        r, g, b, c = tcs.get_raw_data()
        iteration += 1
        average = calculate_avg(c)

        print('waiting to receive message')
        try:
            data, address = sock.recvfrom(65536)
            if data:
                sent = sock.sendto(str(c), address)
                print('c value: ' + str(data) + ' , average: ' + str(average))
                val = int(data.strip('\0'))
                if (c > val):
                    GPIO.output(29, True)
                    GPIO.output(31, False)
                elif (val > c):
                    GPIO.output(29, False)
                    GPIO.output(31, True)

        except timeout as e:
            GPIO.output(29, True)
            GPIO.output(31, True)
            continue

finally:
    GPIO.cleanup()

