import serial, time
from time import gmtime, strftime
from datetime import datetime
import sys, math

#Collect data
ser = serial.Serial('/dev/ttyUSB0', baudrate=115200)

#Configure the labels for the database
device = "rf_mini_gas"
location = "gas_cellar"

with open("out.log", "w") as f:
    try:
        while True:
            now = datetime.now()
            iso = datetime.utcnow()
            data = ser.readline()

            if b"," not in data:
                print("non-measurement-output: ", data)
                continue

            magx, magy, magz, temp, volt, rssi = data.decode().strip().split(",")
            magx, magy, magz, rssi = map(int, [magx, magy, magz, rssi])
            temp, volt = map(float, [temp, volt])

            mmag = math.sqrt(magx*magx+magy*magy+magz*magz)
            
            # Print for debugging, uncomment the below line
            #print(magx, magy, magz, temp, volt, rssi)
            print(f"[{iso}] MX: {magx}, MY: {magy}, MZ: {magz}, |M|: {mmag:.2f}, temp: {temp}, volt: {volt}, rssi: {rssi}")
            f.write(f"[{iso}] MX: {magx}, MY: {magy}, MZ: {magz}, |M|: {mmag:.2f}, temp: {temp}, volt: {volt}, rssi: {rssi}")
            


    except KeyboardInterrupt:
        pass